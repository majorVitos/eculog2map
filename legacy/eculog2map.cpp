

#include "stdafx.h"

#include "config.h"

#include <algorithm>
#include <vector>

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>





int find_binary(const int *data, const int count, const int value)
{
	int a = 0, b = count - 1, k;
	if(!count)
		return -1;
	while(a < b)
	{
		k = (a + b) >> 1;
		if(data[k] >= value)
			b = k;
		else 
			a = k + 1;
	}
	return a;
}

/*
поиск номера рабочей точкив (ее индекса) в списке значений data, размера count
по значению value, возвращает индекс рт, и расстояние distance до номинального значения рабочей точки
растояние относительное, где 0 - полное попадание в рт, 1 - точка по средине двух рабочих точек, 2 - сильно удаленная точка вне диапазона
*/
int get_current_rt(const int *data, const int count, const int value, float *distance)
{
	int ra, rb, rt;
	float dist;
	rt = -1;
	for(int i = 1; i < count; i++)
	{
		if(value <= data[i])
		{
			ra = value - data[i - 1];
			rb = data[i - 0] - value;
			dist = 2.0 / float(data[i] - data[i - 1]);
			if (ra < rb || ra < 0)
			{
				rt = i - 1;
				dist *= fabs(ra);
			}
			else
			{
				rt = i;
				dist *= float(rb);
			}
			break;
		}
	}
	if(rt < 0)//если вышли из диапазона и не нашли рабочую точку, то будет выбрана последняя
	{
		rt = count - 1;
		dist = 2.0 * (value - data[count - 1]) / float(data[count - 1] - data[count - 2]);//по размерности последнего диапазона
	}
	if (distance != 0)
		*distance = dist;
	return rt;
}



/*
отклонение по коэффициенту, если всреднем, то вероятно, достигнута стационарность
в дальнейшем не используется, как низкая эффективность
*/
#define DEVIATION_SIZE 4
float xi[DEVIATION_SIZE+1];
float calc_deviation(float *_mean, const float val, int count)
{
	float &mean = *_mean;
	float dev;
	mean = dev = 0.0;
	if(count > DEVIATION_SIZE)
		count = DEVIATION_SIZE;
	xi[count] = val;
	for(int i = 0; i < count; i++)
	{
		xi[i] = xi[i + 1];
		mean += xi[i];
	}
	mean = mean / float(count);
	for(int i = 0; i < count; i++)
	{
		dev += (xi[i] - mean)*(xi[i] - mean);
	}
	dev = sqrtf(dev / float(count));
	return dev;
}

/*
признак стационарностьи по состоянию ДК, так же возвращает d(COEFF)/dt
*/
#define FLAM_SIZE 12
int flam_buffer[FLAM_SIZE+1];
int flam_stat(const int val, int *dir)
{
	static int cnt = 0;
	int res = 0;
	int prev;
	*dir = 1;
	int up, down;
	up = 1;
	down = 1;
	if(cnt < FLAM_SIZE)
		cnt++;
	flam_buffer[cnt] = val;
	prev = flam_buffer[0];
	for(int i = 0; i < cnt; i++)
	{
		if(prev != flam_buffer[i + 1])
			res++;
		prev = flam_buffer[i] = flam_buffer[i + 1];
		if(i > cnt >> 2)
		{
			down = down & flam_buffer[i];
			up = up & !flam_buffer[i];
		}
	}
	if(up)
		*dir = 1;
	else if(down)
		*dir = -1;
	else
		*dir = 0;
	return res;
}

/*
addition by adc of narrow oxygen sensor
*/
float narrowWide(float value)
{
	if (value < 0.1)
		return 1.07;

	if (value > 0.8)
	{
		return 0.95;
	}
	return 1;
}

