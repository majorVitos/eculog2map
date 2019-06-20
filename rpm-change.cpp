

#include "config.h"
#include "file-cte-log.h"


#include <clocale>
#include <iostream>
#include <sstream>


/*
table_type - reserved (���� �� ������������, � ����� � �������� 2� ��������)
rpms_src - ������ �������� �������� �������, ������ {600, 720, 840, 990, ...}
rpms_dst - ������ �������� ������������ �������, ������ ����� {800, 1000, 1200, ...}
src - �������� �������
dst - ������������, ������ ������ ���� ��������
*/
int change_rpm(const char *type, const int *rpms_src, const int *rpms_dst, const float * const * const src, float **dst)
{
	int is, js;//������ ������� y (�������� �������� ����������), � ��������� x (�������� �������)
	float a, b;
	if (sscanf(type, "%dx%d", &is, &js) != 2)
		return 3;//bad arguments
	for (int i = 0; i < is; i++)
	{
		for (int j = 0, k = 1; j < js; j++)
		{
			for (; k < js; k++)//����� ��������� [k-1;k] ��������
			{
				if (rpms_dst[j] <= rpms_src[k])
					break;
			}
			if (k == js)//�������� � ����� ������� � �� �����, ���������� ���������
				k--;
			a = (src[i][k - 1] - src[i][k]) / float(rpms_src[k - 1] - rpms_src[k]);//�������� ����������������
			b = src[i][k] - a*rpms_src[k];
			dst[i][j] = a*float(rpms_dst[j]) + b;
		}
	}
}


int rpm_change()
{
	char cte_caption[256], cte_head[256];
	float **tablei, **tableo;
	int ret;
	uint16_t is = 16, js = 16;
	std::string input;

	std::string cfg_name = "rpm-chng.cfg";
	std::string input_name = std::string("input") + std::to_string(16) + ".cte";
	std::string output_name = std::string("output") + std::to_string(16) + ".cte";
	tsconfig config;
	if ( (ret = config.open(cfg_name)) != 0)
	{
		std::cerr << "Config file \"" << cfg_name << "\" opening error: code " << ret << " (" << config.error_str << ")\n";
		return ret;
	}
	
	std::cout << "Enter table size (16x16 or 16x32 or ...[default 16x16]):\n";
	std::getline(std::cin, input);
	if (!input.empty() && ( sscanf(input.c_str(), "%hdx%hd", &is, &js) < 2 || is > 256 || js > 256) )
	{
			std::cerr << "Looks like you're joking\n";
			return 1;
	}


	std::vector<int> rpm_src = config.geti("RPM_SRC");
	std::vector<int> rpm_dst = config.geti("RPM_DST");
	if (rpm_src.size() != is && rpm_dst.size() != is)//is "is" rpm?
	{
		std::cerr << "table size not equal with rpm dimenstions: rpm.size = " << is << std::endl;
		return 1;
	}
	input.clear();
	std::cout << "Enter file name .cte [default input16.cte]:\n";
	std::getline(std::cin, input);
	if (!input.empty())
		input_name = input;

	tablei = new float* [js];
	tableo = new float* [js];
	for (size_t j = 0; j < js; j++)
	{
		tablei[j] = new float[is];
		tableo[j] = new float[is];
	}
	

	std::setlocale(LC_ALL, "ru");


	if ((ret = read_cte(input_name.c_str(), js, is, tablei, cte_head, cte_caption)) == 0)
	{
		change_rpm((std::to_string(is) + "x" + std::to_string(js)).c_str(), rpm_src.data(), rpm_dst.data(), tablei, tableo);

		ret = write_cte(output_name.c_str(), js, is, tableo, cte_head, cte_caption);
		if (ret != 0)
			std::cerr << "Error writing cte file" << output_name << " code: " << ret << std::endl;
	}
	else
	{
		std::cerr << "Error reading cte file:" << input_name << " code: " << ret << "\n";
	}

	for (size_t i = 0; i < is; i++)
	{
		delete[] tablei[i];
		delete[] tableo[i];
	}
	delete[] tablei;
	delete[] tableo;
	return ret;
}