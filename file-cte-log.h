#pragma once

#include <vector>
#include <map>
#include <string>
#include <utility>


/*
	cte-file.cpp
*/
/*
Must std::setlocale(LC_ALL, "ru"); to set ',' as a decimal separator
*/
/*
Reading of .cte chiptuner file format / Чтение файлов формата .cte chiptuner
Supported only one sets of calibrations per file / Поддерживается только один набор калибровок на файл, не вижу смысла в мультикалибровках
return: 0 - all good, 1 - file opening problem, 2 - cte file corrupted, 3 - wrong "type"
file_name
type
data
head
caption

Внешний индекс указывает на X - координату (обычно это обороты), внутренний индекс (ползунок: цикловое наполнение, дроссель, давление и т.д.)
есть ошибка диапазона при использовании таблиц32 с указанием что читаем 16 происходит выход за границы массива
*/
int read_cte(const char* file_name, const uint16_t js, const uint16_t is, float** data, char* head, char* caption);

template <uint16_t n, uint16_t m>
int read_cte_file(const char* file_name, float data[n][m], char* input_src_head, char* input_src_caption)
{
	float** data_ptr = new float* [n];
	for (int i = 0; i < n; i++)
		data_ptr[i] = data[i];
	int res = read_cte(file_name, n, m, data_ptr, input_src_head, input_src_caption);
	delete[] data_ptr;
	return res;
}


int write_cte(const char* file_name, const uint16_t js, const uint16_t is, const float* const* data, const char* head, const char* caption);
template <uint16_t n, uint16_t m>
int write_cte_file(const char* file_name, const float data[16][16], const char* input_src_head, const char* input_src_caption)
{
	const float** data_ptr = new const float* [n];
	for (int i = 0; i < n; i++)
		data_ptr[i] = data[i];
	int res = write_cte(file_name, n, m, data_ptr, input_src_head, input_src_caption);
	delete[] data_ptr;
	return res;
}



/*
	file-log.cpp
*/
std::vector<std::string> get_all_logs_filenames(const std::string& path);

typedef std::map<std::string, std::vector<std::string>> data_logs_t;

int read_logs_csv(const std::string &file_name, data_logs_t &data_logs);


//calling looks like: auto dTRT = get_logs_data<int>(data_logs, {"TRT", "THR", "Дроссель"});
//instantiated for <int> and <float>
template <typename T>
std::vector<T> get_logs_data(const data_logs_t& data_logs, const std::vector<std::string>& names);

