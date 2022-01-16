/***
@file socket.c: Contiene l'implemetazione delle funzioni per la comunicazione tramite socket
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/un.h>
#include <shared.h>
#include <util.h>
#include <socket.h>
#include <macros.h>



//invio di un messaggio sul socket
int send_message (int fd, void* message, size_t size) {
    if(message==NULL || size<0){
    	errno=EINVAL;
    	return -1;
    }
    // Scrive il messaggio
    int bytes_written;
	SYSCALL((bytes_written=writen(fd,message,size)),"writen");
    return 0;
}

//lettura di un messaggio dal socket
void* receive_message (int file_descriptor, size_t size) {
	if(file_descriptor<=0 || size<0){
		errno=EINVAL;
		return NULL;
	}

    //Alloco il buffer per contenere il messaggio
	void* buffer = malloc(size);
	memset(buffer, 0, size);
	// Controlla che l'allocazione sia avvenuta con successo
	ERRNO(buffer,"malloc",ENOMEM;)

	//Leggo i dati
	int bytes_read;
	SYSCALL((bytes_read=readn(file_descriptor,buffer,size)),"readn");
	if(bytes_read<=0){
		free(buffer);
		return NULL;
	}
	return buffer;
}

//creazione socket
int create_socket (char* socket_name) {
	int client_fd;
	SYSCALL((client_fd=socket(AF_UNIX,SOCK_STREAM,0)),"socket");
	struct sockaddr_un socket_address;
	strncpy(socket_address.sun_path, socket_name, UNIX_PATH_MAX);
	socket_address.sun_family = AF_UNIX;
	SYSCALL((connect(client_fd,(struct sockaddr*)&socket_address,sizeof(socket_address))),"connect");
	
	return client_fd;
}


//funzione utilizzata per l'ascolto di nuove connessioni
int select_client (int server_fd, fd_set set, struct timeval timeout) {
	// Set di appoggio
	fd_set ready_set = set;
	int result;
	SYSCALL((result=select(server_fd+1,&ready_set,NULL,NULL,&timeout)),"select");
	
	// Se c'è un errore o non ci sono file descriptor attivi esce
	if (result <= 0) return result;
	// se il file descriptor del server è pronto
	if (FD_ISSET(server_fd, &ready_set)) {
		int client_fd;
		//accetto una nuova connessione
		SYSCALL((client_fd=accept(server_fd,NULL,0)),"accept");
		return client_fd;
	}
	return -1;
}