
#include "config.h"


#include <cctype>


#include <memory>


#if 0


if(table_size == 16)
	{
		config_res += read_config("1.cfg", "RPM_es16", 0, &RPM_Quantization);
		config_res += read_config("1.cfg", "Press16_quant", 0, &Factor_Quantization);
	}else
	{
		config_res += read_config("1.cfg", "RPM_es32", 0, &RPM_Quantization);
		config_res += read_config("1.cfg", "Press32_quant", 0, &Factor_Quantization);
	}
	config_res += read_config("1.cfg", "Throttle_quant", 0, &Throttle_Quantization);
	
	
	if (config_res)
	{
		throw "Config file error";
	}
	
	
#endif

	
/*
Читает конфигурацию из файла file_name, имя конфигурации name, количество элементов в конфигурации *plength, куда положить data

;это комментарий
Name1, i, 0, 0, 1;имя, тип i интегер или f флоат
*/
int read_config(const char *file_name, const char *name, int *plength, void *data)
{
	FILE *fi;
	char buffer[256];
	int buff_pos;

	int plength_fake;
	int state, next_state, symb, type, data_pos;
	int ret_val;

	if (!file_name)
		return 4;
	if (!name)
		return 3;
	/*если вызвали функцию, но ничего не хотим вернуть*/
	if (!plength && !data)
		return 2;
	if ((fi = fopen(file_name, "r")) == 0)
		return 1;

	std::vector<float> *dataf = reinterpret_cast<std::vector<float>*>(data);
	std::vector<int> *datai = reinterpret_cast<std::vector<int>*>(data);

	if (!plength)
		plength = &plength_fake;

	*plength = 0;
	state = 0;
	buff_pos = 0;
	data_pos = 0;
	type = 0;//integer for default
	ret_val = 0;
	
	for (; state != 100 && state != 10; )
	{
		symb = fgetc(fi);
		switch (state)
		{
		case 0://name read
			if (symb == EOF)
			{
				state = 100;
				ret_val = 10;
				break;
			}
			if (symb == ';')//comment detected
			{
				state = 1;
				break;
			}
			if (symb == '\n')//passing
				break;
			if (isalpha(symb) || ((buff_pos > 0) && (isdigit(symb) || symb == '_')) )//Name detected
			{
				if (buff_pos == 255)//buffer reaches end, suposedly file error
				{
					state = 100;
					continue;
				}
				buffer[buff_pos++] = symb;
				buffer[buff_pos] = 0;
				break;
			}
			if (symb == ',' || isblank(symb))//delimiter arived
			{
				buff_pos = 0;
				if (strcmp(buffer, name) == 0)//param finded
				{
					state = 2;
					continue;
				}
				state = 1;
				break;
			}
				state = 10;//machine or file error
			break;

		case 1://coment passing
			if (symb == '\n' || symb == EOF)
				state = 0;
			break;

		case 2://data read
			if (symb == EOF)
			{
				if (buff_pos != 0)
				{
					state = 4;
					next_state = 100;
					continue;
				}
				state = 10;//nothing readed this is file error
				continue;
			}
			if (symb == '\n')//end of data reached
			{
				fseek(fi, -1, SEEK_CUR);
				state = 4;
				next_state = 100;
				break;
			}
			if (symb == ',' || isblank(symb))
			{
				if (buff_pos == 0)
					continue;
				state = 4;
				next_state = 2;
				continue;
			}
			if (symb == '.')//float type
				type = 1;
			if (buff_pos == 255)
			{
				state = 10;
				continue;
			}
			buffer[buff_pos++] = symb;
			buffer[buff_pos] = 0;
			break;

		case 4://data convert
			fseek(fi, -1, SEEK_CUR);
			buff_pos = 0;
			state = next_state;
			if (plength)
				(*plength)++;
			if (data == 0)
				continue;
			if (type)
			{
				float tmp;
				sscanf(buffer, "%f", &tmp);
				dataf->push_back(tmp);
				//sscanf(buffer, "%f", pdataf + data_pos++);
			}
			else
			{
				int tmp;
				sscanf(buffer, "%i", &tmp);
				datai->push_back(tmp);
				//sscanf(buffer, "%i", pdatai + data_pos++);
			}
			break;

		default:
			throw "machine internal error";
		}
	}
	fclose(fi);
	return ret_val;
}