struct pointoflearn
{
	float rpm, factor, coef, adc_lam, rpmrt_dist, factorrt_dist;
	int learn;
	pointoflearn(float _rpm, float _factor, float _coef, float _adc_lam, float _rpmrt_dist, float _factorrt_dist, float _trt)
		: rpm(_rpm), factor(_factor), coef(_coef), adc_lam(_adc_lam), rpmrt_dist(_rpmrt_dist), factorrt_dist(_factorrt_dist), learn(0)
	{}
};

std::vector<std::string> get_all_logs_filenames();

int change_pcn_press_table32to16();


const int table_size = 16;

bool power_factor_throttle = false;

int main(int argc, char* argv[])
{
	int RPMS[16], THRTS[16], PRESS[16];
	float **logs_data_ptr;
	char **logs_params_ptr;
	int params_count, data_count;

	char cte_caption[256], cte_head[256];

	int config_num;

	std::vector<int> RPM_Quantization;
	std::vector<int> Factor_Quantization;
	std::vector<int> Throttle_Quantization;
	
	
	tsconfig cfg1("test.cfg");
	cfg1.get_int("RPM_es" + std::to_string(table_size), RPM_Quantization);
	cfg1.get_int("Press_quant" + std::to_string(table_size), Factor_Quantization);
	cfg1.get_int("Throttle_quant", Throttle_Quantization);

	
	std::vector<pointoflearn> table_core[table_size][table_size];

	//тут что то нужно доработать
	float table_MAP_failure[table_size][table_size];
	int table_MAP_failure_cnt[table_size][table_size];
	table_init<float, table_size, table_size>(table_MAP_failure, 0);
	table_init<int, table_size, table_size>(table_MAP_failure_cnt, 0);

	//float table_pcn_exp[table_size][table_size];
	//int table_pcn_exp_count[table_size][table_size];
	float table_gbc[32][32];
	int table_gbc_count[32][32];

	//table_init<float, table_size, table_size>(table_pcn_exp, 0);
	//table_init<int, table_size, table_size>(table_pcn_exp_count, 0);

	setlocale(LC_ALL, "en");
	

	std::vector<std::string> file_names = get_all_logs_filenames();

	for (size_t fi = 0; fi < file_names.size(); fi++)
	{
		read_logs_csv((std::string("logs\\") + file_names[fi]).c_str(), &logs_data_ptr, &logs_params_ptr, &params_count, &data_count);

		const float *dTRT = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 3, "TRT", "THR", "Дроссель");
		const float *dRPM = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "RPM", "обороты двс");
		const float *dfLAM = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "fLAM", "текущее состояние ДК");
		const float *dCOEFF = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "COEFF", "коэфицент коррекции времени впрыска");
		const float *dPRESS = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "PRESS", "PRESSURE J7ES");
		const float *dLAMREG = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "fLAMREG", "флаг зоны регулирования по дк");
		const float *dTWAT = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "TWAT", "температура ОЖ");
		const float *dGBC = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "GBC", "Цикловой расход воздуха");
		const float *dADC_LAM = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "ADCLAM", "АЦП ДК");

		const float *dAFR = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 1, "состав смеси");
		const float *dLC = get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 1, "AFR_LC");

		if (!dTRT || !dRPM || !dfLAM || !dCOEFF || !dPRESS || !dLAMREG || !dTWAT || !dGBC)
		{
			printf("log file corrupted!\n");
			return 1;
		}

		int cur_factor_rt;
		int cur_thrt_rt, cur_press_rt;
		int cur_rpm_rt, prev_rpm_rt = 0;
		int prev_factor_rt = 0;

		int fLAM, flam_p, cur_flam;
		flam_p = 0;
		
		fLAM = 0;


		float mean;
		float deviation;
		int stationary_count_rpm_pf = 0;
		int stationary_lam_count = 0;
		int delay_rpm[4] = { 0, 0, 0, 0 };
		int delay_press[4] = { 0 };
		int delay_num = 0;

		float coef_delta_prevalue = 0;//прудыдущее значение, чтобы не делать t-1
		float forecast_coeff = 1; //коэффициент коррекции в точке нестационарности, предсказание поправки

		float rpmrt_dist, trt_dist, pressrt_dist;



		int stationary_rpm;				//Стационарность по оборотам 
		int stationary_rpm_count = 0;
		int stationary_pf;				//Стационарность по дросселю/давлению
		int stationary_pf_count = 0;
