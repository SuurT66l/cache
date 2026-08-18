// hash_table.c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "hash_table.h"

/* ============ CONSTANTS ============ */
#define MAX_COLLECTION_SIZE 20
#define TABLE_SIZE 10000

ht_collection* collection = NULL;

/* ============ PROTOTYPES ============ */
static ht_item* create_item(const char* key, const char* value);
void free_item(ht_item* item);
void free_buckets(ht_hash_table* table);

linked_list* linkedlist_insert(linked_list* head, ht_item* item);
void free_linkedlist(linked_list* node);

bool check_table_name_unique(char* table_name);
void handle_collision(ht_hash_table* table, int index, ht_item* item);
int update_bucket_if_duplicated(ht_hash_table* table, int index, char* key, char* value);
void move_tables_up(int table_idx);

/* ============ HELPERS ============ */
int get_hash(const char *str) {
    unsigned long hash = 5381; // Starting with a prime number for better distribution
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return (int)(hash % TABLE_SIZE);
}

bool check_table_name_unique(char* table_name){
    for(int i = 0; i < collection -> count; i++){
        if(collection->tables[i] != NULL &&
            strcmp(collection->tables[i]->name, table_name) == 0){
            return false;
        } 
    }
    return true;
}

ht_hash_table* find_table(char* table_name){
    for(int i = 0; i < collection -> count; i++){
        if(collection -> tables[i] != NULL && 
            strcmp(collection->tables[i]->name, table_name) == 0){
            return collection->tables[i];
        }
    }

    return NULL;
}

void handle_collision(ht_hash_table* table, int index, ht_item* item) {
    linked_list* head = table->buckets[index];
    table->buckets[index] = linkedlist_insert(head, item);
}

int update_bucket_if_duplicated(ht_hash_table* table, int index, char* key, char* value) {
    linked_list* current = table->buckets[index];
    while (current) {
        // Found duplicate → update value safely
        if (strcmp(current->item->key, key) == 0) {
            free(current->item->value);
            current->item->value = strdup(value);
            return 1;   // Successfully updated
        }
        current = current->next;
    }
    return 0;   // Not found
}

void move_tables_up(int table_idx) {
    for(int i = table_idx; i < collection->count; i++){
            collection->tables[i] = collection->tables[i+1];
            collection->tables[i+1] = NULL;
    }
}

/* ============ TABLES ============ */
void create_table(char* table_name) {
    if(collection->count == 20){ //Check if collection has space for more entries
        printf("Cannot create more than 20 tables\n");
        return;
    }

    bool isNameUnique = check_table_name_unique(table_name);
    if(!isNameUnique){
        printf("TABLE %s already exists\n", table_name);
        return;
    }

    ht_hash_table* table = (ht_hash_table *)malloc(sizeof(ht_hash_table));
    table->name = strdup(table_name);
    table->size = TABLE_SIZE;
    table->count = 0;
    table->items = (ht_item **)calloc(table->size, sizeof(ht_item*)); 
    table->buckets = (linked_list**)calloc(table->size, sizeof(linked_list*));
    for(int i = 0; i < MAX_COLLECTION_SIZE; i++){
        if(collection->tables[i] == NULL){
            collection->tables[i] = table;
            collection->count++;
            break;
        }
    }

    printf("TABLE %s created\n", table_name);
}

void display_tables(){
    if(collection->count <= 0){
        printf("No tables exist\n");
    } else {
        for(int i = 0; collection->tables[i] != NULL; i++){
            printf("%s\n", collection->tables[i]->name);
        }
    }
}

void drop_table(char* table_name) {
    ht_hash_table* table = find_table(table_name);

    if(!table) {
        printf("TABLE %s doesn't exist\n", table_name);
        return;
    }

    // Find index of the table
    int table_idx = -1;
    for(int i = 0; i < collection->count; i++){
        if(collection->tables[i] == table){
            table_idx = i;
            break;
        }
    }

    // Frees the table from items
    for (int i = 0; i < table->size; i++) {
        ht_item* item = table->items[i];
        if (item != NULL) {
            free_item(item);
        }
    }
    // Free the overflow bucket lists and its items.
    free_buckets(table);
    free(table->items);
    free(table->name);
    free(table);
    move_tables_up(table_idx); // Shift tables up to fill the gap
    collection->count--;
    printf("TABLE %s deleted\n", table_name);
}

