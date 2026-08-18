// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "hash_table.h"

// Prototypes
void process_command(char* input_line);
void print_help();

int main() {
	create_collection();

	printf("Welcome to Cache.\n");
	printf("Type HELP to see the usage.\n");
	printf("Type EXIT to exit.\n");
	while(1){
        printf("\n");
		printf("cache> ");

		char input_line[1024];
		if(!fgets(input_line, sizeof(input_line), stdin)) {
            printf("\nEOF received. Exiting...\n");
            break;
        }

        input_line[strcspn(input_line, "\n")] = '\0'; // Replace '\n' with '\0'

		process_command(input_line);
	}

	delete_collection();
}

void process_command(char* input_line){
    int const MAX_TOKEN_AMOUNT = 4;
    int tokenCounter = 0;
    
    // Array to store pointers to tokens
    char* tokenArr[MAX_TOKEN_AMOUNT];

    char* temp = strdup(input_line); // Create copy to work with strtok
    if (temp == NULL) return; 

    char* token = strtok(temp, " "); //Extract first token

    //If token is empty
    if(token == NULL) {
        printf("Error: invalid command\n");
        free(temp);
        return;
    }

    int command_index = -1;
    char commands[9][7] = {"HELP", "LIST", "CREATE", "ADD", "COUNT", "GET", "DELETE", "DROP", "EXIT"};

    //Extract all tokens
    while (token && tokenCounter <= MAX_TOKEN_AMOUNT) { // "=" to detect exceed amount of tokens
        tokenArr[tokenCounter] = token;
        token = strtok(NULL, " "); // Move to the next token
        tokenCounter++;
    }

    // Check for validity of command
    for(int i = 0; i < 9; i++){
        if(strcmp(tokenArr[0],commands[i]) == 0){
            command_index = i;
            break;
        }
    }

    //If it isn't valid command
    if(command_index == -1){
        printf("Error: invalid command\n");
        free(temp);
        return;
    } else {
        switch(command_index) {
            case 0: { // HELP
                if(tokenCounter != 1){
                    printf("Usage: HELP\n");
                } else {
                    print_help();
                }
                break;
            }
            case 1: { // LIST
                if(tokenCounter != 1){
                    printf("Usage: LIST\n");
                } else {
                    display_tables();
                }
                break;
            }
            case 2: { // CREATE
                if(tokenCounter != 2){
                    printf("Usage: CREATE <table>\n");
                } else {
                    char* table_name = tokenArr[1];
                    create_table(table_name);
                }
                break;
            }
            case 3: { // ADD
                if(tokenCounter != 4){
                    printf("Usage: ADD <table> <key> <value>\n");
                } else {
                    char* table_name = tokenArr[1];
                    char* key = tokenArr[2];
                    char* value = tokenArr[3];
                    ht_insert(table_name, key, value);
                }
                break;
            }
            case 4: { // COUNT
                if(tokenCounter != 2){
                    printf("Usage: COUNT <table>\n");
                } else {
                    char* table_name = tokenArr[1];
                    ht_count_entries(table_name);
                }
                break;
            }
            case 5: { // GET
                if(tokenCounter != 3){
                    printf("Usage: GET <table> <key>\n");
                } else {
                    char* table_name = tokenArr[1];
                    char* key = tokenArr[2];
                    ht_search(table_name, key);
                }
                break;
            }
            case 6: { // DELETE
                if(tokenCounter != 3){
                    printf("Usage: DELETE <table> <key>\n");
                } else {
                    char* table_name = tokenArr[1];
                    char* key = tokenArr[2];
                    ht_delete(table_name, key);
                }
                break;
            }
            case 7: { // DROP
                if(tokenCounter != 2){
                    printf("Usage: DROP <table>\n");
                } else {
                    char* table_name = tokenArr[1];
                    drop_table(table_name);
                }
                break;
            }
            case 8: { // EXIT
                if(tokenCounter != 1){
                    printf("Usage: EXIT\n");
                } else {
                    printf("Exiting...\n");
                    free(temp);
                    delete_collection();
                    exit(0);
                }
                break;
            }
        }
    }

    free(temp); 
}