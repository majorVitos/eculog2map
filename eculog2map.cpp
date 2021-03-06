

#include "config.h"
#include "file-cte-log.h"

#include <iostream>
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

/*�������� ����� ��������� ������� �����
* � data, �� �������� value, ���������� ������, � ���������� distance
*��������� �������������, ��� 0 - ������ ��������� � ��, 0.5 - ����� �� �������
*/
int get_current_rt(const std::vector<int> data, const int value, float* distance)
{
	size_t a = 0, b = data.size();
	size_t k = 0;
	float norm, dist1, dist2;
	while (a < b)
	{
		k = a + ((b - a) >> 1);
		if (value == data[k])
			break;
		if (value > data[k])
			a = k + 1;
		else
			b = k;
	}
	dist1 = float(data[k] - value);
	if (k + 1 < data.size())
		norm = 1.0f / float(data[k] - data[k + 1]);
	else
	{
		if (distance)
			* distance = abs(dist1 / float(data[k - 1] - data[k]));
		return k;
	}
	dist1 = abs(dist1 * norm);
	dist2 = abs(norm * (data[k + 1] - value));
	if (dist1 > dist2)
	{
		if (distance)
			* distance = dist2;
		return k + 1;
	}
	if (distance)
		* distance = dist1;
	return k;
}