#if 1
		for (int t = 0; t < data_count; t++)
		{
			if (dLAMREG[t] < 1)//не в зоне регулирования
				continue;
			if (dTWAT[t] < 80)//двигатель не прогрет
				continue;
			if (dRPM[t] < 0.9*RPMS[0])//обороты ниже рабочих
				continue;
			
			if (!power_factor_throttle)
			{
				if (dPRESS[t] * 10.0 < 0.9*PRESS[0])//Давление слижком низкое
					continue;
			}




			float coef_delta = dfLAM[t] - coef_delta_prevalue;
			coef_delta_prevalue = dfLAM[t];

			float coef_mean, coef_deviation;
			coef_deviation = calc_deviation(&coef_mean, dCOEFF[t], 4);



			cur_rpm_rt =	get_current_rt(RPM_Quantization.data(),		RPM_Quantization.size(), dRPM[t], &rpmrt_dist);
			cur_press_rt =	get_current_rt(Factor_Quantization.data(),	Factor_Quantization.size(), 10 * dPRESS[t], &pressrt_dist);
			cur_thrt_rt =	get_current_rt(Throttle_Quantization.data(),Throttle_Quantization.size(), dTRT[t], &trt_dist);

			float factor_dist;
			if (power_factor_throttle)
			{
				cur_factor_rt = cur_thrt_rt;
				factor_dist = trt_dist;
			}
			else
			{
				cur_factor_rt = cur_press_rt;
				factor_dist = pressrt_dist;
			}


			//int stationary_pf_strong;

			stationary_rpm = abs(cur_rpm_rt - prev_rpm_rt) < 1;
			stationary_pf = abs(cur_factor_rt - prev_factor_rt) < 2;
			
			if(stationary_rpm)
				stationary_rpm_count++;
			else
				stationary_rpm_count = 0;

			if (stationary_pf)
				stationary_pf_count++;
			else
				stationary_pf_count = 0;

			if (stationary_rpm && stationary_pf)
			{
				//видимо стационарность по лямбдеfabs(coef_delta) < 0.04 ||
				if (coef_deviation < 0.05)//(flam > 2 && direction == 0)
				{
					stationary_lam_count++;
				}
				else
					stationary_lam_count = 0;
			}

			if (stationary_rpm_count > 2 && stationary_pf_count > 2 && stationary_lam_count > 2)
			{
				//
				/*попробуем учесть направление, если коэф. увеличивается, но при этом он меньше 1
				то скорее всего предыдущая режимная точка плохо повлияла на коэф. кор. и т.д
				*/
				if (coef_delta >= 0 && dCOEFF[t] > 1 || coef_delta <= 0 && dCOEFF[t] < 1)
				{/*
				 table_pcn_exp[cur_press_rt][cur_rpm_rt] += dCOEFF[t] * narrowWide(dADC_LAM[t]);
				 table_pcn_exp_count[cur_press_rt][cur_rpm_rt]++;*/
					table_core[cur_factor_rt][cur_rpm_rt].push_back(
						pointoflearn(dRPM[t], dTRT[t], dCOEFF[t] * narrowWide(dADC_LAM[t]), dADC_LAM[t], rpmrt_dist,
							factor_dist, cur_thrt_rt));
					/*table_core[cur_factor_rt][cur_rpm_rt].push_back(
						pointoflearn(dRPM[t], dPRESS[t], dCOEFF[t] * narrowWide(dADC_LAM[t]), dADC_LAM[t], rpmrt_dist, 
							pressrt_dist, cur_thrt_rt));*/
				}
			}
#if 0
			if (stationary_pf)
			{
				stationary_count_rpm_pf++;
				//видимо стационарность по лямбдеfabs(coef_delta) < 0.04 ||
				if (coef_deviation < 0.05)//(flam > 2 && direction == 0)
				{
					stationary_count_lam++;
					if (stationary_count_lam > 2 && stationary_count_rpm_pf > 2)
					{
						/*
						if (table_pcn_exp_count[cur_press_rt][cur_rpm_rt] < 0)
						{
							table_pcn_exp[cur_press_rt][cur_rpm_rt] = 0;
							table_pcn_exp_count[cur_press_rt][cur_rpm_rt] = 0;
						}
						/*попробуем учесть направление, если коэф. увеличивается, но при этом он меньше 1
							то скорее всего предыдущая режимная точка плохо повлияла на коэф. кор. и т.д
						*/
						if (coef_delta >= 0 && dCOEFF[t] > 1 || coef_delta <= 0 && dCOEFF[t] < 1)
						{/*
							table_pcn_exp[cur_press_rt][cur_rpm_rt] += dCOEFF[t] * narrowWide(dADC_LAM[t]);
							table_pcn_exp_count[cur_press_rt][cur_rpm_rt]++;*/
							table_core[cur_factor_rt][cur_rpm_rt].push_back(
								pointoflearn(dRPM[t], dPRESS[t], dCOEFF[t] * narrowWide(dADC_LAM[t]), dADC_LAM[t], rpmrt_dist, pressrt_dist));
						}


						if (stationary_count_lam > 1 && fabs(RPMS[cur_thrt_rt] - dRPM[t]) < RPMS[cur_thrt_rt])//расстояние 
						{
							table_gbc[cur_thrt_rt][cur_rpm_rt] += dGBC[t] * dCOEFF[t];
							table_gbc_count[cur_thrt_rt][cur_rpm_rt]++;
						}
					}
				}
				else
				{
					stationary_count_lam = 0;

					if (stationary_count_rpm_pf > 2 &&
						((dCOEFF[t] > 1.0) || (dCOEFF[t] < 1.0)) && table_pcn_exp_count[prev_press_rt][prev_rpm_rt] <= 0)//direction -> delta
					{
						forecast_coeff = dCOEFF[t] * narrowWide(dADC_LAM[t]);
						//зашкаливший коэф нужно тоже вносить, пока нет
						/*table_pcn_exp [cur_press_rt][cur_rpm_rt] += dCOEFF[t];
						table_pcn_exp_count[cur_press_rt][cur_rpm_rt]--;*/
						/*if (table_pcn_exp_count[cur_press_rt][cur_rpm_rt] == 0 || (table_pcn_exp_count[cur_press_rt][cur_rpm_rt] <= 0 &&
							(table_pcn_exp[cur_press_rt][cur_rpm_rt] > 1 && table_pcn_exp[cur_press_rt][cur_rpm_rt] < dCOEFF[t]) ||
							(table_pcn_exp[cur_press_rt][cur_rpm_rt] < 1 && table_pcn_exp[cur_press_rt][cur_rpm_rt] > dCOEFF[t])))
						{
							table_pcn_exp[cur_press_rt][cur_rpm_rt] = dCOEFF[t];
							table_pcn_exp_count[cur_press_rt][cur_rpm_rt] = -1;
						}*/
					}
				}
			}
			else
				stationary_count_rpm_pf = 0;
#endif

#if 0
			if (!stationary_pf_strong)
			{
				/*
					Если точка не обучена, попробуем предсказать
				*/
				if (table_pcn_exp_count[prev_press_rt][prev_rpm_rt] <= 0 && forecast_coeff != 1 && (THRTS[prev_rpm_rt] - dTRT[t - 1])*(THRTS[prev_rpm_rt] - dTRT[t - 1]) < 1000)
				{
					table_pcn_exp[prev_press_rt][prev_rpm_rt] += forecast_coeff;
					table_pcn_exp_count[prev_press_rt][prev_rpm_rt]--;
					/*
					if (forecast_coeff > 1)
						table_pcn_exp[prev_press_rt][prev_rpm_rt] = std::max(table_pcn_exp[prev_press_rt][prev_rpm_rt], forecast_coeff);
					else
						table_pcn_exp[prev_press_rt][prev_rpm_rt] = std::min(table_pcn_exp[prev_press_rt][prev_rpm_rt], forecast_coeff);
					table_pcn_exp_count[prev_press_rt][prev_rpm_rt] = -1;
					forecast_coeff = 1;*/
				}
			}
#endif
			table_MAP_failure[cur_thrt_rt][cur_rpm_rt] += dPRESS[t];
			table_MAP_failure_cnt[cur_thrt_rt][cur_rpm_rt]++;

			prev_rpm_rt = cur_rpm_rt;
			prev_factor_rt = cur_factor_rt;
		}
