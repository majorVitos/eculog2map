#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>

enum TABLE_TYPE { TBT_16x16, TBT_32x16, TBT_32x32 };

void inline PARAM_SIZE_BY_TABLE(TABLE_TYPE TT, int &x, int &y)
{
	switch (TT)
	{
	case TBT_16x16:
		x = y = 16;
		break;
	case TBT_32x16:
		x = 32;
		y = 16;
		break;
	case TBT_32x32:
		x = y = 32;
		break;
	default:
		throw "TABLE_TYPE wrong";
		break;
	}
}

/*
	cte-file.cpp
*/
int read_cte_data(const char *file_name, float DATA[16][16], char *input_src_head, char *input_src_caption);
int read_cte_data32(const char *file_name, float DATA[32][32], char *input_src_head, char *input_src_caption);
int write_cte_data(const char *file_name, const float DATA[16][16], const char *input_src_head, const char *input_src_caption);
int write_cte_data32(const char *file_name, const float DATA[32][32], const char *input_src_head, const char *input_src_caption);


/*
	log_file.cpp
*/
int read_logs_csv(const char *file_name, float ***_logs_data_ptr, char ***_logs_params_ptr, int *_params_count, int *_data_count);
const float* get_logsdata_param_ptr(const float * const * logs_data, const char * const *param, const int params_count, const int num_aliases, ...);
void free_log_params(char **logs_params_ptr, const int params_count);
void free_log_data(float **logs_data, const int params_count);



#include <cstdarg> 

typedef std::map<std::string, std::vector<std::string>> data_logs_t;

int read_logs_csv2(const std::string &file_name, data_logs_t &data_logs);

template <typename T>
std::vector<T> get_logs_data(const data_logs_t &data_logs, const char *first, ...)
{
	std::vector<T> ret;
	auto it = data_logs.end();
	va_list v;
	bool done = false;
	const char* str = first;

	va_start(v, first);
	while(str && str[0])//
	{
		if( (it = data_logs.find(str)) != data_logs.end())
		{
			done = true;
			break;
		}
		str = va_arg(v, const char*);
	}
	va_end(v);
	ret.reserve((*it).second.size());
	for (auto i = (*it).second.cbegin(); i != (*it).second.cend(); ++i)
	{
		ret.push_back(std::stof(*i));
	}
	return ret;
}

int get_logs_data2(const data_logs_t& data_logs, std::vector<int> &ret, const char* first, ...);
int get_logs_data2(const data_logs_t& data_logs, std::vector<float>& ret, const char* first, ...);