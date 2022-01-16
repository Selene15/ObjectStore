/***
@file server.c: File principale del server
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/

#define _POSIX_C_SOURCE 199506L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <ftw.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <macros.h>
#include <shared.h>
#include <hash_table.h>
#include <socket.h>
#include <util.h>
#include <op_server.h>


static  size_t dim_data=0;
static  size_t file_stored=0; 

hash_table_t *client_reg=NULL;

void cleanup(){
    unlink(SOCKET_NAME);
}

// Variabile globale che indica la terminazione
static int OS_STOPPED = 0;


volatile int th_num = 0;
pthread_mutex_t th_num_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t th_num_cond = PTHREAD_COND_INITIALIZER;


//invio la stringa KO al client
void send_ko(int client_fd,const char *mess){
    char buff_err[RESPONSE_DIM];
    memset(buff_err,0,RESPONSE_DIM);
    ERRNO(buff_err,"malloc",ENOMEM);
    sprintf(buff_err,"KO %d \n",errno);

    //invio risposta sul socket
    int result=send_message(client_fd,buff_err,RESPONSE_DIM);
    if(result==-1){
        perror("Writing error message to client");
        return;
    }
    fprintf(stderr, "Client %d: %s\n",client_fd,strerror(errno) );    

}


//calcola i file memorizzati nella directory DATA e la sua dimensione 
int calc_stats(const char *fpath, const struct stat *s, int typeflag){
    if(typeflag==FTW_F){
        dim_data=dim_data+s->st_size;
        file_stored++;
    }
    return 0;
}

//stampa le statisctiche del sistema
void stats(){
    fprintf(stdout, "\n STATS: Clienti connessi: %d\n",th_num);
    ftw("./data",&calc_stats,64);
    fprintf(stdout,"STATS: Dimensione directory DATA: %ld bytes\n",dim_data);
    fprintf(stdout,"STATS: File memorizzati %ld\n", file_stored);
}

//invia la stinga OK al client
void send_ok (int client_fd) {
    char ok_string[RESPONSE_DIM] = "OK \n";
    int result;
    SYSCALL_ERR((result=send_message(client_fd,ok_string,RESPONSE_DIM)),send_ko(client_fd,"Error to send response"),"send_message");
    
}


client_struct *server_register(int client_fd){
    //ricevo l'header del messaggio inviato dal client
    char* buff=receive_message(client_fd,HEADER_DIM);
    
    char* saveptr;
    char* op=strtok_r(buff," ",&saveptr);
    char* name=strtok_r(NULL," ",&saveptr);

    //struttura del client
    client_struct* new_client=(client_struct*)calloc(1,sizeof(client_struct));
   
    new_client->name=NULL;
    new_client->fd=client_fd;
  
    //se l'operazione corrisponde alla register alloco memoria per il singolo client
    if(strcmp(op,"REGISTER")==0 && strlen(name)>0){
        new_client->name=(char*)malloc(sizeof(char)*strlen(name)+1);
        ERRNO(new_client->name,"malloc",ENOMEM);
        strcpy(new_client->name,name);
       
    }
    //controllo che l'utente non è gia regostrato nel sistema
    char* client_name=(char*)ht_find(client_reg,new_client->name);
        //utente già registrato
        if(client_name){
            send_ko(new_client->fd,"User già registrato");
            close(client_fd);
            free(new_client->name);
            free(new_client);
            return NULL;
        }
        //utente da registrare
        send_ok(new_client->fd);
        free(buff);
    return new_client;
           
}

void client_struct_cleanup_mem(client_struct *client) {
        free(client->name);
}


int server_delete (int client_fd, char* client_name, char* name) {
    // Rimuove il blocco dati dal sistema
    int result = delete_block(client_name, name);
    //se c'è stato qualche errore ritorno -1
    if(result!=0) return -1;
    //se non ci sono errori invio ok
    send_ok(client_fd);
    return 0;
}


int server_store (int client_fd,char* client_name, char* name, size_t length) {
    //ricevo i dati inviati dal client
    void* data;
    SYSCALL_NULL((data=receive_message(client_fd,length)),"receive_message");
  
    //memorizzo i dati
    int result;
    SYSCALL((result=store_block(client_name,name,data,length)),"store_block");
    
    //libero la memoria occupata dal blocco dati
    free(data);
    //se non ci sono errori invio ok
    send_ok(client_fd);
    return 0;
}


int server_retrieve (int client_fd, char* client_name, char* name) {
    char response[DATA_DIM];
    memset(response, 0, DATA_DIM);
    int result = 0;
    // Recupera il blocco
    size_t size;
    void* block = retrieve_block(client_name, name, &size);
   
    // Se c'è un errore memorizzo KO nella risposta
    if (block == NULL) {
        sprintf(response, "KO %d \n", errno);
        printf("Client %d: %s\n", client_fd, strerror(errno));
    }
    //costruisco la risposta
    else sprintf(response, "DATA %zu \n ", size);
    // invio la risposta al client
    SYSCALL((result=send_message(client_fd,response,sizeof(char)*DATA_DIM)),"send_message");
   
    //invio il blocco se diverso da NULL
    if (block) {
        SYSCALL((result=send_message(client_fd,block,size)),"send_message");
       
    }
    // libero la memoria occupata dal blocco
    free(block);
    return 0;
}


int server_leave (char* client_name) {
    //elimino l'utente dal sistema
    int result;
    SYSCALL((result=leave_client(client_name)),"leave_client");
    return 1;
}


void* signal_handler (void* ptr) {
    // insieme di segnali da attendere
    sigset_t set = *((sigset_t*) ptr);
   
    int sig;
    while (!OS_STOPPED) {
        // Attende un segnale
        SYSCALL_ERR(sigwait(&set,&sig),pthread_exit(NULL),"sigwait");
        switch(sig){
            case SIGTERM:
                fprintf(stderr, "Received SIGTERM\n");
                OS_STOPPED=1;
                break;
            case SIGINT:
                fprintf(stderr, "Received SIGINT\n");
                OS_STOPPED=1;
                break;
            case SIGQUIT:
                fprintf(stderr, "Received SIGQUIT\n");
                OS_STOPPED=1;
                break;
            case SIGUSR1:
                fprintf(stderr, "Received SIGUSR1\n");
                stats();
            default:
                break;
         }
        
    }
    
    return NULL;
}

//funzone per il parsing della richiesta
int parsing (int client_fd,char* client_name,char* header) {
    //operazione da effettuare
    char* op = (char*) calloc(9, sizeof(char));
    //nome del client
    char* name = (char*) calloc(255, sizeof(char));
    size_t length = 0;
    //estraggo le informazioni dall'header inviato dal client
    sscanf(header, "%s %s %zu \n", op, name, &length);
    
    int result;
     if(strcmp(op,"DELETE")==0)
        result = server_delete(client_fd, client_name,name);
    else if (strcmp(op, "STORE")==0)
        result = server_store(client_fd, client_name, name, length);
    else if (strcmp(op, "RETRIEVE")==0)
        result = server_retrieve(client_fd,client_name, name);
    else if (strcmp(op, "LEAVE")==0)
        result = server_leave(client_name);
    //se l'operazione non è nessuna delle precedenti restituisco -1
    else result = -1;
    
    free(op); free(name);
    return result;
}


//funzione utilizzata dai thread 
void* connection_handler (void* ptr) {

    //incremento il numero dei thread attivi
    pthread_mutex_lock(&th_num_mtx);

        th_num++;   

    pthread_mutex_unlock(&th_num_mtx);
    
    client_struct *client=(client_struct*)ptr;
    int client_fd = client->fd;
    char* client_name=client->name;

    while (!OS_STOPPED) {
        //Header inviato dal client
        char* header = receive_message(client_fd, sizeof(char) * HEADER_DIM);
        if (!header) break;
       
        printf("Operation: %s", header);
        //parsing della richiesta
        int res = parsing(client_fd,client_name,header);
        // libero la memoria occupata dall'header
        free(header);
        // Se la richiesta non è andata a buon fine invia un messaggio di KO al client
        if(res==-1){
            send_ko(client_fd,"Request not valid");

        } 
        
        //se result è uguale a 1, ho ricevuto un messaggio di terminazione 
        if (res == 1) break;
    }
    //libero dalla memoria la struttura del client
    client_struct_cleanup_mem(client);

    // Libera la memoria occupata dal file descriptor
    free(ptr);
    //chiudo il socket del client
    SYSCALL(close(client_fd),"close");
    
    // Stampa un messaggio di uscita
    printf("Connection terminated with client: %d\n", client_fd);

    //decremento il numero dei thread attivi
    pthread_mutex_lock(&th_num_mtx);
    th_num--;
    if (th_num <= 0) pthread_cond_signal(&th_num_cond);
    pthread_mutex_unlock(&th_num_mtx);
    

    pthread_detach(pthread_self());
    return NULL;
}

int main(int argc, char const *argv[]) {

    //elimino un eventuale file con lo stesso nome del socket da creare che potrebbe essere già presente 
    cleanup();

    // Crea la maschera dei segnali
    sigset_t set;
    SYSCALL(sigemptyset(&set),"sigemptyset");
    SYSCALL(sigaddset(&set, SIGINT),"Add SIGINT");
    SYSCALL(sigaddset(&set, SIGPIPE),"Add SIGPIPE");
    SYSCALL(sigaddset(&set, SIGQUIT),"Add SIGQUIT");
    SYSCALL(sigaddset(&set, SIGTERM),"Add SIGTERM");
    SYSCALL(sigaddset(&set, SIGUSR1),"Add SIGUSR1");
    
    //blocco tutti i segnali
    if(pthread_sigmask(SIG_SETMASK,&set,NULL)!=0){
        perror("pthread_sigmask");
        exit(1);
    }

    //creo il thread che gestisce i segnali
    pthread_t sig_handler_id;
    SYSCALL_PTHREAD(pthread_create(&sig_handler_id,NULL,signal_handler,(void*)&set),"Creazione thread Signal Handler");
   
    //creazione server socket
    int server_fd;
    SYSCALL((server_fd=socket(AF_UNIX,SOCK_STREAM,0)),"socket");
    struct sockaddr_un socket_address;
    strncpy(socket_address.sun_path, SOCKET_NAME, UNIX_PATH_MAX);
    socket_address.sun_family = AF_UNIX;
    SYSCALL((bind(server_fd,(struct sockaddr*)&socket_address,sizeof(socket_address))),"bind");
    SYSCALL((listen(server_fd,SOMAXCONN)),"listen");
   
    
    //creazione della directory utilizzata per contenere tutte le directory relative ai client
    int success;
    SYSCALL((success=create_directory("data")),"create_directory");
    //creazione tabella hash utilizzata per memorizzare i client registrati
    SYSCALL((success=hash_init()),"hash_init");
    
    //insime dei file descriptor attivi
    fd_set fset;
    FD_ZERO(&fset);
    FD_SET(server_fd, &fset);
    //timeout contiene il valore del timer per la select
    struct timeval timeout = {1, 0};
    printf("Server online and waiting for connections with client\n");
    printf("SOCKET NAME %s\n", SOCKET_NAME);
    
    while (!OS_STOPPED) {
        
        int client_fd;
        SYSCALL((client_fd=select_client(server_fd,fset,timeout)),"select_client");
        //nuova connessione
        if (client_fd > 0) {
            //creazione struttura client
            client_struct *new_client=server_register(client_fd);
            if(new_client!=NULL){
                register_client(new_client);
                
                //creo un thread che gestisce il singolo client in modalità detached
                 pthread_t th_id;
                 pthread_attr_t thattr;
                 if(pthread_attr_init(&thattr)!=0){
                    fprintf(stderr, "pthread_attr_init FALLITA\n");
                    close(client_fd);
                    break;
                 }
                 if(pthread_attr_setdetachstate(&thattr,PTHREAD_CREATE_DETACHED)!=0){
                    fprintf(stderr, "pthread_attr_setdetachstate FALLITA\n");
                    pthread_attr_destroy(&thattr);
                    close(client_fd);
                    break;
                 }

                 if(pthread_create(&th_id,&thattr,connection_handler,(void*)new_client)!=0){
                    fprintf(stderr, "pthread_create FALLITA\n");
                    pthread_attr_destroy(&thattr);
                    close(client_fd) ;
                    break;
                 }
                 
            }
            
        }
            
           
            
        
    }
    
    //attendo la terminazione di tutti i thread
    pthread_mutex_lock(&th_num_mtx);

    while (th_num > 0) 
            pthread_cond_wait(&th_num_cond, &th_num_mtx);

    pthread_mutex_unlock(&th_num_mtx);

    //libero la tabella hash dei clienti registarti dalla memoria
    SYSCALL(hash_stop(),"hash_Stop");

    SYSCALL_PTHREAD(pthread_join(sig_handler_id,NULL),"pthread_join");
    
    //chiudo il socket
    SYSCALL(close(server_fd),"close_socket");
    
    //elimino la directory DATA
    SYSCALL(remove_directory("data"),"remove_directory")
   
    
    printf("Server offline\n");
    return 0;

}
