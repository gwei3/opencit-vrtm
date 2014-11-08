#ifndef _TCPCHAN_
#define  _TCPCHAN_

#ifndef TCSERVICE
struct tcBuffer {
    int                 m_procid;
    unsigned int        m_reqID;
    unsigned int        m_reqSize;
    unsigned int 		m_ustatus;
    int                 m_origprocid;
};
typedef struct tcBuffer tcBuffer;

#else
#include "tcIO.h"
#endif


int ch_register(int fd);
int ch_open(char* serverip, int port);
int ch_read(int fd, void *buf, int maxsize);
int ch_write(int fd, void *buf, int size);
void ch_close(int fd);




#endif
