
#include "file-cte-log.h"

#include <stdio.h>
#include <string.h>


/*
Внешний индекс указывает на X - координату (обычно это обороты), внутренний индекс (ползунок: цикловое наполнение, дроссель, давление и т.д.)
есть ошибка диапазона при использовании таблиц32 с указанием что читаем 16 происходит выход за границы массива
*/
int read_cte_core(const TABLE_TYPE table_type, const char *file_name, float **DATA, char *input_src_head, char *input_src_caption)
{
	int input_data_count = 0;
	FILE *f_input;
	char buf_trash[256];
	int isize, jsize;
	PARAM_SIZE_BY_TABLE(table_type, isize, jsize);
	if(input_src_head == 0)
		input_src_head = buf_trash;
	if(input_src_caption == 0)
		input_src_caption = buf_trash;

	f_input = fopen(file_name, "r");
	fscanf(f_input, "%s", input_src_head);
	for(int sym = 0, i = 0; sym != '\n';)
	{
		sym = fgetc(f_input);
		if(sym == '\n')
		{
			if(i == 0)
				sym = 0;
			else
				input_src_caption[i] = 0;
		}else
			input_src_caption[i++] = static_cast<char>(sym);
	}
	for(int j = 0; j < jsize; j++)
	{
		for(int i = 0; i < isize; i++)
		{
			DATA[j][i] = 0.;
		}
	}
	
	for(int sym = 0, i, j; sym != EOF; )
	{
		sym = fgetc(f_input);
		switch(sym)
		{
		case 'X':
		case 'x':
			fscanf(f_input, "%i", &i);
			break;
		case 'Z':
		case 'z':
			fscanf(f_input, "%i", &j);
			break;
		case '=':
			fscanf(f_input, "%f", &DATA[j-1][i-1]);
			input_data_count++;
			break;
		default:;
		}
	}
	fclose(f_input);
	return input_data_count;
}
/*
Удаляет нолики из текстового числа, что бы число "1,000" стало "1"
*/
void del_str_txtzeroes_inplace(char *txt)
{
	int len;
	bool done = true;//пока не убедились, что число вещественное не будем ничего удалять
	for (int sym = len = 0; (sym = txt[len]) != 0; len++)
	{
		if (sym == ',' || sym == '.')
			done = false;
	}
	
	for (int i = len; i && !done; i--)
	{
		if (!done && txt[i - 1] == '0')
			txt[i - 1] = 0;
		else
			if (!done && (txt[i - 1] == ',' || txt[i - 1] == '.'))
			{
				done = true;
				txt[i - 1] = 0;
			}
			else
				done = true;
	}
}

int write_cte_core(const TABLE_TYPE table_type, const char *file_name, const float * const * DATA, const char *input_src_head, const char *input_src_caption)
{
	//char int_raw_s[64], int_nozero_s[64];
	char buffer[64];
	FILE *f_output;
	int isize, jsize;
	PARAM_SIZE_BY_TABLE(table_type, isize, jsize);
	f_output = fopen(file_name, "w");
	fprintf(f_output, "%s\n", input_src_head);
	fprintf(f_output, "%s\n", input_src_caption);

	for (int j = 0; j < jsize; j++)
	{
		for (int i = 0; i < isize; i++)
		{
			/*
			if (DATA[j][i] > 1)
				fprintf(f_output, "X%iZ%i=%0.1f\n", i + 1, j + 1, DATA[j][i]);
			else
				//fprintf(f_output, "X%iZ%i=%0.7f\n", i + 1, j + 1, DATA[j][i]);
			*/
				
			sprintf(buffer, "%0.10f", DATA[j][i]);
			//utilit_del_str_zeroes(int_raw_s, int_nozero_s);
			del_str_txtzeroes_inplace(buffer);
			fprintf(f_output, "X%iZ%i=%s\n", i + 1, j + 1, buffer);
			
		}
	}
	fclose(f_output);
	return 0;
}


int read_cte_data(const char *file_name, float DATA[16][16], char *input_src_head, char *input_src_caption)
{
	float **data_ptr = new float*[16];
	for (int i = 0; i < 16; i++)
	{
		data_ptr[i] = new float[16];
	}
	int res = read_cte_core(TABLE_TYPE::TBT_16x16, file_name, data_ptr, input_src_head, input_src_caption);
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			DATA[i][j] = data_ptr[i][j];
		}
		delete[] data_ptr[i];
	}
	delete[] data_ptr;
	return res;
}

int read_cte_data32(const char *file_name, float DATA[32][32], char *input_src_head, char *input_src_caption)
{
	float **data_ptr = new float*[32];
	for (int i = 0; i < 32; i++)
	{
		data_ptr[i] = new float[32];
	}
	int res = read_cte_core(TABLE_TYPE::TBT_32x32, file_name, data_ptr, input_src_head, input_src_caption);
	for (int i = 0; i < 32; i++)
	{
		for (int j = 0; j < 32; j++)
		{
			DATA[i][j] = data_ptr[i][j];
		}
		delete[] data_ptr[i];
	}
	delete[] data_ptr;
	return res;
}
/*
Заглушка для записи таблиц 16x16

Запись в вайл cte

позже будет удалена
*/
int write_cte_data(const char *file_name, const float DATA[16][16], const char *input_src_head, const char *input_src_caption)
{
	float **data_ptr = new float*[16];
	for (int i = 0; i < 16; i++)
	{
		data_ptr[i] = new float[16];
		for (int j = 0; j < 16; j++)
		{
			data_ptr[i][j] = DATA[i][j];
		}
	}
	int res = write_cte_core(TABLE_TYPE::TBT_16x16, file_name, data_ptr, input_src_head, input_src_caption);
	for (int i = 0; i < 16; i++)
	{
		delete[] data_ptr[i];
	}
	delete[] data_ptr;
	return res;
}

int write_cte_data32(const char *file_name, const float DATA[32][32], const char *input_src_head, const char *input_src_caption)
{
	float **data_ptr = new float*[32];
	for (int i = 0; i < 32; i++)
	{
		data_ptr[i] = new float[32];
		for (int j = 0; j < 32; j++)
		{
			data_ptr[i][j] = DATA[i][j];
		}
	}
	int res = write_cte_core(TABLE_TYPE::TBT_32x32, file_name, data_ptr, input_src_head, input_src_caption);
	for (int i = 0; i < 32; i++)
	{
		delete[] data_ptr[i];
	}
	delete[] data_ptr;
	return res;
}