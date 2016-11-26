#include <stdio.h>
#include <stdlib.h>

int acker(int m, int n)
{
    if(m == 0) 
        return n + 1;
    else if(n == 0)
        return acker(m - 1, 1);
    else
        return acker(m - 1, acker(m, n - 1));
}
int main(int argc, char *argv[])
{
    int a, b;
    scanf("%d %d", &a, &b);
    printf("%d\n", acker(a, b));
    return 0;
}
