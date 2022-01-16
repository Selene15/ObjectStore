/***
@file objectstore.c: Contiene l'implementazione delle funzioni della libreria di connessione al server
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <socket.h>
#include <sys/un.h>
#include <objectstore.h>
#include <macros.h>
#include <shared.h>


// File descriptor del client
static int objstore_fd = -1;


static int parse_error (char* response) {
    int errcode = -1;
    sscanf(response, "KO %d \n", &errcode);
    if (errcode <= 0) return 0;
    errno = errcode;
    return 1;
}


static int send_header (char* format, char* name, size_t length) {
    // Buffer che contiene l'header
    char header[HEADER_DIM];
    memset(header, 0, HEADER_DIM);
    // Distingue il tipo di parametri passati e costruisce la stringa
    if (length != 0) sprintf(header, format, name, length);
    else sprintf(header, format, name);
    // Invia l'header
    int result;
    SYSCALL((result=send_message(objstore_fd,header,sizeof(char)*HEADER_DIM)),"send_message");
  
    return result;
}

//funzione che connette il client al server e invio un messaggio di registrazione
int os_connect (char* name) {
 // Si collega al server socket
    SYSCALL((objstore_fd=create_socket(SOCKET_NAME)),"create_client_socket");
    //invio l'header
    int result = send_header("REGISTER %s \n", name, 0);
    if(result == -1) return 0;
    //Ricevo la risposta
    char* response;
    SYSCALL_ERR_NULL((response=receive_message(objstore_fd,sizeof(char)*RESPONSE_DIM)),free(response),"receive_message");
    //se l'operaizone è andata a buon fine libero la memoria e restituisco 1
    if(strcmp(response,"OK \n")==0){
        free(response);
        return 1;
    }
    //altrimenti libero la memoria e restituisco 0
    else{
        free(response);
        return 0;

    }
    
}

//funzione che invia un messaggio di richiesta di memorizzazione di un blocco dati
int os_store (char* name, void* block, size_t len) {
    if(name==NULL || block==NULL || len<=0){
        errno=EINVAL;
        return 0;
    }
    if(objstore_fd<=0){
        errno=ENOTCONN;
        return -1;
    }
    
    // Invio l'header
    int result;
    SYSCALL((result=send_header("STORE %s %ld \n", name,len)),"send_header");
    // Invio il blocco dati
    SYSCALL((result=send_message(objstore_fd,block,len)),"send_message");
   
    //Ricevo la risposta
    char* response;
    SYSCALL_ERR_NULL((response=receive_message(objstore_fd,sizeof(char)*RESPONSE_DIM)),free(response),"receive_message");
   
    //se l'operaizone è andata a buon fine libero la memoria e restituisco 1
    if(strcmp(response,"OK \n")==0){
        free(response);
        return 1;
    }
    //altrimenti libero la memoria e restituisco 0
    else{
        free(response);
        return 0;
    }
}

//funzione che richiede un blocco dati al sderver
void* os_retrieve (char* name) {
    ERRNO(name,"invalid name",EINVAL);
    
    //Invio l'header
    int result;
    SYSCALL((result=send_header("RETRIEVE %s \n",name,0)),"send_header");
   
    // Ricevo il messaggio
    char* res_header;
    res_header=receive_message(objstore_fd,sizeof(char)*DATA_DIM);
    if(res_header==NULL) return NULL;
    //Controlla che non sia stato restituito un errore
    int is_error = parse_error(res_header);
    if(is_error!=0){
        free(res_header);
            return NULL;
    }
    
    //Leggo la dimensione del blocco dati in arrivo
    size_t size = 0;
    sscanf(res_header, "DATA %zu \n", &size);
    free(res_header);
    //Se c'è stato un errore restiruisco NULL
    if(size<=0) return NULL;
    
    //Ricevo il blocco dati
    void* data;
    data=receive_message(objstore_fd,size);
    //Se c'è stato un errore restiruisco NULL
    if(data==NULL) return 0;
   
    return data;
}

//funzione che richiede l'eliminazione di un blocco dati al sderver
int os_delete (char* name) {
    //Invio l'header
    int result;
    SYSCALL((result=send_header("DELETE %s \n",name,0)),"send_header");
   
    //Ricevo la risposta
    char* response;
    response=receive_message(objstore_fd,sizeof(char)*RESPONSE_DIM);
    
    //se l'operaizone è andata a buon fine libero la memoria e restituisco 1
    if(strcmp(response,"OK \n")==0){
        free(response);
        return 1;
    }
    //altrimenti libero la memoria e restituisco 0
    else{
        free(response);
        return 0;
    }
}

//funzione che richiede la disconnessione dell'utente dal server
int os_disconnect() {
    //Invio l'header
    char leave_string[HEADER_DIM] = "LEAVE \n";
    int result = send_message(objstore_fd, leave_string, sizeof(char) * HEADER_DIM);
    if(result==-1) return 0;
    //Chiudo il socket
    SYSCALL((result=close(objstore_fd)),"close");
    return (result == 0);
}