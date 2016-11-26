#include <stdio.h>
#include <sys/time.h>

int partition(int *A, int left, int right)
{
  int key = A[left];
  int L = left, R = right;
  while(L != R){
    while(A[R] >= key && L != R) --R;
    A[L] = A[R];
    while(A[L] <= key && L != R) ++L;
    A[R] = A[L];
  }
  A[L] = key;
  return L;
}
     
void quickSort(int *A, int left, int right)
{
  if(left >= right) return;
  int p = partition(A, left, right);
  quickSort(A, left, p-1);
  quickSort(A, p+1, right);
}

#define MAX 100
int N;
int a[MAX];
int main()
{
  //freopen("qsort.txt", "r", stdin);
  scanf("%d", &N);
  for(int i = 0; i < N; ++i){
    scanf("%d", a+i);
  }

  struct timeval start, stop;
  quickSort(a, 0, N - 1);
  
  printf("sorted:\n");
  for (int i = 0; i < N; ++i){
    printf("%d\n", a[i]);
  }
  return 0;
}
