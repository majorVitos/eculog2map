
#include "file-cte-log.h"

#include <memory>
#include <cstdio>
//#include <string.h>


/*
Reading of .cte chiptuner file format / Чтение файлов формата .cte chiptuner
Supported only one sets of calibrations per file / Поддерживается только один набор калибровок на файл, не вижу смысла в мультикалибровках

Внешний индекс указывает на X - координату (обычно это обороты), внутренний индекс (ползунок: цикловое наполнение, дроссель, давление и т.д.)
есть ошибка диапазона при использовании таблиц32 с указанием что читаем 16 происходит выход за границы массива
*/
int read_cte(const char *file_name, const uint16_t js, const uint16_t is, float **data, char *head, char *caption)
{
	char buf[256];
	int ret, i, j;
	float value;
	char x, z, es;
	head = !head ? buf : head;
	caption = !caption ? buf : caption;
	std::unique_ptr<std::FILE, int(*)(std::FILE*)> fi(fopen(file_name, "rb"), std::fclose);
	if (!fi)
		return 1;
	if (fscanf(&*fi, "%255s\r\n", head) == EOF)
		return EOF;
	if (fscanf(&*fi, "%255[^\r\n]\r\n", caption) == EOF)
		return EOF;
	while ((ret = fscanf(&*fi, "%c%d%c%d%c%f\r\n", &x, &i, &z, &j, &es, &value)) != EOF)
	{
		if (ret < 6 || (x != 'x' && x != 'X') || (z != 'z' && z != 'Z') || (es != '=') || i > is || j > js)
			return 2;//cte file corrupted
		data[j - 1][i - 1] = value;
	}
	if (i != is || j != js)
		return 2;
	return 0;
}


/*
Subroutine deletes last zeros in rational number
Удаляет нолики из текстового числа, что бы число "1,000" стало "1"
*/
void del_str_txtzeroes_inplace(char *txt)
{
	uint32_t len = 0, p = 0, sym;
	bool rat = false;
	while((sym = txt[len++]) != 0)
	{
		if (sym == ',' || sym == '.')
		{
			rat = true;
			p = len - 1;
		}else if (!rat || sym != '0')
			p = len;
	}
	if (rat)
		txt[p] = 0;
}

int write_cte( const char *file_name, const uint16_t js, const uint16_t is, const float * const * data, const char *head, const char *caption)
{
	char buffer[64];
	std::unique_ptr<std::FILE, int(*)(std::FILE*)> fo(std::fopen(file_name, "w"), std::fclose);
	if (!fo)
		return 1;
	fprintf(&*fo, "%s\n", head);
	fprintf(&*fo, "%s\n", caption);
	for (int j = 0; j < js; j++)
	{
		for (int i = 0; i < is; i++)
		{		
			sprintf(buffer, "%0.10f", data[j][i]);
			del_str_txtzeroes_inplace(buffer);
			fprintf(&*fo, "X%iZ%i=%s\n", i + 1, j + 1, buffer);
		}
	}
	return 0;
}
