/***
@file op_server.h
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/

#if !defined(_WORKERS)
#define _WORKERS


typedef struct client_struct {
   char *name;
   int fd;
} client_struct;

int hash_init();

int hash_stop();

void register_client (client_struct *new_client);


int store_block (char* cleint_name, char* name, void* data, size_t size);


void* retrieve_block (char *client_name, char* name, size_t* size_ptr);


int delete_block (char* client_name, char* name);


int leave_client (char* client_name);

int remove_directory(char *dirpath);

int get_report (int* clients_ptr, int* objects_ptr, int* size_ptr);

int create_directory(char* name);

#endif // _WORKERS
