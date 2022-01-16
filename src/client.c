/***
@file client.c: File principale del client
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <macros.h>
#include <socket.h>
#include <objectstore.h>
#include <shared.h>

//creazione array
static char* create_array (size_t size) {
    char* array = (char*) malloc(size);
    memset(array,0,size);
    ERRNO(array,"malloc",ENOMEM);
    for (int i = 0; i < 100000; i++) array[i] = (char) i;
    return array;
}

//Test 1: Store
static int store_data () {
    
    char name[2] = "A";
    // Buffer da 100KB
    char* array;
    SYSCALL_NULL((array=create_array(100000)),"create_array");
    
    // Invia 20 messaggi con dimensione da 100 a 100000
    int size = 100;
    int step = (100000 - 100) / 20;
    for (int i = 0; i < 19; i++) {
        if(os_store(name,array,size)==0){
            free(array);
            return errno;
        }
        size += step;
        name[0]++;
    }
    if(os_store(name,array,size)==0){
            free(array);
            return errno;
        }
    
    // Libera la memoria occupata dall'array
    free(array);

    return 0;
}


//Test 2: Retrieve
static int retrieve_data () {
    char* array = create_array(100000);
    // Step con cui aumenta la dimensione dei dati
    int step = (100000 - 100) / 20;
    char name[2] = "A";
    int size = 100;

    for (int i = 0; i < 19; i++) {
        char* data = os_retrieve(name);
        
        if(data==NULL){
            free(data);
            free(array);
            return 1;

        }
        for (int i = 0; i < size; i++)
        if (data[i] != array[i]){
            free(array);
            free(data);
            return -1;
        } 

        free(data);
       
        size += step;
        // Cambia il nome del prossimo blocco
        name[0]++;
    }
    free(array);
    
    return 0;
}

//Test 3:Delete
static int delete_data () {
    // Nome del blocco
    char name[2] = "A";
    for (int i = 0; i < 20; i++) {
        if(os_delete(name)!=1){
            return 1;
        }

        // Incrementa il nome
        name[0]++;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    // Controlla che sia stato passato il corretto numero di argomenti
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <name_client> <test>\n", argv[0]);
        exit(1);
    }
    // Nome del client
    char* name = strdup(argv[1]);
    // Numero di test da effettuare
    int test = strtol(argv[2], NULL, 10);
    if ((test < 1) || (test > 3)) {
        fprintf(stderr, "Test number must be between 1 and 3\n");
        exit(1);
    }
    // Si connette al server
    if(os_connect(name) != 1){
        fprintf(stderr, "[%s] Connecting to client: %s\n", name, strerror(errno)); 
        free(name); 
        return 1;
    }
    int error = 0;
	switch(test){
        case 1:
            error=store_data();
            break;
        case 2:
            error=retrieve_data();
            break;
        case 3:
            error=delete_data();
            break;
    }
   
    if (!error){
        printf("***%s connected***\n",name);

        printf("Test %d: Success\n", test);
    }
    else fprintf(stderr, "[%s] Test %d: %s\n", name, test, strerror(error));
    // Libera la memoria occupata dal nome
    free(name);
    // Si disconnette
    if(os_disconnect()!=1)
        return errno;
    return error;
}
