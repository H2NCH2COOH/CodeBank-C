#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "stream.h"

#ifndef container_of
#define container_of(ptr, type, member) ({ \
                const typeof( ((type *)0)->member ) *__mptr = (ptr); \
                (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

Stream* new_stream(size_t size)
{
    Stream* s;
    
    s=malloc(sizeof(Stream));
    if(s==NULL)        
    {
        errno=ENOMEM;
        return NULL;
    }
    
    s->buff=malloc(size);
    if(s->buff==NULL)
    {
        free(s);
        errno=ENOMEM;
        return NULL;
    }
    
    s->size=size;
    s->len=0;
    s->idx=0;
    
    s->input_fd=NULL;
    s->output_fd=NULL;
    
    return s;
}

size_t stream_pending_len(Stream* stream)
{
    return stream->len;
}

int stream_write(Stream* stream,const char* in,size_t len)
{
    size_t size,i;
    char* new_buff;
    
    if(stream->size<stream->len+len)
    {
        //Need expand
        for(size=stream->size;size<stream->len+len;size*=2);
        
        new_buff=malloc(size);
        if(new_buff==NULL)
        {
            errno=ENOMEM;
            return -1;
        }
        
        for(i=0;i<stream->len;++i)
        {
            new_buff[i]=stream->buff[(stream->idx+i)%stream->size];
        }
        stream->buff=new_buff;
        stream->idx=0;
        stream->size=size;
    }
    
    for(i=0;i<len;++i)
    {
        stream->buff[(stream->idx+stream->len+i)%stream->size]=in[i];
    }
    stream->len+=len;
    
    return 0;
}

size_t stream_read(Stream* stream,char* out,size_t len)
{
    size_t i;
    
    len=(len>stream->len)? stream->len:len;
    
    for(i=0;i<len;++i)
    {
        out[i]=stream->buff[(stream->idx+i)%stream->size];
    }
    
    stream->idx=(stream->idx+len)%stream->size;
    stream->len-=len;
    
    return len;
}

size_t stream_peek(Stream* stream,char* out,size_t len)
{
    size_t i;
    
    len=(len>stream->len)? stream->len:len;
    
    for(i=0;i<len;++i)
    {
        out[i]=stream->buff[(stream->idx+i)%stream->size];
    }
    
    return len;
}

size_t stream_skip(Stream* stream,size_t len)
{
    len=(len>stream->len)? stream->len:len;
    
    stream->idx=(stream->idx+len)%stream->size;
    stream->len-=len;
    
    return len;
}

static void stream_input_cb(struct ev_loop* loop,ev_io* w,int revents)
{
    struct stream_input_fd* in;
    Stream* stream;
    int ret;
    char buff[1024];
    
    (void)revents;
    
    in=container_of(w,struct stream_input_fd,io);
    stream=in->stream;
    
    if(in->input_cb)
    {
        in->input_cb(stream,loop,in->fd);
    }
    else
    {
        ret=1;
        while(ret>0)
        {
            errno=0;
            ret=read(in->fd,buff,sizeof(buff));
            if(ret<0)
            {
                //TODO: log
                return;
            }
            else if(ret==0)
            {
                //Peer closed
                if(in->close_cb)
                {
                    in->close_cb(stream,loop,in->fd);
                }
                else
                {
                    ev_io_stop(loop,&in->io);
                    close(in->fd);
                    in->fd=-1;
                }
                return;
            }
            else
            {
                if(stream_write(stream,buff,ret)<0)
                {
                    //No mem
                    return;
                }
            }
        }
    }
}

int stream_set_input_fd(Stream* stream,int fd,stream_fd_cb input_cb,stream_fd_cb close_cb)
{
    struct stream_input_fd* in;
    
    in=malloc(sizeof(struct stream_input_fd));
    if(in==NULL)
    {
        errno=ENOMEM;
        return -1;
    }
    
    in->stream=stream;
    in->fd=fd;
    in->input_cb=input_cb;
    in->close_cb=close_cb;
    
    ev_io_init(&in->io,stream_input_cb,fd,EV_READ);
    
    stream->input_fd=in;
    
    return 0;
}

void stream_start_input_fd(Stream* stream,struct ev_loop* loop)
{
    if(stream->input_fd)
    {
        ev_io_start(loop,&stream->input_fd->io);
    }
}

void stream_stop_input_fd(Stream* stream,struct ev_loop* loop)
{
    if(stream->input_fd)
    {
        ev_io_stop(loop,&stream->input_fd->io);
    }
}

static void stream_output_cb(struct ev_loop* loop,ev_io* w,int revents)
{
    Stream* stream;
    struct stream_output_fd* out;
    int ret;
    char* buff;
    size_t i;
    int errno_save;
    
    (void)revents;
    
    out=container_of(w,struct stream_output_fd,io);
    stream=out->stream;
    
    if(out->output_cb)
    {
        out->output_cb(stream,loop,out->fd);
    }
    else
    {
        buff=malloc(stream->len);
        if(buff==NULL)
        {
            //TODO: log
            return;
        }
        
        for(i=0;i<stream->len;++i)
        {
            buff[i]=stream->buff[(stream->idx+i)%stream->size];
        }
        
        errno=0;
        ret=write(out->fd,buff,stream->len);
        if(ret<0)
        {
            errno_save=errno;
            if(errno_save!=EAGAIN&&errno_save!=EWOULDBLOCK)
            {
                if(out->close_cb)
                {
                    errno=errno_save;
                    out->close_cb(stream,loop,out->fd);
                }
                else
                {
                    ev_io_stop(loop,&out->io);
                    close(out->fd);
                    out->fd=-1;
                }
            }
            free(buff);
            return;
        }
        else
        {
            free(buff);
            stream->idx=(stream->idx+ret)%stream->size;
            stream->len-=ret;
            
            if(stream->len==0)
            {
                ev_io_stop(loop,&out->io);
            }
        }
    }
}

int stream_set_output_fd(Stream* stream,int fd,stream_fd_cb output_cb,stream_fd_cb close_cb)
{
    struct stream_output_fd* out;
    int ret;
    int errno_save;
    
    out=malloc(sizeof(struct stream_output_fd));
    if(out==NULL)
    {
        errno=ENOMEM;
        return -1;
    }
    
    ret=fcntl(fd,F_GETFL);
    
    errno=0;
    ret=fcntl(fd,F_SETFL,O_NONBLOCK|ret);
    if(ret<0)
    {
        errno_save=errno;
        free(out);
        errno=errno_save;
        return -1;
    }
    
    out->stream=stream;
    out->fd=fd;
    out->output_cb=output_cb;
    out->close_cb=close_cb;
    
    ev_io_init(&out->io,stream_output_cb,fd,EV_WRITE);
    
    stream->output_fd=out;
    
    return 0;
}

void stream_start_output_fd(Stream* stream,struct ev_loop* loop)
{
    if(stream->output_fd)
    {
        ev_io_start(loop,&stream->output_fd->io);
    }
}

void stream_stop_output_fd(Stream* stream,struct ev_loop* loop)
{
    if(stream->output_fd)
    {
        ev_io_start(loop,&stream->output_fd->io);
    }
}
