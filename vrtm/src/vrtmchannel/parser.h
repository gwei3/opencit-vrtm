#ifndef byte
typedef unsigned char byte;
#endif

int cbuf_to_xmlrpc(const char* func, const char *method, int size, const byte* data, int bufsize, byte* buf);
int args_to_xmlrpc(const char* method, int nargs, char** args, int bufsize, byte* buf);

int xmlrpc_to_cbuf(const char* func, int* psize, byte* data, const byte* buf);
int xmlrpc_to_args(char** psz, int* pnargs, char**pargs, const byte* buf);
