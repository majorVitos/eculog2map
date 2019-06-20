
#include <iostream>


int eculog2map();
int rpm_change();

int main(int argc, char* argv[])
{
	int action;

	if (argc > 1)
	{
		printf("Unsuported parameter: %s", argv[1]);
	}
	std::cout << "eculog2map actions:\n" << "  1). Analyze log for map correction" << std::endl;
	std::cout << "  2). Approx table via RPM change" << std::endl;
	std::cout << "\n0).\tExiting" << std::endl;
	std::cout << "Press number of action:" << std::endl;
	std::cin >> action;
	//action = 2; debug door
	try
	{
		switch (action)
		{
		case 0:
			std::cout << "Exiting...\n";
			return 0;

		case 1:
			return eculog2map();

		case 2:
			return rpm_change();
		default:
			break;
		}
	}
	catch (const std::string& ex)
	{
		std::cerr << "Unhandled exception:\n" << ex;
		return 1000;
	}
	return 0;
}