void ht_insert(char* table_name, char* key, char* value) {
    ht_hash_table* table = find_table(table_name);

    if(!table) {
        printf("TABLE %s doesn't exist\n", table_name);
        return;
    }

    if(!key || !value) return;

    if(table->count == 10000) {
        printf("Table is full\n");
        return;
    }

    // Creates the item.
    ht_item* item = create_item(key, value);

    // Computes the index.
    int index = get_hash(key);

    ht_item *current_item = table->items[index];

    // Index should be without item 
    if(current_item == NULL){
        if(table->count == table->size) {
            //Hashtable is full
            printf("Table is full\n");
            free_item(item);
            return;
        }
        table->items[index] = item;
        table->count++;
        printf("VALUE %s at KEY %s added to TABLE %s\n", value, key, table_name);
    } else {
        // Scenario 1: Update the value(if keys the sames)
        if(strcmp(current_item->key, key) == 0) {
            free(table->items[index]->value);
            table->items[index]->value = strdup(value);
            free_item(item);
            printf("VALUE %s at KEY %s added to TABLE %s\n", value, key, table_name);
            return;
        } else { 
            // Scenario 2: Update bucket
            if(update_bucket_if_duplicated(table, index, key, value)){
                free_item(item);
                printf("VALUE %s at KEY %s added to TABLE %s\n", value, key, table_name);
                return;
            }

            // Check if table is already occupied with max amount of records
            if (table->count >= table->size) {
                printf("Table is full\n");
                free_item(item);
                return;
            }

            // Scenario 3: Handle the collision.
            handle_collision(table, index, item);
            table->count++;
            printf("VALUE %s at KEY %s added to TABLE %s\n", value, key, table_name);
            return;
        }
    }
}

void ht_search(char* table_name, char* key) {
    ht_hash_table* table = find_table(table_name);
    if(!table) {
        printf("TABLE %s doesn't exist\n", table_name);
        return;
    }

    int index = get_hash(key);
    ht_item* item = table->items[index];

    if(item != NULL) {
        if(strcmp(item->key, key) == 0){
            printf("VALUE: %s\n", item->value);
            return;
        }
    }

    // Look inside bucket
    linked_list* current_node = table->buckets[index]; // Head
    while(current_node != NULL){
        if(strcmp(current_node->item->key, key) == 0){
            printf("VALUE: %s\n", current_node->item->value);
            return;
        }
        current_node = current_node->next; // Move to next node
    }

    printf("KEY %s doesn't exist in TABLE %s\n", key, table_name);
}

void ht_count_entries(char* table_name){
    ht_hash_table* table = find_table(table_name);
    if(!table) {
        printf("TABLE %s doesn't exist\n", table_name);
        return;
    }
    printf("Item count in TABLE %s: %d\n", table_name, table->count);
}

void ht_delete(char* table_name, char* key) {
    ht_hash_table* table = find_table(table_name);
    if(!table) {
        printf("TABLE %s doesn't exist\n", table_name);
        return;
    }

    // Deletes an item from the table.
    int index = get_hash(key);
    ht_item* item = table->items[index];
    linked_list* head = table->buckets[index];

    // Item with such key doesn't exist
    if (item == NULL) {
        printf("KEY %s doesn't exist in TABLE %s\n", key, table_name);
        // Does not exist.
        return;
    } else {
        // Check if collision chain is empty 
        if (head == NULL) {
            //Check if key correspond to table entity
            if(strcmp(item->key, key) == 0){
                // Remove the item from the table itself
                // Set table index to NULL.
                table->items[index] = NULL;
                free_item(item);
                table->count--;
                printf("KEY %s deleted from TABLE %s\n", key, table_name);
                return;
            }
        } else if (head != NULL) {
            // Collision chain exists.
            if (strcmp(item->key, key) == 0) {
                free_item(item); // Delete from the table
                // Set the head of the bucket as the new item
                linked_list* node = head;
                head = head->next; // Move to next node
                node->next = NULL; // Clear reference to next node from previous head
                table->items[index] = create_item(node->item->key, node->item->value); // Set previously head node as an item
                free_linkedlist(node);
                table->buckets[index] = head;
                printf("KEY %s deleted from TABLE %s\n", key, table_name);
                return;
            }

            // If item key doesn't correspond
            // Look for elements in the bucket
            linked_list* curr = head;
            linked_list* prev = NULL;
            while (curr) {
                if (strcmp(curr->item->key, key) == 0) {
                    // First element of the bucket
                    if (prev == NULL) {
                        linked_list* new_head = head->next;
                        head->next = NULL;
                        free_linkedlist(head);
                        table->buckets[index] = new_head;
                        printf("KEY %s deleted from TABLE %s\n", key, table_name);
                        return;
                    } else {
                        // This is somewhere in the chain.
                        prev->next = curr->next;
                        curr->next = NULL;
                        free_linkedlist(curr);
                        table->buckets[index] = head; // Just in case
                        printf("KEY %s deleted from TABLE %s\n", key, table_name);
                        return;
                    }
                }
                prev = curr;
                curr = curr->next;
            }
        }
    }
}

