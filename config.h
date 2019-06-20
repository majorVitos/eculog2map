#pragma once

#include <cstdio>
#include <cctype>
#include <string>
#include <vector>
#include <utility>

typedef std::pair<std::string, std::vector<std::string>> tparam_str;
typedef std::pair<std::string, std::vector<float>> tparam_f;
typedef std::pair<std::string, std::vector<int>> tparam_i;

class tsconfig
{
	int read_comment(std::FILE *f);
	int read_name(std::FILE *f, std::string &name);
	int read_str(std::FILE *f, std::vector<std::string> &strings);
	int read_float(std::FILE *f, std::vector<float> &floats);
	int read_int(std::FILE *f, std::vector<int> &ints);
	std::vector<tparam_str> paramss;
	std::vector<tparam_f>	paramsf;
	std::vector<tparam_i>	paramsi;
public:
	tsconfig()
	{}
	tsconfig(const std::string file);
	int open(const std::string file);

	std::vector<int> geti(const std::string name) throw();
	std::vector<float> getf(const std::string name) throw();
	std::vector < std::string > gets(const std::string name) throw();
	//int
	int error;
	std::string error_str;
};
