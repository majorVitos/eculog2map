


#include "stdafx.h"

/*
table_type - reserved (пока не используется, в связи с наличием 2д массивов)
rpms_src - массив оборотов исходной таблицы, формат {600, 720, 840, 990, ...}
rpms_dst - массив оборотов генерируемой таблицы, формат тотже {800, 1000, 1200, ...}
src - исходная таблица
dst - генерируемая, память должна быть выделена
*/
void change_rpm(const TABLE_TYPE table_type, const int *rpms_src, const int *rpms_dst, const float * const * const src, float **dst)
{
	int isize, jszie;//размер фактора y (например цикловое наполнение), и аргумента x (например обороты)
	float a, b;
	PARAM_SIZE_BY_TABLE(table_type, isize, jszie);
	for (int i = 0; i < isize; i++)
	{
		for (int j = 0, k = 1; j < jszie; j++)
		{
			for (; k < jszie; k++)//поиск интервала [k-1;k] оборотов
			{
				if (rpms_dst[j] <= rpms_src[k])
					break;
			}
			if (k == jszie)//уперлись в конец массива и не нашли, используем последнюю
				k--;
			a = (src[i][k - 1] - src[i][k]) / float(rpms_src[k - 1] - rpms_src[k]);//линейное интерполирование
			b = src[i][k] - a*rpms_src[k];
			dst[i][j] = a*float(rpms_dst[j]) + b;
		}
	}
}

/*
функция заглушка, пока интерфейс не рефакторенный
*/
void change_rpm16(const int *rpms_src, const int *rpms_dst, const float src[16][16], float dst[16][16])
{
	float **srcp, **dstp;
	srcp = new float*[16];
	dstp = new float*[16];
	for (int i = 0; i < 16; i++)
	{
		srcp[i] = new float[16];
		dstp[i] = new float[16];
		for (int j = 0; j < 16; j++)
		{
			for (int j = 0; j < 16; j++)
				srcp[i][j] = src[i][j];
		}
	}
	change_rpm(TBT_16x16, rpms_src, rpms_dst, srcp, dstp);
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			dst[i][j] = dstp[i][j];
		}
		delete[] dstp[i];
		delete[] srcp[i];
	}
	delete[] dstp;
	delete[] srcp;
}