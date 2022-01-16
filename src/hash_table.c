
/***
@file hash_table.c: Contiene l'implementazione della tabella hash
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/



#include <hash_table.h>

//Lock per ogni indice della tabella hash
static pthread_mutex_t* mtxs;
// Lock per la tabella hash
static pthread_mutex_t ht_mtx = PTHREAD_MUTEX_INITIALIZER;

//Creazione tabella hash
hash_table_t* ht_create(long size){
    if(size < 1){
        errno = EINVAL;
        return NULL;
    }

    hash_table_t* hash_table = NULL;

    //Alloco la tabella
    if((hash_table = (hash_table_t*)malloc(sizeof(hash_table_t))) == NULL)
        return NULL;

    if((hash_table->hash_table = (hash_entry_t**)malloc(sizeof(hash_entry_t*) * size)) == NULL){
        free(hash_table);
        return NULL;
    }

    for(int i = 0; i < size; i++)
        hash_table->hash_table[i] = NULL;

    hash_table->size = size;

    if((mtxs = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * size)) == NULL){
        free(hash_table->hash_table);
        free(hash_table);
        return NULL;
    }
    for(int i = 0; i < size; i++)
        pthread_mutex_init(&mtxs[i], NULL);

    return hash_table;
}

//Funzione hash Java
long hash(hash_table_t* hash_table, const char* key){
    unsigned long hash = 0;

    for(int i = 0; key[i] != '\0'; i++)
        hash = 31 * hash + key[i];

    return (hash % hash_table->size);
}

//Inserimento di un elemento in tabell hash
int ht_insert(hash_table_t* hash_table, char* key, int value){
    if(hash_table == NULL || key == NULL){
        errno = EINVAL;
        return -1;
    }

    //Ottengo l'indice dove il nuovo elemento dovrà essere inserito
    long index = hash(hash_table, key);

   SYSCALL_ZERO(pthread_mutex_lock(&mtxs[index]), "pthread_mutex_lock call");

    //controllo se la chiave già esiste
    hash_entry_t* cur = hash_table->hash_table[index];
    while(cur != NULL){
        if(strcmp(cur->key, key) == 0){
            SYSCALL_ZERO(pthread_mutex_unlock(&mtxs[index]), "pthread_mutex_unlock call");
            return 0;
        }
        cur = cur->next;
    }

    hash_entry_t* new_entry = NULL;
    if((new_entry = (hash_entry_t*)malloc(sizeof(hash_entry_t))) == NULL){
        SYSCALL_ZERO(pthread_mutex_unlock(&mtxs[index]), "pthread_mutex_unlock call");
        return -1;
    }

    // inizializzo il nuovo elemento
    if((new_entry->key = (char*)calloc(strlen(key) + 1, sizeof(char))) == NULL){
        SYSCALL_ZERO(pthread_mutex_unlock(&mtxs[index]), "pthread_mutex_unlock call");
        free(new_entry);
        return -1;
    }
    strncpy(new_entry->key, key, strlen(key));
    new_entry->value = value;
    new_entry->next = NULL;
    
    new_entry->next = hash_table->hash_table[index];
    hash_table->hash_table[index] = new_entry;

    SYSCALL_ZERO(pthread_mutex_unlock(&mtxs[index]),"pthread_mutex_unlock call");

    return 1;
}

//Ricerca di un elemento in tabella hash
char* ht_find(hash_table_t* hash_table, char* key){
    if(hash_table == NULL || key == NULL){
        errno = EINVAL;
        return NULL;
    }

    //Ottengo l'indice hash
    long index = hash(hash_table, key);

    SYSCALL_ZERO(pthread_mutex_lock(&mtxs[index]), "pthread_mutex_lock call");

    hash_entry_t* cur = hash_table->hash_table[index];
    
    while(cur != NULL){
        if(strcmp(cur->key, key) == 0){
          SYSCALL_ZERO(pthread_mutex_unlock(&mtxs[index]), "pthread_mutex_unlock call");
            return cur->key;
        }
        cur = cur->next;
    }

    SYSCALL_ZERO(pthread_mutex_unlock(&mtxs[index]), "pthread_mutex_unlock call");

    //chiave non trovaŧa
    return NULL;
}

//Rimozione di un elemento in tabella hash
int ht_remove(hash_table_t* hash_table, char* key){
    if(hash_table == NULL || key == NULL){
        errno = EINVAL;
        return -1;
    }

    //ottengo l'indice hash
    long index = hash(hash_table, key); 

    SYSCALL_ZERO(pthread_mutex_lock(&mtxs[index]), "pthread_mutex_lock call");

    hash_entry_t* cur = hash_table->hash_table[index];
    hash_entry_t* prev = NULL;

    while((strcmp(cur->key, key) != 0) && cur->next != NULL){
        prev = cur;
        cur = cur->next;
    }

    //chiave trovata
    if(strcmp(cur->key, key) == 0){
        if(prev)
            prev->next = cur->next;
        else
            hash_table->hash_table[index] = cur->next;
        free(cur->key);
        free(cur);

        SYSCALL_ZERO(pthread_mutex_unlock(&mtxs[index]), "pthread_mutex_unlock call");

        return 1;
    }

    SYSCALL_ZERO(pthread_mutex_unlock(&mtxs[index]), "pthread_mutex_unlock call");
    
    //chiave non trovata
    return 0;
}

//Eliminazione della tabella hash
int ht_destroy(hash_table_t* hash_table){
    if(hash_table == NULL){
        errno = EINVAL;
        return 0;
    }

    SYSCALL_ZERO(pthread_mutex_lock(&ht_mtx), "pthread_mutex_lock call");

    
    for(int i = 0; i < hash_table->size; i++){
        hash_entry_t* cur = NULL;
        while((cur = hash_table->hash_table[i]) != NULL){
            hash_table->hash_table[i] = hash_table->hash_table[i]->next;
            free(cur->key);
            free(cur); 
        }
    }

    
    if(hash_table->hash_table) 
        free(hash_table->hash_table);
    
    if(hash_table) 
        free(hash_table);
    
    if(mtxs) 
        free(mtxs);

    SYSCALL_ZERO(pthread_mutex_unlock(&ht_mtx), "pthread_mutex_unlock call");

    return 1;
}



