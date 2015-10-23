
#define PADDEDREQ 64*1024
#define PARAMSIZE static_cast<int>(PADDEDREQ-sizeof(tcBuffer))

struct tcBuffer {
    unsigned int        m_reqID;
    unsigned int        m_reqSize;
    unsigned int 		m_ustatus;
};
typedef struct tcBuffer tcBuffer;

void ch_register(int fd);
int ch_open(char* serverip, int port);
int ch_read(int fd, void *buf, int maxsize);
int ch_write(int fd, void *buf, int size);
void ch_close(int fd);



