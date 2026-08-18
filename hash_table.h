// hash_table.h
#ifndef HASH_TABLE
#define HASH_TABLE

typedef struct ht_item {
    char* key;
    char* value;
} ht_item;

typedef struct linked_list {
    ht_item* item;
    struct linked_list* next; // ← Points to the next node (or NULL)
} linked_list;

typedef struct ht_hash_table {
    char* name;
    int size;
    int count;
    ht_item** items; // Contains an array of pointers to items.
    linked_list** buckets;
} ht_hash_table;

typedef struct ht_collection {
    int size;
    int count;
    ht_hash_table** tables;
} ht_collection;


void create_table(char* table_name);
void display_tables();
void drop_table(char* table_name);

void ht_insert(char* table_name, char* key, char* value);
void ht_search(char* table_name, char* key);
void ht_count_entries(char* table_name);
void ht_delete(char* table_name, char* key);

void create_collection(void);
void delete_collection(void);

#endif