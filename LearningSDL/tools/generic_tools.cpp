#include "generic_tools.hpp"

int generic_tools::random_number(int limit) 
{
	srand(time(0));
	int num = 0 + rand() % limit;
	return num;
}

int generic_tools::random_number(int limit_start, int limit_end) 
{
	//srand(time(0));
	int num = limit_start + rand() % (limit_end - limit_start + 1);
	return num;
}


void generic_tools::clear_console()
{
	system("cls");
}

void generic_tools::sleep(int time) 
{
	std::this_thread::sleep_for(std::chrono::seconds(time));
}

void generic_tools::wait_for_enter() 
{
	std::cout << "\nPresiona enter para salir";
	std::cin.ignore();
	while (std::cin.get() != '\n');
}