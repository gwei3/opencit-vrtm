
#ifdef _WIN32
bool start_vrtm_interface(const char* name);
#elif __linux__
void* start_vrtm_interface(void* name);
#endif