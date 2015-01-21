#ifndef PTY_COMM_H
#define PTY_COMM_H

extern int comm_unlink();
extern int comm_read();
extern int comm_accept();
extern int comm_write();
extern int comm_putfd();
extern int comm_getfd();

#endif
