#ifndef __STREAM_H__
#define __STREAM_H__

#include <stddef.h>
#include <stdbool.h>
#include <ev.h>

typedef struct Stream_s Stream;

typedef void(*stream_fd_cb)(Stream* stream,struct ev_loop* loop,int fd);

struct stream_input_fd;
struct stream_output_fd;

struct Stream_s
{
    char* buff;
    size_t size;
    
    size_t idx;
    size_t len;
    
    struct stream_input_fd* input_fd;
    struct stream_output_fd* output_fd;
};

struct stream_input_fd
{
    ev_io io;
    
    Stream* stream;
    int fd;
    stream_fd_cb input_cb;
    stream_fd_cb close_cb;
};

struct stream_output_fd
{
    ev_io io;
    
    Stream* stream;
    int fd;
    stream_fd_cb output_cb;
    stream_fd_cb close_cb;
};

Stream* new_stream(size_t size);
size_t stream_pending_len(Stream* stream);
int stream_write(Stream* stream,const char* in,size_t len);
size_t stream_read(Stream* stream,char* out,size_t len);
size_t stream_peek(Stream* stream,char* out,size_t len);
size_t stream_skip(Stream* stream,size_t len);

int stream_set_input_fd(Stream* stream,int fd,stream_fd_cb input_cb,stream_fd_cb close_cb);
void stream_start_input_fd(Stream* stream,struct ev_loop* loop);
void stream_stop_input_fd(Stream* stream,struct ev_loop* loop);

int stream_set_output_fd(Stream* stream,int fd,stream_fd_cb output_cb,stream_fd_cb close_cb);
void stream_start_output_fd(Stream* stream,struct ev_loop* loop);
void stream_stop_output_fd(Stream* stream,struct ev_loop* loop);

#endif /* __STREAM_H__ */