#else

		int stationary_pf;		//Стационарность дросселю/давлению
		int stationary_rpm;		// по оборотам
		int stationary_pf_prev = 0;
		int stationary_count;
		for (int t = 0; t < data_count; t++)
		{
			if (dTWAT[t] < 70)//двигатель не прогрет
				continue;
			if (dRPM[t] < 0.9*RPMS[0])//обороты ниже рабочих
				continue;
			if (dPRESS[t] * 10.0 < 0.9*PRESS[0])//Давление слижком низкое
				continue;
			if (dLC[t] == 0)
				continue;
			cur_rpm_rt = get_current_rt(RPM_Quantization.data(), RPM_Quantization.size(), dRPM[t], &rpmrt_dist);
			cur_press_rt = get_current_rt(Factor_Quantization.data(), Factor_Quantization.size(), 10 * dPRESS[t], &pressrt_dist);
			cur_thrt_rt = get_current_rt(Throttle_Quantization.data(), Throttle_Quantization.size(), dTRT[t], 0);

			cur_factor_rt = cur_press_rt;

			if (dRPM[t] > 2900 && dLC[t] > 16)
			{
				cur_factor_rt = cur_press_rt;
			}
			stationary_pf = abs(cur_press_rt - prev_press_rt) < 3;
			stationary_rpm = abs(cur_rpm_rt - prev_rpm_rt) < 2;
			//stationary_pf = prev_rpm_rt == cur_rpm_rt && prev_press_rt == cur_press_rt;
			if (stationary_pf && stationary_pf_prev)
				stationary_count++;
			else
				stationary_count = 0;
			if(stationary_pf && stationary_rpm)//stationary_count > 1)
			{
				table_core[cur_factor_rt][cur_rpm_rt].
					push_back(pointoflearn(dRPM[t], dPRESS[t], dLC[t]/dAFR[t], 0, rpmrt_dist, pressrt_dist));
			}

			prev_rpm_rt = cur_rpm_rt;
			prev_press_rt = cur_press_rt;
			stationary_pf_prev = stationary_pf;
		}
