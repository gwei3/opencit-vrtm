#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

#ifdef _WIN32
int initialise_lib(WSADATA* wsaData);
void clean_lib();
int get_socket_error();
#endif

void close_connection(int socket_fd);
