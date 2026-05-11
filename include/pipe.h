#ifndef PIPE_H
#define PIPE_H

#include "types.h"
#include <stddef.h>

/*
 * Pipe simulator — mirrors Chapter 7 (Inter-process communication).
 *
 * Each pipe is a fixed-size ring buffer (like the PIPESIZE=512 in
 * your course OS), with a read-end fd and a write-end fd.
 */

#define PIPE_BUF_SIZE  512
#define MAX_PIPES      8
#define MAX_FDS        (MAX_PIPES * 2)

typedef struct {
    char  data[PIPE_BUF_SIZE];
    int   nread;          /* absolute byte index of next read   */
    int   nwrite;         /* absolute byte index of next write  */
    int   read_open;      /* 1 = read end still open            */
    int   write_open;     /* 1 = write end still open           */
    int   id;             /* pipe id                            */
} Pipe;

typedef struct {
    Pipe  pipes[MAX_PIPES];
    int   n_pipes;
} PipePool;

/* ---------- public API ---------- */
void pipe_pool_init(PipePool *pool);

/* Returns read_fd in out[0], write_fd in out[1]. Returns 0 on success. */
int  pipe_open(PipePool *pool, int out_fds[2]);

int  pipe_write(PipePool *pool, int write_fd, const char *buf, int n);
int  pipe_read(PipePool *pool,  int read_fd,  char *buf,       int n);
int  pipe_close(PipePool *pool, int fd);

void pipe_print_status(const PipePool *pool);

/* Interactive demo: producer writes, consumer reads */
void pipe_demo(PipePool *pool);

#endif /* PIPE_H */