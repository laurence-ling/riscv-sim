#include <stdio.h>

int a[20] = {0};
int r = 10;
int main()
{
	int N = 0;
	printf("Please give the number N: ");
	scanf("%d", &N);
	a[0] = r + 1;
	a[1] = r - 1;
	a[2] *= r;
	a[3] /= r;
	for (int i = 0; i < N; ++i)
		printf("a[%d] = %d\n", i, a[i]);
	return 0;
}
