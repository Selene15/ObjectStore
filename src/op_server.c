/***
@file op_server.c: Contiene l'implementazione delle funzioni che si interfacciano con il file system e la tabella hash
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ftw.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <macros.h>
#include <shared.h>
#include <util.h>
#include <hash_table.h>
#include <op_server.h>


// Tabella hash in cui memorizzo le coppie (username, file descriptor)
hash_table_t *client_reg;

//Restituisce la dimensione del file
static size_t get_file_size (char* file_name) {
    struct stat sb = {0};
    int result = stat(file_name, &sb);
    if(result==-1){
        errno=ENOENT;
        return -1;
    }
    return sb.st_size;
}

//crea la directory
 int create_directory(char* name) {
    struct stat sb = {0};
    if (!(stat(name, &sb) == 0 && S_ISDIR(sb.st_mode))) {
        return mkdir(name, 0777);
    }

    return 0; 
}



//Crea un path: DATA_DIRECTORY/directory/[filename]
static char* create_path (char* directory, char* filename) {
    size_t length = strlen(DATA_DIRECTORY) + strlen(directory) + 2;
    if (filename){
        
        length += strlen(filename) + 1;
    } 
    char* path;
    //alloca memoria per il path 
    SYSCALL_NULL((path=(char*)malloc(sizeof(char)*length)),"malloc");
    memset(path,0,sizeof(char)*length);

    // Costruisce il path
    if (filename)
        sprintf(path, "%s/%s/%s", DATA_DIRECTORY, directory, filename);
    else
        sprintf(path, "%s/%s", DATA_DIRECTORY, directory);
    return path;
}

//inizializza la struttura dati "client_reg"
int hash_init() {
    
    SYSCALL_NULL((client_reg=ht_create(HASHTABLEDIM)),"create hashtable");
    return 0;
}

//Libero la memoria occupata dalla struttura dati
int hash_stop () {
    //Elimino la tabella hash
    ht_destroy(client_reg);
    return 0;
}

//Registra un nuovo utente
void register_client(client_struct *new_client){

    char* path;
    SYSCALL_NULL((path=create_path(new_client->name,NULL)),"create_path");
    //creo la cartella relativa all'utente sul disco
    int result;
    SYSCALL((result=create_directory(path)),"create_directory");
    free(path);
   //Inserisco l'utente nella struttura dati
    SYSCALL((result=ht_insert(client_reg,new_client->name,new_client->fd)),"insert hashtable");
      
}





//Memorizzo un blocco dati nel file 
int store_block (char* client_name, char* name, void* data, size_t size) {
    //Controllo se l'utente è presente nel sistema
    char* client;
    SYSCALL_NULL((client=ht_find(client_reg,client_name)),"find hashtable");
    
    //Creo il percorso del file
    char* path;
    SYSCALL_NULL((path=create_path(client_name,name)),"create_path");
    
    //Apro il file
    int file_fd;
    SYSCALL((file_fd=open(path,O_CREAT|O_WRONLY,0777)),"open");
    //Libero la memoria occpuata dal path
    free(path);
    //Scrivo tutti i bytes sul file aperto
    int bytes_written;
    SYSCALL((bytes_written=writen(file_fd,data,size)),"writen");
    //Chiudo il file
    int result;
    SYSCALL((result=close(file_fd)),"close");
   
    return 0;
}

//Recupero un blocco dati dal sistema
void* retrieve_block (char* client_name, char* name, size_t* size_ptr) {
    ERRNO(client_name,"parametri non validi", EINVAL);
    ERRNO(name,"parametri non validi", EINVAL);

    char* client;
    //Controllo che l'uetnet è presente nel sistema
    client=ht_find(client_reg,client_name);
    //se non lo trovo restituisco NULL
    if(client==NULL) return NULL;
    // Costruisce il percorso del file
    char* path;
    SYSCALL_NULL((path=create_path(client_name,name)),"create_path");
    //Recupero la dimensione del file
    size_t size = get_file_size(path);
   
    if(size==-1){
        free(path);
        return NULL;
    }
    //Alloco memoria per un buffer che conterrà il blocco dati
    void* buffer = malloc(size);
    ERRNO(buffer,"malloc",ENOMEM);
    //Apro il file
    int file_fd;
    SYSCALL((file_fd=open(path, O_RDONLY)),"open");
    free(path);
    //Leggo il contenuto e lo memorizzo nel buffer
    int bytes_read;
    bytes_read=readn(file_fd,buffer,size);

    //chiudo il file
    int result;
    SYSCALL((result=close(file_fd)),"close");

    if(bytes_read==-1 && result==-1){
        free(buffer);
        return NULL;
    }
    *size_ptr = size;
    return buffer;
}

//Rimuovo un blocco dati dell'utente dal disco
int delete_block (char* client_name, char* name) {
    //Controllo che l'uetnet è presente nel sistema
    char* client;
    SYSCALL_NULL((client=ht_find(client_reg,client_name)),"find hashtable");
    //Costruisco il path del file
    char* path;
    SYSCALL_NULL((path=create_path(client_name,name)),"create_path");
    //Rimuovo il file
    int result = unlink(path);
    //Libero la memoria occupata dal path
    free(path);
    return result;
}

//Rimuovo l'utente dal sistema
int leave_client (char* client_name) {
    //Rimuovo l'utente dalla struttura dati
    int result;
    SYSCALL_HT((result=ht_remove(client_reg,client_name)),"delete hashtable");
    
    return 0;
}

//Eliminazione della directory dirpath e del suo contenuto
int remove_directory(char *dirpath){
        DIR *d=opendir(dirpath);
        struct stat sb;

        if(d==NULL)
            return -1;
        struct dirent *file;
        while((file=readdir(d))!=NULL){
            errno=0;
            if(strcmp(file->d_name, ".")!=0 && strcmp(file->d_name,"..")!=0){
                size_t dimpath=strlen(file->d_name)+strlen(dirpath)+2;
                char *pathfile=(char*)malloc(dimpath*sizeof(char));
                if(pathfile==NULL)
                    return -1;
                memset(pathfile,0,dimpath*sizeof(char));
                //creo il path che utilizzerò per l'eliminazione effettiva
                sprintf(pathfile,"%s/%s",dirpath,file->d_name);
                if ((stat(pathfile, &sb) == 0 && S_ISDIR(sb.st_mode)))

                    remove_directory(pathfile);

                remove(pathfile);
                free(pathfile);
            }
        }

        closedir(d);
        rmdir(dirpath);
       
        
        return 0;
    }


