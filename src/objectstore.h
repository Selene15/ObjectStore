/***
@file objectstore.h
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/

#if !defined(_CLIENT)
#define _CLIENT


int os_connect (char* name);


int os_store (char* name, void* block, size_t len);


void* os_retrieve (char* name);


int os_delete (char* name);


int os_disconnect();

#endif // _CLIENT
