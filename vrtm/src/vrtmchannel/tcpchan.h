#include "vrtmCommon.h"

#define PADDEDREQ 64*1024
#define PARAMSIZE static_cast<int>(PADDEDREQ-sizeof(tcBuffer))

struct tcBuffer {
    unsigned int        m_reqID;
    unsigned int        m_reqSize;
    unsigned int 		m_ustatus;
};
typedef struct tcBuffer tcBuffer;

VRTMCHANNEL_DLLPORT void ch_register(int fd);
VRTMCHANNEL_DLLPORT int ch_open(char* serverip, int port);
VRTMCHANNEL_DLLPORT int ch_read(int fd, void *buf, int maxsize);
VRTMCHANNEL_DLLPORT int ch_write(int fd, void *buf, int size);
VRTMCHANNEL_DLLPORT void ch_close(int fd);



