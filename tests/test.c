#include <stdio.h>

int main()
{
  int a = 5;
  int b = a;
  int c = 10;
  for (int i = 0;i < 10;i++)
    b++;
  printf("%d", b);
}