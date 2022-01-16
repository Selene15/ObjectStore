/***
@file util.h
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/


#if !defined(_UTIL)
#define _UTIL


size_t readn (int file_descriptor, void* buffer, size_t n);


size_t writen (int file_descriptor, const void *buffer, size_t n);

#endif // _SAFE_IO
