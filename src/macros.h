/***
@file macros.h: Contiene le macro utilizzate
@author: Selene Gerali 546091
@project: Sistemi Operativi e Laboratorio
*/

#if !defined(_ASSERT)

#include <errno.h>

#define SYSCALL_HT(fun,err_msg) \
	if(fun==0){perror(err_msg); exit(EXIT_FAILURE);}

#define SYSCALL(fun, err_msg) \
    if (fun == -1){ perror(err_msg); exit(EXIT_FAILURE); }

#define SYSCALL_NULL(fun, err_msg) \
    if(fun == NULL){ perror(err_msg); exit(EXIT_FAILURE);}

#define ERRNO(el,err_msg,err_value)\
    if(el==NULL){ perror(err_msg); errno=err_value; exit(EXIT_FAILURE);}

#define ERRNO_1(el,err_msg,err_value) \
    if(el==-1){ perror(err_msg); errno=err_value; exit(EXIT_FAILURE);}

#define SYSCALL_ERR(fun1,fun2,err_msg) \
    if(fun1==-1){ perror(err_msg); fun2; exit(EXIT_FAILURE);}

#define SYSCALL_ERR_NULL(fun1,fun2,err_msg) \
    if(fun1==NULL){ perror(err_msg); fun2; exit(EXIT_FAILURE);}

#define SYSCALL_ZERO(fun,err_msg) \
    if(fun!=0) { perror(err_msg); exit(EXIT_FAILURE);}

#define SYSCALL_PTHREAD(fun,err_msg) \
    if(fun!=0) { perror(err_msg); exit(1);}
#endif // _ASSERT
