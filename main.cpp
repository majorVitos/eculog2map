



#include "config.h"
#include "file-cte-log.h"


#include <algorithm>
#include <vector>

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>



template<class T, int n, int m>
void table_init(T table[n][m], T v)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < m; j++)
		{
			table[i][j] = v;
		}
	}
}

/*бинарный поиск ближайшей рабочей точки
* в data, по значению value, возвращает индекс, и расстояние distance
*растояние относительное, где 0 - полное попадание в рт, 0.5 - точка по средине
*/
int get_current_rt(const std::vector<int> data, const int value, float *distance)
{
	size_t a = 0, b = data.size();
	size_t k = 0;
	float norm, dist1, dist2;
	while(a < b)
	{
		k = a + ((b - a) >> 1);
		if (value == data[k])
			break;
		if (value > data[k])
			a = k + 1;
		else
			b = k;
	}
	dist1 = data[k] - value;
	if (k + 1 < data.size())
		norm = 1.0f / float(data[k] - data[k + 1]);
	else
	{
		if (distance)
			*distance = abs( dist1 / float(data[k - 1] - data[k]));  
		return k;
	}
	dist1 = abs(dist1 * norm);
	dist2 = abs(norm * (data[k + 1] - value));
	if (dist1 > dist2)
	{
		if(distance)
			*distance = dist2;
		return k + 1;
	}
	if(distance)
		*distance = dist1;
	return k;
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
*	addition by adc of narrow oxygen sensor / 
*/
float narrowWide(float value)
{
	if (value < 0.1f)	return 1.07f;
	if (value > 0.8f)	return 0.95f;
	return 1.f;
}

float pressureByADC(const float ADC)
{
	return (ADC + 0.0195f)*61.656f;
}

struct pointoflearn
{
	float rpm, trt, factor, gbc, time, coef, adc_lam, rpmrt_dist, factorrt_dist;
	int learn;
	pointoflearn(float _rpm, float _trt, float _factor, float _gbc, float _time, float _coef, float _adc_lam, float _rpmrt_dist, float _factorrt_dist)
		: rpm(_rpm), trt(_trt), factor(_factor), gbc(_gbc), time(_time), coef(_coef), adc_lam(_adc_lam), rpmrt_dist(_rpmrt_dist), factorrt_dist(_factorrt_dist), learn(0)
	{}
};

std::vector<std::string> get_all_logs_filenames();




const int table_size = 16;

bool flag_pf_throttle = false;//power_factor = throttle
bool flag_use_lsu = true;

int main(int argc, char* argv[])
{
	float **logs_data_ptr;
	char **logs_params_ptr;
	int params_count, data_count;
	int err;

	std::vector<pointoflearn> table_core[table_size][table_size];	
	std::vector<int> RPM_Quantization;
	std::vector<int> Factor_Quantization;
	std::vector<int> Throttle_Quantization;
	
	if (argc > 1)
	{
		printf("Unsuported parameter: %s", argv[1]);
	}

	tsconfig cfg1("test.cfg");
	cfg1.get_int("RPM_es" + std::to_string(table_size), RPM_Quantization);
	cfg1.get_int("Press_quant" + std::to_string(table_size), Factor_Quantization);
	cfg1.get_int("Throttle_quant", Throttle_Quantization);


	std::setlocale(LC_ALL, "en");
	

	std::vector<std::string> file_names = get_all_logs_filenames();
	std::vector<log_data_type> logs_data;

	for (size_t fi = 0; fi < file_names.size(); fi++)
	{
		//err = read_logs_csv2(std::string("logs\\") + file_names[fi], logs_data);


		err = read_logs_csv((std::string("logs\\") + file_names[fi]).c_str(), &logs_data_ptr, &logs_params_ptr, &params_count, &data_count);

		const float *dTIME		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "TIME", "Time");
		const float *dTRT		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 3, "TRT", "THR", "Дроссель");
		const float *dRPM		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "RPM", "обороты двс");
		const float *dfLAM		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "fLAM", "текущее состояние ДК");
		const float *dCOEFF		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "COEFF", "коэфицент коррекции времени впрыска");
		const float *dPRESS		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "PRESS", "PRESSURE J7ES");
		const float *dLAMREG	= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "fLAMREG", "флаг зоны регулирования по дк");
		const float *dTWAT		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "TWAT", "температура ОЖ");
		const float *dGBC		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "GBC", "Цикловой расход воздуха");
