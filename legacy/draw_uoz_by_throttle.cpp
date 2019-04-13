
// ConsoleApplication1.cpp: определяет точку входа для консольного приложения.
//
//#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <clocale>

/*
//cte-file
*/
int read_cte_data(const char *file_name, float DATA[16][16], char *input_src_head, char *input_src_caption);
int write_cte_data(const char *file_name, const float DATA[16][16], const char *input_src_head, const char *input_src_caption);

float find_new_value(const int rpm_dst, const int RPM_src[16], const float DATA_src[16]);//get_table_value


int read_config(const char *file_name, int RPM_src[16], int RPM_dst[16])
{
	FILE *f_cfg;
	try
	{
		f_cfg = fopen(file_name, "r");
		for (int i = 0; i < 16; i++)
		{
			if (fscanf(f_cfg, "%i", RPM_src + i) == EOF)
				throw 1;
		}
		for (int i = 0; i < 16; i++)
		{
			if (fscanf(f_cfg, "%i", RPM_dst + i) == EOF)
				throw 2;
		}
		fclose(f_cfg);
	}
	catch (int err)
	{
		printf("error in RPM.txt file");
		if (err == 1)
			printf(" with src RPM, count do not match\n");
		if (err == 2)
			printf(" with dst RPM, count do not match\n");
		fclose(f_cfg);
		return 1;
	}
	return 0;
}

void draw_uoz_by_throttle(const char *bcn_file_name, const char *uoz_file_name, const float DATA_uoz[16][16]);

void draw_uoz_by_throttle(const char *bcn_file_name, const char *bcn_uoz_file_name, const float DATA_uoz[16][16])
{
	float DATA_bcn[16][16], DATA_Tuoz[16][16];
	int uoz_index[16] = { 29, 57, 85, 113, 141, 169, 197, 225, 253, 281, 309, 337, 365, 393, 421, 449 };
	const char *input_src_head = "[3A4D9EF3]";
	const char *input_src_caption = "Name=УОЗ от дросселя";

	if (read_cte_data(bcn_file_name, DATA_bcn, 0, 0) < 256)
	{
		printf("\n************************\nsome thing wrong with input file\n************************\n");
	}

	for (int j = 0; j < 16; j++)
	{
		for (int i = 0; i < 16; i++)
		{
			for (int k = 1; k < 17; k++)
			{
				if (k == 16)//не нашли, берем наибольший
				{
					DATA_Tuoz[j][i] = DATA_uoz[15][i];
				}
				else
					if (DATA_bcn[j][i] <= uoz_index[k])
					{
						if ((DATA_bcn[j][i] - uoz_index[k - 1]) < (DATA_bcn[j][i] - uoz_index[k]))//проверим кто ближе, правая точка или левая
							k--;
						DATA_Tuoz[j][i] = DATA_uoz[k][i];
						break;
					}
			}
		}
	}
	write_cte_data(bcn_uoz_file_name, DATA_Tuoz, input_src_head, input_src_caption);
}


int draw_uoz_by_throttle_main()
{
	float DATA_src[16][16], DATA_dst[16][16];
	int RPM_src[16], RPM_dst[16];

	char input_src_head[128], input_src_caption[256];

	setlocale(LC_ALL, "");

	if (read_config("RPM_uoz_by_throttle.txt", RPM_src, RPM_dst) != 0)
		return 1;

	if (read_cte_data("input.cte", DATA_src, input_src_head, input_src_caption) < 256)
	{
		printf("\n************************\nsome thing wrong with input file\n************************\n");
		return 2;
	}

	for (int j = 0; j < 16; j++)
	{
		for (int i = 0; i < 16; i++)
		{
			DATA_dst[j][i] = find_new_value(RPM_dst[i], RPM_src, DATA_src[j]);
		}
	}

	write_cte_data("output.txt", DATA_dst, input_src_head, input_src_caption);

	draw_uoz_by_throttle("bcn.cte", "uoz.cte", DATA_dst);
	return 0;
}

