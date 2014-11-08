#ifndef _DOMBUILDER_
#define  _DOMBUILDER_


extern "C" int create_domain(int argc, char **argv);
extern "C" int tftp_get_file (char* dir, char *cmdline);
extern "C" int tftp_get_and_uncompress (char* dir, char *cmdline);

extern "C" int connect_to_mtwproxy ( );
extern "C" void populate_whitelist_hashes();
extern "C" bool is_kernel_hash_whitelisted(char* kernel_hash);
extern "C" bool is_hash_whitelisted(u32 parent, int hashtype, int rgHashsize, byte* rgHash);

#endif
