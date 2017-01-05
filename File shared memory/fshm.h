#ifndef _FSHM_H_
#define _FSHM_H_

#include <stddef.h>

extern int fshm_file_create_mode;

typedef void (*FSHMAccFunc)(void* mem,size_t len,void* data);

int fshm_do_access(const char* path,size_t len,FSHMAccFunc callback,void* data);
int fshm_try_access(const char* path,size_t len,FSHMAccFunc callback,void* data);

#endif /* _FSHM_H_ */