/* ============ ITEMS ============ */
static ht_item* create_item(const char* key, const char* value) {
    ht_item* item = (ht_item *)malloc(sizeof(ht_item));
    item->key = strdup(key); // Allocates memory and returns pointer
    item->value = strdup(value);
    return item;
}

void free_item(ht_item* item) {
    // Frees an item.
    free(item->key);
    free(item->value);
    free(item);
}

void free_buckets(ht_hash_table* table) {
    // Free all the overflow bucket lists.
    linked_list** buckets = table->buckets;
    for (int i = 0; i < table->size; i++){
        free_linkedlist(buckets[i]);
    }

    free(buckets);
}

/* ============ LINKED LIST ============ */
linked_list* allocate_list() {
    // Allocates memory for a LinkedList pointer.
    linked_list* list = (linked_list*)malloc(sizeof(linked_list));
    return list;
}

linked_list* linkedlist_insert(linked_list* head, ht_item* item) {
    linked_list* new_node = allocate_list();
    if (!new_node) return head;
    new_node->item = item;
    new_node->next = NULL;
    // Inserts the head
    if (!head) {
        head = new_node;
        return head;
    }

    //Search for last linked item
    linked_list* temp = head;
    while (temp->next) {
        temp = temp->next;
    }
    temp->next = new_node;
    return head;
}

void free_linkedlist(linked_list* node) {
    linked_list* temp = node;

    while (node) {
        temp = node;
        node = node->next; //Move to  next node
        free(temp->item->key);
        free(temp->item->value);
        free(temp->item);
        free(temp);
    }
}

/* ======== COLLECTION ============ */
void create_collection(){
    // Allocate memory for collection
    collection = (ht_collection *)malloc(sizeof(ht_collection));

    collection->size = MAX_COLLECTION_SIZE;
    collection->count = 0;
    collection->tables = (ht_hash_table **)calloc(collection->size, sizeof(ht_hash_table*));

    // If something went wrong with filling collection with tables
    if (!collection->tables) {
        free(collection); 
        return;
    }
}

void delete_collection(){
    int idx = 0;
    while(collection->count > 0){
        idx = collection->count - 1; // Get the last index
        //printf("Deleting table: %s\n", collection->tables[0]->name);
        ht_hash_table* table = collection->tables[idx];

        // Frees the table.
        for (int i = 0; i < table->size; i++) {
            ht_item* item = table->items[i];
            if (item != NULL) {
                free_item(item);
            }
        }
        // Free the overflow bucket lists and its items.
        free_buckets(table);
        free(table->items);
        free(table->name);
        free(table);
        collection->count--;
    };

    free(collection->tables);
    free(collection);
}

/* ========== PRINT =============== */
void print_help(){
    printf("Available commands:\n");
    printf("\n");
    printf("HELP - HELP\n");
    printf("LIST - LIST\n");
    printf("CREATE - CREATE <table>\n");
    printf("ADD - ADD <table> <key> <value>\n");
    printf("COUNT - COUNT <table>\n");
    printf("GET - GET <table> <key>\n");
    printf("DELETE - DELETE <table> <key>\n");
    printf("DROP - DROP <table>\n");
    printf("EXIT - EXIT\n");
}