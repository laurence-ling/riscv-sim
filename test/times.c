#include <sys/times.h>
#include <stdio.h>

struct tms time_info;
int main()
{
	clock_t clk = times(&time_info);
	unsigned int t = clk;
	printf("%d\n%d\n", sizeof(time_info), sizeof(clk));
	return 0;
}
