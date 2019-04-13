
/*
хоть бы написал что это
*/
float get_table_value16(const float table[16][16], const int *x_table, const int *z_table, const float x, const float z)
{
	int i, j;
	float a0, b0, a1, b1, y0, y1, a2, b2, ret;
	for (i = 1; i < 16; i++)
	{
		if (z <= z_table[i])
			break;
	}
	if (i == 16)
		i--;
	for (j = 1; j < 16; j++)
	{
		if (x <= x_table[j])
			break;
	}
	if (j == 16)
		j--;
	//i,j - координаты сектора

	//нижняя кривая
	a0 = float(table[i - 1][j - 1] - table[i - 1][j]) / float(x_table[j - 1] - x_table[j]);
	b0 = float(table[i - 1][j]) - a0*x_table[j];
	y0 = a0*x + b0;
	//
	a1 = float(table[i][j - 1] - table[i - 1][j]) / float(x_table[j - 1] - x_table[j]);
	b1 = float(table[i][j]) - a0*x_table[j];
	y1 = a1*x + b1;
	
	a2 = (y1 - y0) / float(z_table[i] - z_table[i - 1]);
	b2 = y0 - a2*z_table[i];
	ret = a2*z + b2;
	return ret;
}


/*
пока побудет тут
*/
float find_new_value(const int rpm_dst, const int RPM_src[16], const float DATA_src[16])
{
	float a, b;
	for (int i = 1; i < 17; i++)
	{
		if (i == 16)//нет в интервале, экстраполируем
		{
			a = float(DATA_src[14] - DATA_src[15]) / float(RPM_src[14] - RPM_src[15]);
			b = float(DATA_src[15]) - a*RPM_src[15];
			break;
		}
		else
			if (rpm_dst <= RPM_src[i])
			{
				a = float(DATA_src[i - 1] - DATA_src[i]) / float(RPM_src[i - 1] - RPM_src[i]);
				b = float(DATA_src[i]) - a*RPM_src[i];
				break;
			}
	}
	return a*float(rpm_dst) + b;
}