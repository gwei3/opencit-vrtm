#include "vrtmsockets.h"

int initialise_lib(WSADATA* wsaData) {
#ifdef _WIN32
	return WSAStartup(MAKEWORD(2, 2), wsaData);
#else
	return 0;
#endif
}

void clean_lib() {
#ifdef _WIN32
	WSACleanup();
#endif
}

int get_socket_error() {
#ifdef _WIN32
	return WSAGetLastError();
#else
	return 0;
#endif
}

/*
int vrtmsocket_write(int socket_fd, char* buffer, int buffer_len) {
	int iResult;
#ifdef _WIN32
	iResult = send(socket_fd, buffer, buffer_len, 0);
#elif __linux__
	iResult = write(socket_fd, buffer, buffer_len);
#endif
	return iResult;
}

int vrtmsocket_read(int socket_fd, char* buffer, int buffer_len) {
	int iResult;
#ifdef _WIN32
	iResult = recv(socket_fd, buffer, buffer_len, 0);
#elif __linux__
	iResult = read(socket_fd, buffer, buffer_len);
#endif
	return iResult;
}
*/
void close_connection(int socket_fd) {
#ifdef _WIN32
	closesocket(socket_fd);
#elif __linux__
	close(socket_fd);
#endif
}