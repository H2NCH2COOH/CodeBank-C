#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include "fshm.h"

int fshm_file_create_mode=S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP;

static int fshm_transaction(int non_block,const char* path,size_t len,FSHMAccFunc callback,void* data)
{
    int fd;
    int ret;
    int err;
    void* mem;

    err=errno;

    if(
        path==NULL||
        len==0||
        callback==NULL
    )
    {
        errno=EINVAL;
        return -1;
    }

    fd=open(path,O_RDWR|O_CREAT,fshm_file_create_mode);
    if(fd<0)
    {
        return -1;
    }

    ret=lockf(fd,(non_block)? F_TLOCK:F_LOCK,0);
    if(ret!=0)
    {
        err=errno;
        close(fd);
        errno=err;
        return -1;
    }

    ret=ftruncate(fd,len);
    if(ret!=0)
    {
        err=errno;
        goto error;
    }

    mem=mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(mem==MAP_FAILED)
    {
        err=errno;
        goto error;
    }

    callback(mem,len,data);

    munmap(mem,len);
error:
    lockf(fd,F_ULOCK,0);
    close(fd);

    errno=err;
    return 0;
}

int fshm_do_access(const char* path,size_t len,FSHMAccFunc callback,void* data)
{
    return fshm_transaction(0,path,len,callback,data);
}

int fshm_try_access(const char* path,size_t len,FSHMAccFunc callback,void* data)
{
    return fshm_transaction(1,path,len,callback,data);
}
