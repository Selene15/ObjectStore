/***
@file util.h: Contiene le operazioni di readn e writen
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/


#include <unistd.h>
#include <errno.h>

#include <macros.h>

#include <util.h>


size_t readn (int file_descriptor, void* buffer, size_t n) {
	if(file_descriptor<=0 || buffer==NULL || n<=0){
		errno=EINVAL;
		return -1;
	}
	// Numero di bytes rimasti
	size_t nleft = n;
	// Numero di bytes letti ad ogni iterazione
	size_t nread;
	char* ptr = buffer;
	// Continua a scorrere finché non ha letto tutti i bytes
	while (nleft > 0) {
		nread = read(file_descriptor, ptr, nleft);
		if (nread < 0) {
			if (errno == EINTR) nread = 0;
			else return (-1);
		}
		else if (nread == 0) break;
		nleft -= nread;
		ptr += nread;
	}
	// Restituisce il numero di bytes letti
	return (n - nleft);
}


size_t writen (int file_descriptor, const void *buffer, size_t n) {
	if(file_descriptor<=0 || n<=0){
		errno=EINVAL;
		return -1;
	}
	size_t nleft = n;
	size_t nwritten;
	const char* ptr = buffer;
	// Continua finché non ha scritto tutti i bytes
	while (nleft > 0) {
		nwritten = write(file_descriptor, ptr, nleft);
		if (nwritten <= 0) {
			if (nwritten < 0 && errno == EINTR) nwritten = 0;
			else return (-1);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	// Restituisce il numero di bytes scritti
	return n;
}