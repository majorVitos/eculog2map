

#if 0
/*
получаем поправку цн по давлению от среднего давления по дросселю и поправке по дросселю, каламбурная идея, каламбурный результат
*/
void pcn_by_press_from_thrt(const int data_count, const int THRTS[16], const int RPMS[16], const int PRESS[16],
	const float *dTHR, const float *dRPM, const float *dPRESS)
{

	float RPM_THRT_PRES_min[16][16];
	float RPM_THRT_PRES_max[16][16];
	float RPM_THRT_PRES_avg[16][16];
	int RPM_THRT_PRES_cnt[16][16];

	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			RPM_THRT_PRES_min[i][j] = 1000;
			RPM_THRT_PRES_max[i][j] = 0;
			RPM_THRT_PRES_avg[i][j] = 0;
			RPM_THRT_PRES_cnt[i][j] = 0;
		}
	}

	for (int k = 0; k < data_count; k++)
	{
		int i = get_current_rt(THRTS, 16, dTHR[k], 0);//find_near(THRTS, 16, dTHR[k]);
		int j = get_current_rt(RPMS, 16, dRPM[k], 0);//find_near(RPMS, 16, dRPM[k]);
		if (RPM_THRT_PRES_min[i][j] > dPRESS[k])
			RPM_THRT_PRES_min[i][j] = dPRESS[k];
		if (RPM_THRT_PRES_max[i][j] < dPRESS[k])
			RPM_THRT_PRES_max[i][j] = dPRESS[k];
		RPM_THRT_PRES_avg[i][j] += dPRESS[k];
		RPM_THRT_PRES_cnt[i][j]++;
	}

	setlocale(LC_ALL, "ru");
	char cte_caption[256], cte_head[256];
	float pcn_table[16][16];
	read_cte_data("pcn_thr_rpm.cte", pcn_table, cte_head, cte_caption);

	float pcn_pres[16][16];
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			pcn_pres[i][j] = 2;
		}
	}

	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (RPM_THRT_PRES_cnt[i][j])
			{
				float cur_pres = RPM_THRT_PRES_avg[i][j] / float(RPM_THRT_PRES_cnt[i][j]);
				int k = get_current_rt(PRESS, 16, cur_pres*10., 0);//find_near(PRESS, 16, cur_pres*10.);
				pcn_pres[k][j] = pcn_table[i][j];
			}
		}
	}

	write_cte_data("pcn_press_rpm.cte", pcn_pres, "[7BF20B0D]", "Name=Поправка ЦН по давлению");
}
#endif