typedef unsigned u32;
#ifndef byte
typedef unsigned char byte;
#endif

#ifndef NULL
#define NULL 0
#endif

#define SHA256DIGESTBYTESIZE       32
#ifdef RSIZE_MAX_STR
#define MAX_LEN RSIZE_MAX_STR
#else
#define	MAX_LEN 4096
#endif
