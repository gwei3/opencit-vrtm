#ifndef _LOADCONFIGH_
#define _LOADCONFIGH_

int load_config(const char * configFile, std:: map<std::string, std::string> & config_map);
void clear_config(std:: map<std::string, std::string> & config_map);

inline std::string trim_right_copy(const std::string &s, const std::string &delimiters)
{
	return s.substr(0,s.find_last_not_of(delimiters) +1 );
}

inline std::string trim_left_copy(const std::string &s, const std::string &delimiters)
{
	return s.substr(s.find_first_not_of(delimiters));
}

inline std::string trim_copy(const std::string &s, const std::string delimiters)
{
	return trim_left_copy(trim_right_copy(s,delimiters),delimiters);
}

#endif
