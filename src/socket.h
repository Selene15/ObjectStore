/***
@file socket.h
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/


#if !defined(_SOCKET)
#define _SOCKET


int send_message (int file_descriptor, void* message, size_t size);


void* receive_message (int file_descriptor, size_t size);


int create_socket (char* socket_name);


int close_socket (int socket_fd);


int close_server_socket (int socket_fd, char* filename);


int select_client (int server_fd, fd_set set, struct timeval timeout);

#endif // _SOCKET
