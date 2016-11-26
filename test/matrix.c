#include <stdio.h>
#include <sys/time.h>

#define M 1000
int a[M][M], b[M][M], c[M][M];

int main()
{
  int N;
  scanf("%d", &N);
  for(int i = 0; i < N; ++i)
      for(int j = 0; j < N; ++j)
          scanf("%d", &(a[i][j]));
  for(int i = 0; i < N; ++i)
      for(int j = 0; j < N; ++j)
          scanf("%d", &(b[i][j]));
  
  for(int i = 0; i < N; ++i)
      for(int j = 0; j < N; ++j)
          for(int k = 0; k < N; ++k)
              c[i][j] += a[i][k] * b[k][j];

  for(int i = 0; i < N; ++i) {
      for(int j = 0; j < N; ++j)
          printf("%d ", c[i][j]);
      printf("\n");
  }
  return 0;
}
