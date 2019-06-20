
#include "config.h"

#include <memory>

#include <algorithm>

inline int skip_whitespace(std::FILE *f)
{
	int symb = 0;
	while (std::isspace((symb = fgetc(f))) != 0 && symb != EOF);
	return symb;
}

tsconfig::tsconfig(const std::string file)
{
	open(file);
}

int tsconfig::open(const std::string file)
{
	error = 0;
	std::unique_ptr<std::FILE, int(*)(std::FILE*)> fi(std::fopen(file.c_str(), "rb"), fclose);
	int ret = 0;
	if (!fi)
	{
		error_str = "cant open file: " + file;
		return error = 1;
	}
	do
	{
		std::string name;
		std::vector<std::string> strings;
		std::vector<float> floats;
		std::vector<int> ints;
		if ((ret = read_comment(fi.get())) == 0)	continue;
		if (ret == EOF)	break;
		if ((ret = read_name(fi.get(), name)) == 0)
		{
			if ((ret = read_str(fi.get(), strings)) == 0)
			{
				paramss.push_back(tparam_str(name, strings));
				continue;
			}
			if (ret == EOF)//Посли чтения имени ожидалось, что будет что то прочитано
			{
				error = 3;
				error_str = "Expected parameter to read";
				break;
			}
			if ((ret = read_float(fi.get(), floats)) == 0)
			{
				paramsf.push_back(tparam_f(name, floats));
				continue;
			}
			if (ret == EOF)//Посли чтения имени ожидалось, что будет что то прочитано
			{
				error = 3;
				error_str = "Expected parameter to read";
				break;
			}
			if ((ret = read_int(fi.get(), ints)) == 0)
			{
				paramsi.push_back(tparam_i(name, ints));
				continue;
			}
			if (ret == EOF)//Посли чтения имени ожидалось, что будет что то прочитано
			{
				error = 3;
				error_str = "Expected parameter to read";
				break;
			}
		}
		else
		{
			if (ret == EOF)	break;
			error = 2;
			error_str = "Error reading parameter name";
			break;
		}
		error = 10;
		error_str = "Probably problem in code";
	} while (!error);
	return error;
}

int tsconfig::read_comment(std::FILE *f)
{
	int symb;
	if ((symb = skip_whitespace(f)) < 0)
		return symb;//EOF reached
	if (symb == ';')
	{
		while ((symb = fgetc(f)) != '\n' && symb != EOF);
		return 0;
	}
	std::fseek(f, -1, SEEK_CUR);//return to one symbol, because there is no comment line
	return 1;
}
int tsconfig::read_name(std::FILE *f, std::string &name)
{
	char buffer[256];
	int ret;
	ret = fscanf(f, "%255[^, ],", buffer);
	buffer[255] = 0;//No theroless buffer warning message
	name = std::string(buffer);
	if (ret > 0)	return 0;//All fine
	return ret;//EOF and no name readed
}
int tsconfig::read_str(std::FILE *f, std::vector<std::string> &strings)//Potential problem !!!
{
	char buffer[256];
	std::string str;
	int symb;
	int ret;
	bool flag = false;//At least one value readed
	symb = skip_whitespace(f);
	if (symb != '\"')
	{
		std::fseek(f, -1, SEEK_CUR);
		return 1;	//not a string
	}
	do
	{
		ret = fscanf(f, "%[^\"]\"", buffer);
		buffer[255] = 0;
		if (ret < 0 && !flag)	return ret;//EOF
		if (ret < 0 && flag)	return 0;//All done
		if (ret > 0)
		{
			strings.push_back(buffer);
			flag = true;
		}
		else return 1;
		do
		{
			symb = fgetc(f);
			if (symb == '\n' || symb == EOF)	return 0;//Line or file ended
		} while (symb != '\"');
	} while (1);
	return 0;
}
int tsconfig::read_float(std::FILE *f, std::vector<float> &floats)
{
	int symb = 0;
	int ret = 0;
	float value = 0;
	bool flag = false;//At least one value readed
	symb = skip_whitespace(f);
	std::fseek(f, -1, SEEK_CUR);
	for(;(symb = fgetc(f)) != '.'; ret++)
	{
		if (symb == EOF)				return EOF;
		if (symb == '+' || symb == '-')	continue;
		if (isdigit(symb) == 0)
		{
			std::fseek(f, -(ret + 1), SEEK_CUR);
			return 1;	//not a float value
		}
	}
	std::fseek(f, -(ret+1), SEEK_CUR);
	do
	{
		ret = fscanf(f, "%f,", &value);
		if (ret < 0 && !flag)	return ret;//EOF
		if (ret < 0 && flag)	return 0;
		if (ret > 0)
		{
			floats.push_back(value);
			flag = true;
		}
	} while (ret);
	return 0;
}
int tsconfig::read_int(std::FILE *f, std::vector<int> &ints)
{
	int symb = 0;
	int ret = 0;
	int value = 0;
	bool flag = false;//At least one value readed
	symb = skip_whitespace(f);
	std::fseek(f, -1, SEEK_CUR);
	do
	{
		ret = fscanf(f, "%d,", &value);
		if (ret < 0 && !flag)	return ret;//EOF
		if (ret < 0 && flag)	return 0;//All done
		if (ret > 0)
		{
			ints.push_back(value);
			flag = true;
		}
	} while (ret);
	return 0;
}


std::vector<int> tsconfig::geti(const std::string name) throw()
{
	for (auto i : paramsi)
		if (i.first.compare(name) == 0)
			return i.second;
	return std::vector<int>();
}
std::vector<float> tsconfig::getf(const std::string name) throw()
{
	for (auto i : paramsf)
		if (i.first.compare(name) == 0)
			return i.second;
	return std::vector<float>();
}
std::vector< std::string > tsconfig::gets(const std::string name) throw()
{
	for (auto i : paramss)
		if (i.first.compare(name) == 0)
			return i.second;
	return std::vector< std::string >();
}