/*
���������� �� ������������, ���� ��������, �� ��������, ���������� ��������������
� ���������� �� ������������, ��� ������ �������������
*/
#define DEVIATION_SIZE 4
float xi[DEVIATION_SIZE + 1];
float calc_deviation(float* _mean, const float val, int count)
{
	float& mean = *_mean;
	float dev;
	mean = dev = 0.0;
	if (count > DEVIATION_SIZE)
		count = DEVIATION_SIZE;
	xi[count] = val;
	for (int i = 0; i < count; i++)
	{
		xi[i] = xi[i + 1];
		mean += xi[i];
	}
	mean = mean / float(count);
	for (int i = 0; i < count; i++)
	{
		dev += (xi[i] - mean) * (xi[i] - mean);
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
	return (ADC + 0.0195f) * 61.656f;
}

struct pointoflearn
{
	int rpm, trt;
	float factor, gbc, time, coef, adc_lam, rpmrt_dist, factorrt_dist;
	int learn;
	pointoflearn(int _rpm, int _trt, float _factor, float _gbc, float _time, float _coef, float _adc_lam, float _rpmrt_dist, float _factorrt_dist)
		: rpm(_rpm), trt(_trt), factor(_factor), gbc(_gbc), time(_time), coef(_coef), adc_lam(_adc_lam), rpmrt_dist(_rpmrt_dist), factorrt_dist(_factorrt_dist), learn(0)
	{}
};



const int table_size = 16;

bool flag_pf_throttle = false;//power_factor = throttle
bool flag_use_lsu = true;

int eculog2map()
{
	std::vector<pointoflearn> table_core[table_size][table_size];
	int err;
	
	tsconfig cfg1("test.cfg");
	if (cfg1.error)
	{
		std::cout << "Problem with config file:" << "test.cfg" << "\n" << cfg1.error_str;
		return 100 + cfg1.error;
	}
	std::vector<int> RPM_Quantization		= cfg1.geti("RPM_es" + std::to_string(table_size) );
	std::vector<int> Factor_Quantization	= cfg1.geti("Press_quant" + std::to_string(table_size));
	std::vector<int> Throttle_Quantization  =  cfg1.geti("Throttle_quant");
	
	std::setlocale(LC_ALL, "en");
	std::vector<std::string> file_names = get_all_logs_filenames("logs\\");


	for (size_t fi = 0; fi < file_names.size(); fi++)
	{
		data_logs_t data_logs;
		err = read_logs_csv(file_names[fi], data_logs);
		if (err != 0)
		{
			perror("");
			return 4;
		}

		std::vector<float>	dTIME	= get_logs_data<float>(data_logs, cfg1.gets("TIME"));
		std::vector<float>	dCOEFF	= get_logs_data<float>(data_logs, cfg1.gets("COEFF"));
		std::vector<int>	dTRT	= get_logs_data<int>(data_logs, cfg1.gets("TRT"));
		std::vector<int>	dRPM	= get_logs_data<int>(data_logs, cfg1.gets("RPM"));
		std::vector<int>	dfLAM	= get_logs_data<int>(data_logs, cfg1.gets("fLAM"));
		std::vector<float>	dPRESS	= get_logs_data<float>(data_logs, cfg1.gets("PRESS"));
		std::vector<int>	dLAMREG	= get_logs_data<int>(data_logs, cfg1.gets("fLAMREG"));
		std::vector<float>	dTWAT	= get_logs_data<float>(data_logs, cfg1.gets("TWAT"));
		std::vector<float>	dGBC	= get_logs_data<float>(data_logs, cfg1.gets("GBC"));
		std::vector<float>	dADC_LAM= get_logs_data<float>(data_logs, cfg1.gets("ADCLAM"));
		std::vector<float>	dADC_MAF= get_logs_data<float>(data_logs, cfg1.gets("ADCMAF"));
		std::vector<float>	dINJ	= get_logs_data<float>(data_logs, cfg1.gets("INJ"));
		std::vector<float>	dAFR	= get_logs_data<float>(data_logs, cfg1.gets("AFR"));
		std::vector<float>	dLC_AFR = get_logs_data<float>(data_logs, cfg1.gets("AFR_LC"));
		if((dTIME.size() & dCOEFF.size() & dTRT.size() & dRPM.size() & dfLAM.size() & dPRESS.size() & dLAMREG.size() &
			dTWAT.size() & dGBC.size() & dADC_LAM.size() & dADC_MAF.size() & dINJ.size() & dAFR.size() & dLC_AFR.size()) == 0)
		{
			std::string es = "logs data: Not found names";
			return 2;
		}

		int factor_rt, prev_factor_rt = 0;
		int rpm_rt, prev_rpm_rt = 0;

		float rpmrt_dist, power_factor_dist;
		int stationary_rpm;				//�������������� �� �������� 
		int stationary_rpm_count = 0;
		int stationary_pf;				//�������������� �� ��������/��������
		int stationary_pf_count = 0;
		int stationary_lam_count = 0;

		float coef_mean, coef_deviation;

		for (size_t t = 1; t < dTIME.size(); t++)
		{
			coef_deviation = calc_deviation(&coef_mean, dCOEFF[t], 4);
			if (dLAMREG[t] < 1 && !flag_use_lsu)	continue;//�� � ���� ������������� ��� � ��� ���
			if (dLC_AFR[t] < 1.0f && flag_use_lsu)	continue;//
			if (dTWAT[t] < 70)	continue;//��������� �� �������
			if (dRPM[t] < 0.9 * RPM_Quantization[0])	continue;//������� ���� �������

			float press = pressureByADC(dADC_MAF[t]);
			if (flag_pf_throttle != true)
			{
				press = dPRESS[t];
				if (press * 10.0 < 0.9 * Factor_Quantization[0])//�������� ������� ������
					continue;
			}


			rpm_rt = get_current_rt(RPM_Quantization, static_cast<int>(dRPM[t]), &rpmrt_dist);

			if (flag_pf_throttle)
				factor_rt = get_current_rt(Throttle_Quantization, static_cast<int>(dTRT[t]), &power_factor_dist);
			else
				factor_rt = get_current_rt(Factor_Quantization, static_cast<int>(10 * press), &power_factor_dist);


			stationary_rpm = abs(rpm_rt - prev_rpm_rt) < 1;
			stationary_pf = abs(factor_rt - prev_factor_rt) < 2;
			/*
			*
			*
			*/
			if (stationary_rpm & (dINJ[t] > 0))	stationary_rpm_count++;
			else				stationary_rpm_count = 0;
			if (stationary_pf & (dINJ[t] > 0))	stationary_pf_count++;
			else				stationary_pf_count = 0;

			if (stationary_rpm && stationary_pf && (coef_deviation < 0.05))//������ �������������� �� ������
				stationary_lam_count++;
			else	stationary_lam_count = 0;

			if (stationary_rpm_count > 2 && stationary_pf_count > 2 && stationary_lam_count > 2)
			{
				int flam_delta = dfLAM[t] - dfLAM[t - 1];//is this integer
				if ((flam_delta >= 0 && dCOEFF[t] >= 1) || (flam_delta <= 0 && dCOEFF[t] <= 1))
				{
					float coef = dCOEFF[t] * dLC_AFR[t] / dAFR[t];
					table_core[factor_rt][rpm_rt].push_back(pointoflearn
						//(dTIME[t], dRPM[t], dTRT[t], dTRT[t], dCOEFF[t] * narrowWide(dADC_LAM[t]), dADC_LAM[t], rpmrt_dist, power_factor_dist));
						//(dTIME[t], dRPM[t], dTRT[t], dPRESS[t], dCOEFF[t] * narrowWide(dADC_LAM[t]), dADC_LAM[t], rpmrt_dist, power_factor_dist));
						(dRPM[t], dTRT[t], press, dGBC[t] * coef, dTIME[t], coef, 0.0, rpmrt_dist, power_factor_dist));
				}
			}

			prev_rpm_rt = rpm_rt;
			prev_factor_rt = factor_rt;
		}
	}

	std::setlocale(LC_ALL, "ru");


	for (int i = 0; i < table_size; i++)
	{
		for (int j = 0; j < table_size; j++)
		{
			std::sort(table_core[i][j].begin(), table_core[i][j].end(), [](const pointoflearn& a, const pointoflearn& b) -> bool
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



	float table_correction[table_size][table_size];
	float table_pcn_in[table_size][table_size];

	char cte_caption[256], cte_head[256];
	read_cte_file<16,16>((std::string("pcn_press") + std::to_string(table_size) + ".cte").c_str(), table_pcn_in, cte_head, cte_caption);//pcn_press16.cte and pcn_press32.cte

	for (size_t i = 0; i < table_size; i++)
	{
		for (size_t j = 0; j < table_size; j++)
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
	write_cte_file<16,16>((std::string("pcn_press_experimental") + std::to_string(table_size) + ".cte").c_str(), table_pcn_in, cte_head, cte_caption);//"pcn_press_experimental16.cte"
	//write_cte_data("bcn_thrt_experimental16.cte", table_pcn_in, cte_head, cte_caption);

	FILE* fo2 = fopen("correction_exp.txt", "w");
	for (int i = 0; i < table_size; i++)
	{
		for (int j = 0; j < table_size; j++)
		{
			fprintf(fo2, "%0.3f\t", table_correction[i][j]);
		}
		fprintf(fo2, "\n");
	}
	fclose(fo2);


	float table_bcn[16][16];
	char cte_bcn_caption[256], cte_bcn_head[256];
	int res = read_cte_file<16,16>("bcn_press.cte", table_bcn, cte_bcn_head, cte_bcn_caption);
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
	write_cte_file<16,16>("bcn_press.cte", table_bcn, cte_bcn_head, cte_bcn_caption);

	/*
	��� ������������, ���� ���� �� �������� ��� ������ ������ ������, �� ����� �� ���������
	���������� ������ ���� ������ � ������ ����, �������� ���� � ������
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
	write_cte_data(map_fail_name, table_MAP_failure, "[13128542]", "�������� �������� ��� ������ ���");

	*/
	return res;
}