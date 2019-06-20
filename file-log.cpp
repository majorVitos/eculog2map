
#include "file-cte-log.h"

#include <memory>
#include <vector>
#include <cstring>

#include <cstdio>
#include <cstdlib>
#include <cctype>

//#include <cstdarg> 

#include <cassert>

#include <sstream>
#include <algorithm>


#include <filesystem>
/*
Для компилятора G++ из состава GCC версии 6.x и выше также потребуется флаг компоновщика: -lstdc++fs
*/




#include <iostream>


std::vector<std::string> get_all_logs_filenames(const std::string &path)
{
	std::vector< std::string > res;
	namespace fs = std::experimental::filesystem;//Visual studio

	for (const auto &entry : fs::directory_iterator(path) )
		res.push_back( entry.path().generic_string() );

	/****Windows legacy
	#if defined (_MSC_VER) || defined WIN32
	#include <windows.h>
	#include <tchar.h>
	***********
	HANDLE hf;
	WIN32_FIND_DATAA FindFileData;
	hf = FindFirstFileA((path + "*.csv").c_str(), &FindFileData);
	if (hf != INVALID_HANDLE_VALUE)
	{
		do
		{
			printf("%s\n", FindFileData.cFileName);
			res.push_back(path + FindFileData.cFileName);
		} while (FindNextFileA(hf, &FindFileData) != 0);
		FindClose(hf);
	}*/
	return res;
}


typedef std::pair<std::string, std::vector<std::string>> type1;

int read_logs_csv(const std::string &file_name, data_logs_t &data_logs)
{
	char buffer[256];
	char* tmp;
	std::vector<type1> data;
	bool _do;

	std::unique_ptr<std::FILE, int(*)(std::FILE*)> fi(fopen(file_name.c_str(), "rb"), std::fclose);
	if (!fi)
		return 1;

	//filling names
	do
	{
		if (fscanf(fi.get(), "%255[^,\r\n],", buffer) == EOF)
			return EOF;
		buffer[255] = 0;
		tmp = buffer;
		for (; isalnum(*(unsigned char*)tmp) == 0 && *tmp != 0; tmp++);//skip firsts whitespaceses / пропуск начальных пробелов
		if ((_do = data_logs.emplace(std::string(tmp), std::vector<std::string>()).second))
			data.push_back(type1(tmp, std::vector<std::string>()));
	} while (_do);
	if (data.size() == 0)//No data in file
		return 2;
	_do = true;
	//filling data
	while((fscanf(fi.get(), "\r\n")) == 0 && _do)
	{
		for (auto i = data.begin(); i != data.end(); ++i)
		{
			if ((fscanf(fi.get(), "%255[^,\r\n],", buffer)) == EOF)
			{
				while(i != data.begin())//Partially read data must be cleared / частично загружены данные, нужно откатить назад 
					(*(--i)).second.pop_back();
				_do = false;
				break;
			}
			(*i).second.push_back(buffer);
		}
	}
	for (auto i = data.begin(); i != data.end(); ++i)
	{
		auto v = data_logs.find((*i).first);
		assert(v != data_logs.end());
		(*v).second.swap((*i).second);
	}
	return 0;
}


template <typename T>
std::vector<T> get_logs_data(const data_logs_t& data_logs, const std::vector<std::string>& names)
{
	auto it = data_logs.cend();
	std::vector<T> ret;
	for (const auto &name : names)
	{
		if ((it = data_logs.find(name)) != data_logs.end())
		{
			ret.reserve((*it).second.size());
			for (const auto& i : (*it).second)
			{
				std::stringstream s(i);
				T sv;
				s >> sv;
				ret.push_back(sv);
			}
			break;
		}
	}
	return ret;
}

template
std::vector<int> get_logs_data(const data_logs_t& data_logs, const std::vector<std::string> &names);
template
std::vector<float> get_logs_data(const data_logs_t& data_logs, const std::vector<std::string> &names);