//		const float *dADC_LAM	= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "ADCLAM", "АЦП ДК");
	
		const float *dADC_MAF	= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "ADCMAF", "АЦП ДМРВ");
		const float *dINJ		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "INJ", "Время впрыска");
		const float *dAFR		= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "AFR", "состав смеси");//Desired afr
		const float *dLC_AFR	= get_logsdata_param_ptr(logs_data_ptr, logs_params_ptr, params_count, 2, "AFR_LC1", "AFR_LC");

		if (!dTIME || !dTRT || !dRPM || !dfLAM || !dCOEFF || !dPRESS || !dLAMREG || !dTWAT || !dGBC)
		{
			printf("log file corrupted!\n");
			return 1;
		}

		int factor_rt, prev_factor_rt = 0;
		int rpm_rt, prev_rpm_rt = 0;

		float rpmrt_dist, power_factor_dist;
		int stationary_rpm;				//Стационарность по оборотам 
		int stationary_rpm_count = 0;
		int stationary_pf;				//Стационарность по дросселю/давлению
		int stationary_pf_count = 0;
		int stationary_lam_count = 0;

		float coef_mean, coef_deviation;

		for (int t = 1; t < data_count; t++)
		{
			coef_deviation = calc_deviation(&coef_mean, dCOEFF[t], 4);
			if (dLAMREG[t] < 1 && !flag_use_lsu)	continue;//не в зоне регулирования УДК и нет ШДК
			if (dTWAT[t] < 70 )	continue;//двигатель не прогрет
			if (dRPM[t] < 0.9*RPM_Quantization[0])	continue;//обороты ниже рабочих
			
			float press = pressureByADC(dADC_MAF[t]);
			if (flag_pf_throttle != true)
			{
				press = dPRESS[t];
				if (press * 10.0 < 0.9*Factor_Quantization[0])//Давление слижком низкое
					continue;
			}

			
			rpm_rt =	get_current_rt(RPM_Quantization, static_cast<int>(dRPM[t]), &rpmrt_dist);

			if(flag_pf_throttle)
				factor_rt = get_current_rt(Throttle_Quantization, static_cast<int>(dTRT[t]), &power_factor_dist);
			else
				factor_rt =	get_current_rt(Factor_Quantization,	static_cast<int>(10*press), &power_factor_dist);


			stationary_rpm = abs(rpm_rt - prev_rpm_rt) < 1;
			stationary_pf = abs(factor_rt - prev_factor_rt) < 2;
			/*
			*
			*
			*/
			if (stationary_rpm & (dINJ[t] > 0) )	stationary_rpm_count++;
			else				stationary_rpm_count = 0;
			if (stationary_pf  & (dINJ[t] > 0) )	stationary_pf_count++;
			else				stationary_pf_count = 0;

			if (stationary_rpm && stationary_pf && (coef_deviation < 0.05))//видимо стационарность по лямбде
					stationary_lam_count++;
			else	stationary_lam_count = 0;

			if (stationary_rpm_count > 2 && stationary_pf_count > 2 && stationary_lam_count > 2)
			{
				float flam_delta = dfLAM[t] - dfLAM[t - 1];
				if ( (flam_delta >= 0 && dCOEFF[t] >= 1) || (flam_delta <= 0 && dCOEFF[t] <= 1) )
				{
					float coef = dCOEFF[t] * dLC_AFR[t] / dAFR[t];
					table_core[factor_rt][rpm_rt].push_back(pointoflearn
					//(dTIME[t], dRPM[t], dTRT[t], dTRT[t], dCOEFF[t] * narrowWide(dADC_LAM[t]), dADC_LAM[t], rpmrt_dist, power_factor_dist));
					//(dTIME[t], dRPM[t], dTRT[t], dPRESS[t], dCOEFF[t] * narrowWide(dADC_LAM[t]), dADC_LAM[t], rpmrt_dist, power_factor_dist));
					(dRPM[t], dTRT[t], press, dGBC[t]*coef, dTIME[t], coef, 0, rpmrt_dist, power_factor_dist));
				}
			}

			prev_rpm_rt = rpm_rt;
			prev_factor_rt = factor_rt;
		}
		free_log_params(logs_params_ptr, params_count);
		free_log_data(logs_data_ptr, params_count);
	}

	std::setlocale(LC_ALL, "ru");
	

	for (int i = 0; i < table_size; i++)
	{
		for (int j = 0; j < table_size; j++)
		{
			std::sort(table_core[i][j].begin(), table_core[i][j].end(), [](const pointoflearn &a, const pointoflearn &b) -> bool
			{
				return std::hypotf(a.rpmrt_dist, a.factorrt_dist) < std::hypotf(b.rpmrt_dist, b.factorrt_dist);
			});
			int cnt = 0;
			for (auto p = table_core[i][j].begin(); p != table_core[i][j].end(); ++p)
			{
				p->learn = 1;
				if (++cnt > 10)
					break;
			}
		}
	}

