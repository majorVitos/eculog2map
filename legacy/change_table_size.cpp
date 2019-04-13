


#include "cte-file.h"

#include <clocale>
#include <cstring>


/*
	Функция линейного интерполирования по известным точкам
*/
float math_line_function(const float x0, const float x1, const float y0, const float y1, const float x)
{
	float a, b;
	a = (y1 - y0) / (x1 - x0);
	b = y0 - a*x0;
	return  a*x + b;
}

/*
	Функция построения таблицы увеличенной размерности и интерполяции несуществующих точек
*/
int table32from16(const int X16[16], const int X32[32], const int Z16[16], const int Z32[32], const float table16[16][16], float table32[32][32])
{
	//Проход по существующим элементам
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (X16[i] != X32[i << 1] || Z16[j] != Z32[j << 1])
			{
				return 1;
			}
			table32[i << 1][j << 1] = table16[i][j];
			continue;
		}
	}
	//Проход по существующей z и несуществующей x
	for (int i = 0; i < 32; i++)
	{
		for (int j = 0; j < 31; j++)
		{
			if ( ((i & 1) == 0) & ((j & 1) == 1))
			{
				table32[i][j] = math_line_function(X32[j - 1], X32[j + 1], table32[i][j - 1], table32[i][j + 1], X32[j]);
			}
		}
		table32[i][31] = math_line_function(X32[29], X32[30], table32[i][29], table32[i][30], X32[31]);//захардкожено!!!
	}


	//Проход по несуществующей z (нечетный индекс)
	for(int i = 0; i < 31; i++)
	{
		for(int j = 0; j < 32; j++)
		{
			if ((i & 1) == 1)
			{
				table32[i][j] = math_line_function(Z32[i - 1], Z32[i + 1], table32[i - 1][j], table32[i + 1][j], Z32[i]);
			}
		}

	}
	for (int j = 0; j < 32; j++)//Захардкожено!!!
	{
		table32[31][j] = math_line_function(Z32[29], Z32[30], table32[29][j], table32[30][j], Z32[31]);
	}
	return 0;
}

int table16from32(const float table32[32][32], float table16[16][16])
{
	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 16; j++)
		{
			table16[i][j] = table32[i << 1][j << 1];
		}
	}
	return 0;
}


/*
int change_pcn_press_table16to32()
{

	char cte_caption[256], cte_head[256];

	int rpmj7es16[16] = { 800, 1000, 1200, 1600, 2000, 2400, 2800, 3200, 3720, 4520, 4800, 5280, 5760, 6200, 6640, 10200 };
	int rpmj7es32[32] = { 800, 920, 1000, 1120, 1200, 1400, 1600, 1800, 2000, 2240, 2400, 2640, 2800, 3040, 3200, 3400,
		3720, 4120, 4520, 4680, 4800, 5000, 5280, 5520, 5760, 6000, 6200, 6520,  6640, 6960, 10200, 10201 };
	int presj7es16[16] = { 200,	254,	308,	362,	416,	470,	524,	578,	632,	686,	740,	794,	848,	902,	956,	1010 };
	int presj7es32[32] = { 200,	227,	254,	281,	308,	335,	362,	389,	416,	443,	470,	497,	524,	551,	578,	605,
		632,	659,	686,	713,	740,	767,	794,	821,	848,	875,	902,	929,	956,	983,	1010,	1037 };

	setlocale(LC_ALL, "ru");
	float table_pcn_press16[16][16];
	float table_pcn_press32[32][32];
	read_cte_data("pcn_by_press.cte", table_pcn_press16, cte_head, cte_caption);

	table32from16(rpmj7es16, rpmj7es32, presj7es16, presj7es32, table_pcn_press16, table_pcn_press32);

	strcpy(cte_head, "[7BF20B0E]");
	strcpy(cte_caption, "Name = Поправка ЦН 32х32 по давлению");
	write_cte_data32("new_pcn_press32!!!.cte", table_pcn_press32, cte_head, cte_caption);

	return 0;
}
*/

int change_pcn_press_table32to16()
{

	char cte_caption[256], cte_head[256];

	/*int rpmj7es16[16] = { 800, 1000, 1200, 1600, 2000, 2400, 2800, 3200, 3720, 4520, 4800, 5280, 5760, 6200, 6640, 10200 };
	int rpmj7es32[32] = { 800, 920, 1000, 1120, 1200, 1400, 1600, 1800, 2000, 2240, 2400, 2640, 2800, 3040, 3200, 3400,
		3720, 4120, 4520, 4680, 4800, 5000, 5280, 5520, 5760, 6000, 6200, 6520,  6640, 6960, 10200, 10201 };
	int presj7es16[16] = { 200,	254,	308,	362,	416,	470,	524,	578,	632,	686,	740,	794,	848,	902,	956,	1010 };
	int presj7es32[32] = { 200,	227,	254,	281,	308,	335,	362,	389,	416,	443,	470,	497,	524,	551,	578,	605,
		632,	659,	686,	713,	740,	767,	794,	821,	848,	875,	902,	929,	956,	983,	1010,	1037 };
*/
	setlocale(LC_ALL, "ru");
	float table_pcn_press16[16][16];
	float table_pcn_press32[32][32];
	read_cte_data32("pcn_by_press.cte", table_pcn_press32, cte_head, cte_caption);

	table16from32(table_pcn_press32, table_pcn_press16);
	
	strcpy(cte_head, "[7BF20B0D]");
	strcpy(cte_caption, "Name=Поправка ЦН по давлению");
	write_cte_data("new_pcn_press16!!!.cte", table_pcn_press16, cte_head, cte_caption);

	return 0;
}