/***
@file shared.h
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/
#if !defined(_SHARED)
#define _SHARED

// Nome del socket condiviso tra server e client
#define SOCKET_NAME "./objstore.sock"

// Nome della cartella dati
#define DATA_DIRECTORY "./data"

//Dimensione massima di un header che contiene un comando dato da verbo di lunghezza massima ("RETRIEVE") + massima dimensione di un nome di file POSIX (255) + due spazi + \n + \0
#define HEADER_DIM 267

//Dimensione massima di una risposta di tipo diverso dai dati, costituito da "KO <er> \n"
#define RESPONSE_DIM 8

//Dimensione massima della stringa "DATA <length> \n ", con la lunghezza massima di length data da 2^64 (20 cifre)
#define DATA_DIM 29

//Dimensione massima del path
#define UNIX_PATH_MAX 108

//Dimensione della tabella hash
#define HASHTABLEDIM 2048
#endif // _SHARED