/*
*
*
*
	for (int i = 0; i < table_size; i++)
	{
		for (int j = 0; j < table_size; j++)
		{
			float mindist;//Minimum distance
			int count_points;
			float mid_inf = 0.4f;
			
			for(int cycles_inf = 0; cycles_inf < 3; cycles_inf++)
			{
				count_points = 0;
				mindist = 1;
				for (size_t k = 0; k < table_core[i][j].size(); k++)//minimum calculating
				{
					mindist = std::min(mindist, hypotf(table_core[i][j][k].rpmrt_dist, table_core[i][j][k].factorrt_dist) );
				}
				for (size_t k = 0; k < table_core[i][j].size(); k++)//providing learn point that not above 20%
				{
					if (hypotf(table_core[i][j][k].rpmrt_dist, table_core[i][j][k].factorrt_dist) <= mindist* (1.0 + mid_inf))
					{
						table_core[i][j][k].learn = 1;
						count_points++;
					}
					else
						table_core[i][j][k].learn = 0;

				}
				mid_inf *= 0.5;
				if (count_points >= 5)
					break;
			}
		}
	}
	*/

	float table_correction[table_size][table_size];
	float table_pcn_in[table_size][table_size];

	char cte_caption[256], cte_head[256];
	read_cte_data( (std::string("pcn_press") + std::to_string(table_size) + ".cte").c_str(), table_pcn_in, cte_head, cte_caption);//pcn_press16.cte and pcn_press32.cte

	for(size_t i = 0; i < table_size; i++)
	{
		for(size_t j = 0; j < table_size; j++)
		{
			float meancoef = 0;
			int meancnt = 0;
			for (size_t k = 0; k < table_core[i][j].size(); k++)
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
	/*
	*	
	*/
	write_cte_data((std::string("pcn_press_experimental") + std::to_string(table_size) + ".cte").c_str(), table_pcn_in, cte_head, cte_caption);//"pcn_press_experimental16.cte"
	//write_cte_data("bcn_thrt_experimental16.cte", table_pcn_in, cte_head, cte_caption);
	
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
	

	float table_bcn[16][16];
	char cte_bcn_caption[256], cte_bcn_head[256];
	int res = read_cte_data("bcn_press.cte", table_bcn, cte_bcn_head, cte_bcn_caption);
	for (size_t i = 0; i < 16; i++)
	{
		for (size_t j = 0; j < 16; j++)
		{
			int cnt = 0;
			float mean = 0.0f;
			for (size_t k = 0; k < table_core[i][j].size(); k++)
			{
				if (table_core[i][j][k].learn)
				{
					mean += table_core[i][j][k].gbc;
					cnt++;
				}
			}
			if (cnt)
				table_bcn[i][j] = mean / float(cnt);
		}
	}
	write_cte_data("bcn_press.cte", table_bcn, cte_bcn_head, cte_bcn_caption);


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


