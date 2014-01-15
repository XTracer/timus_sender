#include <iostream>
	
int main()
{
	long n, i;
	std::cin >> n;
	for (i = 3; n%i; ++i);
	std::cout << i-1 << std::endl;

	return 0;
}