#endif
		free_log_params(logs_params_ptr, params_count);
		free_log_data(logs_data_ptr, params_count);
	}


	setlocale(LC_ALL, "ru");
	
	/*
		Learning points is all that not above 20% of minimal distance from factor
	*/
	for (int i = 0; i < table_size; i++)
	{
		for (int j = 0; j < table_size; j++)
		{
			float mindist = 1;//Minimum of 2D distance
			int count_points;
			float mid_inf = 0.4;
			/*
				letc find minimum dist and all points that not above 20% if they too much, 
				that not above 10% etc while 3 cycles not arrived
			*/
			for(int cycles_inf = 0; cycles_inf < 3; cycles_inf++)
			{
				count_points = 0;
				mindist = 1;
				for (int k = 0; k < table_core[i][j].size(); k++)//minimum calculating
				{
					mindist = std::min(mindist, hypot(table_core[i][j][k].rpmrt_dist, table_core[i][j][k].factorrt_dist));
				}
				for (int k = 0; k < table_core[i][j].size(); k++)//providing learn point that not above 20%
				{
					if (hypot(table_core[i][j][k].rpmrt_dist, table_core[i][j][k].factorrt_dist) <= mindist* (1.0 + mid_inf))
					{
						table_core[i][j][k].learn = 1;
						count_points++;
					}
					else
						table_core[i][j][k].learn = 0;

				}
				mid_inf *= 0.5;
				if (count_points <= 5)
					break;
			}
		}
	}

	float table_correction[table_size][table_size];
	float table_pcn_in[table_size][table_size];
	#if table_size == 32
		read_cte_data32("pcn_press32.cte", table_pcn_in, cte_head, cte_caption);
	#else
		read_cte_data("pcn_press16.cte", table_pcn_in, cte_head, cte_caption);
		//read_cte_data("bcn_thrt.cte", table_pcn_in, cte_head, cte_caption);
	#endif
	
	for(int i = 0; i < table_size; i++)
	{
		for(int j = 0; j < table_size; j++)
		{
			/*if(table_pcn_exp_count[i][j] != 0)//table_pcn_exp[i][j] != 0)
				table_pcn_in[i][j] = table_pcn_in[i][j] * (table_pcn_exp[i][j]/float(abs(table_pcn_exp_count[i][j])));*/

			float meancoef = 0;
			int meancnt = 0;
			for (int k = 0; k < table_core[i][j].size(); k++)
			{
				if (table_core[i][j][k].learn)
				{
					meancoef += table_core[i][j][k].coef;
					meancnt++;
				}
			}
			if (meancnt)
				table_correction[i][j] = meancoef / float(meancnt);
			else
				table_correction[i][j] = 1;
		}
	}

	for (int i = 0; i < table_size; i++)
	{
		for (int j = 0; j < table_size; j++)
		{
			table_pcn_in[i][j] *= table_correction[i][j];
		}
	}

	#if table_size == 32
		write_cte_data32("pcn_press_rpm_experimental32.cte", table_pcn_in, cte_head, cte_caption);
	#else
		write_cte_data("pcn_press_rpm_experimental16.cte", table_pcn_in, cte_head, cte_caption);
		//write_cte_data("bcn_thrt_experimental16.cte", table_pcn_in, cte_head, cte_caption);
	#endif
	
	
	FILE *fo2 = fopen("correction_exp.txt", "w");
	for(int i = 0; i < table_size; i++)
	{
		for(int j = 0; j < table_size; j++)
		{
			fprintf(fo2, "%0.3f\t", table_correction[i][j]);
		}
		fprintf(fo2, "\n");
	}
	fclose(fo2);
	

	/*
	float table_bcn_old[16][16];
	read_cte_data("bcn.cte", table_bcn_old, cte_head, cte_caption);
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			if (table_gbc_count[i][j] > 0)
				table_bcn_old[i][j] = table_gbc[i][j] / float(table_gbc_count[i][j]);
		}
	}
	write_cte_data("bcn_new.cte", table_bcn_old, cte_head, cte_caption);
	*/

	/*
	тут задумывалось, если файл не открылся для чтения старых данных, то новый не создается
	добавление данных идет только в старый файл, возможно даже в пустой
	const char * map_fail_name = "imitator_press_whenMAP_failure.cte";
	read_cte_data(map_fail_name, table_MAP_failure, cte_head, cte_caption);
	for (int i = 0; i < table_size; i++)
	{
		for (int j = 0; j < table_size; j++)
		{
			if(table_MAP_failure_cnt[i][j] != 0)
				table_MAP_failure[i][j] = 10.0*table_MAP_failure[i][j] / float(table_MAP_failure_cnt[i][j]);
		}
	}
	write_cte_data(map_fail_name, table_MAP_failure, "[13128542]", "Имитатор давления при отказе ДАД");

	*/
	return 0;
}


