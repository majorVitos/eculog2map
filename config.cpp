
#include "config.h"

#include <memory>

inline int skip_whitespace(std::FILE *f)
{
	int symb = 0;
	while (std::isspace((symb = fgetc(f))) != 0 && symb != EOF);
	return symb;
}

tsconfig::tsconfig(const std::string file)
{
	std::unique_ptr<std::FILE, int(*)(std::FILE*)> fi(std::fopen(file.c_str(), "rb"), fclose);
	int ret = 0;
	if (!fi)
	{
		error = 1;
		error_str = "cant open file: " + file;
	}
	do
	{
		std::string name;
		std::vector<std::string> strings;
		std::vector<float> floats;
		std::vector<int> ints;
		if ((ret = read_comment(fi.get())) == 0)
			continue;
		if ((ret = read_name(fi.get(), name)) != 0)
		{
			error = 2;
			error_str = "";
		}
		if ((ret = read_str(fi.get(), strings)) == 0)
		{
			paramss.push_back(tparam_str(name, strings));
			continue;
		}
		if (ret == EOF)	break;
		if ( (ret = read_float(fi.get(), floats)) == 0)
		{
			paramsf.push_back(tparam_f(name, floats));
			continue;
		}
		if (ret == EOF)	break;
		if ( (ret = read_int(fi.get(), ints)) == 0)
		{
			paramsi.push_back(tparam_i(name, ints));
			continue;
		}
		if (ret == EOF)	break;
		error = 10;
		error_str = "";
		break;
	} while (1);
}

int tsconfig::read_comment(std::FILE *f)
{
	int symb;
	symb = skip_whitespace(f);
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
	ret = fscanf(f, "%255[^,],", buffer);
	name = std::string(buffer);
	if (ret > 0)	return 0;//number or items readed not needed
	return ret;//EOF
}
int tsconfig::read_str(std::FILE *f, std::vector<std::string> &strings)
{
	char buffer[256];
	std::string str;
	int symb;
	int ret;
	symb = skip_whitespace(f);
	if (symb != '\"')
	{
		std::fseek(f, -1, SEEK_CUR);
		return 1;	//not a string
	}
	do
	{
		ret = fscanf(f, "%[^\"]\"", buffer);
		strings.push_back(buffer);
		if (ret < 0)	return ret;//EOF
		do
		{
			symb = fgetc(f);
			if (symb == '\n')	return 0;
		} while (symb != '\"');
	} while (1);
//	return 0;
}
int tsconfig::read_float(std::FILE *f, std::vector<float> &floats)
{
	int symb = 0;
	int ret = 0;
	float value = 0;
	symb = skip_whitespace(f);
	std::fseek(f, -1, SEEK_CUR);
	for(;(symb = fgetc(f)) != '.'; ret++)
	{
		if (symb == EOF)				return EOF;
		if (symb == '+' || symb == '-')	continue;
		if (isdigit(symb) == 0)
		{
			std::fseek(f, -(ret + 1), SEEK_CUR);
			return 1;
		}
	}
	std::fseek(f, -(ret+1), SEEK_CUR);
	do
	{
		ret = fscanf(f, "%f,", &value);
		if (ret < 0)	return ret;//EOF
		if (ret > 0)	floats.push_back(value);
	} while (ret);
	return 0;
}
int tsconfig::read_int(std::FILE *f, std::vector<int> &ints)
{
	int symb = 0;
	int ret = 0;
	int value = 0;
	symb = skip_whitespace(f);
	std::fseek(f, -1, SEEK_CUR);
	do
	{
		ret = fscanf(f, "%d,", &value);
		if (ret < 0)	return ret;//EOF
		if (ret > 0)	ints.push_back(value);
	} while (ret);
	return 0;
}
int tsconfig::get_int(const std::string name, std::vector<int> &param)
{
	for (auto i = paramsi.begin(); i != paramsi.end(); ++i)
	{
		if ((*i).first.compare(name) == 0)
		{
			param = std::vector<int>((*i).second);
			return 0;
		}
	}
	return 1;
}


