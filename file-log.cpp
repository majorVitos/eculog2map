
#include <memory>
#include <vector>
#include <cstring>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstdarg> 

/*
Чтение из файла логов всех данных, его разбор
file_name - имя файла, _logs_data_ptr - все строки данных, _logs_params_ptr - все имена данных или параметры,
_params_count - число имен параметров, _data_count - число данных по каждому из параметров (например THRT[_data_count])
*/
int read_logs_csv(const char *file_name, float ***_logs_data_ptr, char ***_logs_params_ptr, int *_params_count, int *_data_count)
{
	char tmp[256];
	float **&logs_data_ptr = *_logs_data_ptr;
	char **&logs_params_ptr = *_logs_params_ptr;
	int &params_count = *_params_count;//число параметров в логе
	int &data_count = *_data_count;
	char delimiter_symbol = ',';		//Разделитель параметров

	std::unique_ptr<std::FILE, int (*)(std::FILE*)> fi(fopen(file_name, "rb"), std::fclose);
	if (!fi)
		return 1;
	
	/*Подсчет числа параметров в логе
	*/
	params_count = 1;
	for (int symb; (symb = fgetc(fi.get())) != '\n';)
	{
		if (symb == delimiter_symbol)
			params_count++;
	}
	fseek(fi.get(), 0, SEEK_SET);
	/*Заполнение имен параметров
	*/
	logs_params_ptr = new char*[params_count];
	for (int i = 0; i < params_count; i++)
	{
		fscanf(fi.get(), "%[^,\n],", tmp);
		char *tmp2 = tmp;//пропуск начальных пробелов
		for (; isalnum(*(unsigned char*)tmp2) == 0 && *tmp2 != 0; tmp2++)
		{
		}
		logs_params_ptr[i] = new char[std::strlen(tmp) + 1];
		std::strcpy(logs_params_ptr[i], tmp2);
	}
	/*Подсчет числа данных, по строкам
	*/
	data_count = -1;
	for (int symb; feof(fi.get()) == 0;)
	{
		symb = fgetc(fi.get());
		if (symb == '\n')
			data_count++;
	}
	fseek(fi.get(), 0, SEEK_SET);
	for (int symb; (symb = fgetc(fi.get())) != '\n' && feof(fi.get()) == 0;)
	{
	}


	logs_data_ptr = new float*[params_count];
	for (int i = 0; i < params_count; i++)
	{
		logs_data_ptr[i] = new float[data_count];
	}
	/*Заполнение данных
	*/
	for (int i = 0; i < data_count; i++)
	{
		for (int j = 0; j < params_count; j++)
		{
			char buffer[256];
			int k = 0, symb;
			for (; k < 255; k++)
			{
				symb = fgetc(fi.get());
				if (symb == delimiter_symbol || symb == '\n')
					break;
				if (isdigit(symb) == 0 && symb != '.')
					symb = ' ';
				buffer[k] = static_cast<char>(symb);
				buffer[k + 1] = 0;
			}
			logs_data_ptr[j][i] = std::strtof(buffer, 0); //std::strtod(buffer, 0);
	
			if (symb == '\n' && j != params_count - 1)
			{
				throw "Log file Error";
				return 1;
			}
		}
	}
	return 0;
}



/*
Возвращаяет адресс строки данных соответствующую имени str (например thrt, press, twat )
logs_data - все строки с данными, param - все имена данных, params_count - число имен или строк,
num_aliases - число псевдонимов параметра (TRT, THR, Дроссель), дальше строки имен параметров "TRT", "THR"
*/
const float* get_logsdata_param_ptr(const float * const * logs_data, const char * const *param, const int params_count, const int num_aliases, ...)
{
	va_list vl;
	const char *str;
	va_start(vl, num_aliases);
	for (int j = 0; j < num_aliases; j++)
	{
		str = va_arg(vl, char*);
		for (int i = 0; i < params_count; i++)
		{
			if (std::strcmp(param[i], str) == 0)
			{
				va_end(vl);
				return logs_data[i];
			}
		}
	}
	va_end(vl);
	return 0;
}

void free_log_params(char **logs_params_ptr, const int params_count)
{
	for (int i = 0; i < params_count; i++)
	{
		delete[] logs_params_ptr[i];
	}
	delete[] logs_params_ptr;
}

void free_log_data(float **logs_data, const int params_count)
{
	for (int i = 0; i < params_count; i++)
	{
		delete[] logs_data[i];
	}
	delete[] logs_data;
}

#if defined (_MSC_VER) || defined WIN32
#include <windows.h>
#include <tchar.h>
std::vector<std::string> get_all_logs_filenames()
{
	std::vector< std::string > file_names;
	HANDLE hf;
	WIN32_FIND_DATAA FindFileData;
	hf = FindFirstFileA("logs\\*.csv", &FindFileData);
	if (hf != INVALID_HANDLE_VALUE)
	{
		do
		{
			printf("%s\n", FindFileData.cFileName);
			file_names.push_back(FindFileData.cFileName);
		} while (FindNextFileA(hf, &FindFileData) != 0);
		FindClose(hf);
	}
	return file_names;
}
#else//UNIX
#include <sys/types.h>
#include <dirent.h>
std::vector<std::string> get_all_logs_filenames()
{
    std::vector< std::string > file_names;
	DIR *dir;
	struct dirent *entry;
	dir = opendir("./logs");
	if (!dir)
	{
		perror("diropen");
		throw "tratata";
	}
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_REG)
			file_names.push_back(entry->d_name);
	}
    closedir(dir);
    return file_names;
}
#endif
