
#include "stdafx.h"

#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <vector>


int uoz_by_rpm_press_from_rpm_gbc(const int RPMS[16], const int PRESS[16], const int data_count, const float *dRPM, const float *dPRESS, const float *dGBC);

//main.cpp
int get_current_rt(const int *data, const int count, const int value, float *distance);



int logs_statistic()
{
	HANDLE hf;
	WIN32_FIND_DATAA FindFileData;
	vector< string > file_names;
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

	int RPMS[16], THRTS[16], PRESS[16];
	setlocale(LC_ALL, "en");

	float table_data[16][16];
	int table_data_count[16][16];

	table_init<float, 16, 16>(table_data, 0);
	table_init<int, 16, 16>(table_data_count, 0);

	float **logs_data_ptr;
	char **logs_params_ptr;
	int params_count, data_count;

	for (size_t i = 0; i < file_names.size(); i++)
	{
		read_logs_csv( (string("logs\\") + file_names[i]).c_str(), &logs_data_ptr, &logs_params_ptr, &params_count, &data_count);

		const float *dRPM = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "RPM", "обороты двс");
		const float *dPRESS = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "PRESS", "PRESSURE J7ES");
		const float *dGBC = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "GBC", "Цикловой расход воздуха");
		const float *dTWAT = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "TWAT", "температура ОЖ");

		for (int t = 0; t < data_count; t++)
		{
			if (dTWAT[t] < 70.)//двигатель не прогрет
				continue;
			if (dRPM[t] < 0.9*RPMS[0])//обороты ниже рабочих
				continue;
			if (dPRESS[t] * 10.0 < 0.9*PRESS[0])//Давление слижком низкое
				continue;
			int i = get_current_rt(PRESS, 16, 10.0*dPRESS[t], 0);
			int j = get_current_rt(RPMS, 16, dRPM[t], 0);
			table_data[i][j] += dGBC[t];
			table_data_count[i][j]++;
		}

		free_log_params(logs_params_ptr, params_count);
		free_log_data(logs_data_ptr, params_count);
	}

	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if(table_data_count[i][j])
				table_data[i][j] /= float(table_data_count[i][j]);
		}
	}
	char cte_caption[256], cte_head[256];
	strcpy(cte_head, "[279E7F4F]");
	strcpy(cte_caption, "Name = БЦН по давлению");
	write_cte_data("bcn_by_pressure.cte", table_data, cte_head, cte_caption);
	return 0;
}

int _2main()
{
	logs_statistic();
	return 0;
}


int uoz_by_rpm_press_from_rpm_gbc(const int RPMS[16], const int PRESS[16], const int data_count, const float *dRPM, const float *dPRESS, const float *dGBC)
{
	float RPM_PRESS_UOZ_table[16][16] = { 0 };
	float RPM_PRESS_GBC_table[16][16] = { 0 };
	int RPM_PRESS_GBC_count[16][16] = { 0 };

	setlocale(LC_ALL, "ru");
	char cte_caption[256], cte_head[256];
	float uoz_table[16][16], uoz_table2[16][16];
	int rpm205do[16] = { 600, 720, 840, 990, 1170, 1380, 1650, 1950, 2310, 2730, 3210, 3840, 4530, 5370, 6360, 7650 };
	int rpmj7esMy[16] = { 800, 1000, 1200, 1600, 2000, 2400, 2800, 3200, 3720, 4520, 4800, 5280, 5760, 6200, 6640, 10200 };
	int gbc205do[16] = { 29, 57, 85, 113, 141, 169, 197, 225, 253, 281, 309, 337, 365, 393, 421, 449 };
	read_cte_data("uoz_205do54.cte", uoz_table, cte_head, cte_caption);

	change_rpm16(rpm205do, rpmj7esMy, uoz_table, uoz_table2);
	write_cte_data("uoz_2.cte", uoz_table2, cte_head, cte_caption);
	get_table_value16(uoz_table2, rpmj7esMy, gbc205do, 1200, 29);
	for (int k = 0; k < data_count; k++)
	{
		int i = get_current_rt(PRESS, 16, 10.0*dPRESS[k], 0);
		int j = get_current_rt(RPMS, 16, dRPM[k], 0);
		RPM_PRESS_GBC_table[i][j] += dGBC[k];
		RPM_PRESS_GBC_count[i][j]++;
	}

	FILE *fo2 = fopen("gbc_press.txt", "w");
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			fprintf(fo2, "%0.3f\t", RPM_PRESS_GBC_count[i][j] ? RPM_PRESS_GBC_table[i][j] / fabs(RPM_PRESS_GBC_count[i][j]) : 1.0);
		}
		fprintf(fo2, "\n");
	}
	fclose(fo2);

	float gbc, uoz;
	int ii;
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (RPM_PRESS_GBC_count[i][j] != 0 || i >= 14)
			{
				gbc = RPM_PRESS_GBC_table[i][j] / fabs(RPM_PRESS_GBC_count[i][j]);
				if (i >= 14)
					gbc = 400;
				//jj = get_current_rt(rpm205do, 16, RPMS[j], 0);
				ii = get_current_rt(gbc205do, 16, gbc, 0);
				uoz = find_new_value(RPMS[j], rpm205do, uoz_table[ii]);
				//uoz = uoz_table[ii][jj];
				RPM_PRESS_UOZ_table[i][j] = uoz;
			}
		}
	}

	strcpy(cte_head, "[9F8A59E4]");
	strcpy(cte_caption, "Name = УОЗ от давления");
	write_cte_data("uoz_press_b.cte", RPM_PRESS_UOZ_table, cte_head, cte_caption);
	return 0;
}

/*
int rpm_stock[16] = { 600, 720, 840, 990, 1170, 1380, 1650, 1950, 2310, 2730, 3210, 3840, 4530, 5370, 6360, 7650 };

float stockuoztab[16][16] = { 0 };
float newuoz[16][16] = { 0 };
char head[256], capt[256];
read_cte_data("1.6_16v_21126.cte", stockuoztab, head, capt);
change_rpm16(rpm_stock, rpmj7es16, stockuoztab, newuoz);
write_cte_data("newuoz.cte", newuoz, head, capt);
return 0;*/
/*
gbc_by_rpm_press(RPMS, PRESS, data_count, dRPM, dPRESS, dGBC);
return 0;*/

//pcn_by_press_from_thrt(data_count, THRTS, RPMS, PRESS, dTHR, dRPM, dPRESS);
/*int c[] = {0, 1, 2, 3, 4};
int n = THRTS[find_near(THRTS, 16, 101)];*/

/*
float table_pcn_coeff[16][16];
int table_pcn_count[16][16];
for(int i = 0; i < 16; i++)
{
for(int j = 0; j < 16; j++)
{
table_pcn_coeff[i][j] = 0;
table_pcn_count[i][j] = 0;
}
}*/