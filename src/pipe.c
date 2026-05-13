#include "pipe.h"
#include <stdio.h>
#include <string.h>
 
#define FD_TO_PIPE(fd)  ((fd)/2)
#define FD_IS_WRITE(fd) ((fd)%2==1)
 
void pipe_pool_init(PipePool *pool) { memset(pool,0,sizeof(*pool)); }
 
int pipe_open(PipePool *pool, int out_fds[2]) {
    if (pool->n_pipes>=MAX_PIPES) return -1;
    int id=pool->n_pipes++;
    Pipe *p=&pool->pipes[id];
    memset(p,0,sizeof(*p));
    p->id=id; p->read_open=1; p->write_open=1;
    out_fds[0]=id*2; out_fds[1]=id*2+1;
    printf("%s  [pipe]%s Created pipe %d  (read_fd=%d write_fd=%d)\n",
           CYAN,RESET,id,out_fds[0],out_fds[1]);
    return 0;
}
 
int pipe_write(PipePool *pool, int fd, const char *buf, int n) {
    if (!FD_IS_WRITE(fd)) { printf("%s  [pipe] fd %d not write end%s\n",RED,fd,RESET); return -1; }
    Pipe *p=&pool->pipes[FD_TO_PIPE(fd)];
    if (!p->read_open) { printf("%s  [pipe] read end closed%s\n",RED,RESET); return -1; }
    int w=0;
    while (w<n) {
        if (p->nwrite-p->nread>=PIPE_BUF_SIZE) { break; }
        int avail=PIPE_BUF_SIZE-(p->nwrite-p->nread);
        int tw=(n-w<avail)?n-w:avail;
        for (int i=0;i<tw;i++) { p->data[p->nwrite%PIPE_BUF_SIZE]=buf[w+i]; p->nwrite++; }
        w+=tw;
    }
    printf("%s  [write]%s pipe%d  %d bytes: \"%.*s\"\n",GREEN,RESET,p->id,w,w,buf);
    return w;
}
 
int pipe_read(PipePool *pool, int fd, char *buf, int n) {
    if (FD_IS_WRITE(fd)) { printf("%s  [pipe] fd %d not read end%s\n",RED,fd,RESET); return -1; }
    Pipe *p=&pool->pipes[FD_TO_PIPE(fd)];
    if (p->nread==p->nwrite) {
        if (!p->write_open) return -1;
        printf("%s  [read]%s  pipe%d empty — would block\n",YELLOW,RESET,p->id);
        return 0;
    }
    int avail=p->nwrite-p->nread;
    int tr=(n<avail)?n:avail;
    for (int i=0;i<tr;i++) { buf[i]=p->data[p->nread%PIPE_BUF_SIZE]; p->nread++; }
    buf[tr]='\0';
    printf("%s  [read]%s  pipe%d  %d bytes: \"%s\"\n",BLUE,RESET,p->id,tr,buf);
    return tr;
}
 
int pipe_close(PipePool *pool, int fd) {
    Pipe *p=&pool->pipes[FD_TO_PIPE(fd)];
    if (FD_IS_WRITE(fd)) { p->write_open=0; printf("%s  [close]%s write end pipe%d\n",YELLOW,RESET,p->id); }
    else { p->read_open=0; printf("%s  [close]%s read  end pipe%d\n",YELLOW,RESET,p->id); }
    return 0;
}
 
void pipe_print_status(const PipePool *pool) {
    printf("\n%s+-- Pipe Status --%s\n",CYAN,RESET);
    for (int i=0;i<pool->n_pipes;i++) {
        const Pipe *p=&pool->pipes[i];
        printf("| Pipe %d  R:%s%s%s W:%s%s%s  %d/%d bytes used\n",i,
               p->read_open?GREEN:RED, p->read_open?"open":"closed",RESET,
               p->write_open?GREEN:RED, p->write_open?"open":"closed",RESET,
               p->nwrite-p->nread,PIPE_BUF_SIZE);
    }
    printf("%s+------------------%s\n\n",CYAN,RESET);
}
 
void pipe_demo(PipePool *pool) {
    int fds[2]; pipe_open(pool,fds);
    const char *m1="Hello from producer!";
    const char *m2="OS pipes use ring buffers.";
    pipe_write(pool,fds[1],m1,(int)strlen(m1));
    pipe_write(pool,fds[1],m2,(int)strlen(m2));
    pipe_close(pool,fds[1]);
    char buf[128];
    pipe_read(pool,fds[0],buf,64);
    pipe_read(pool,fds[0],buf,64);
    pipe_close(pool,fds[0]);
    pipe_print_status(pool);
}