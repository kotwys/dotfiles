#include <stdio.h>
#include <time.h>

int main()
{
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  printf("%2d月%2d日\n", t->tm_mon + 1, t->tm_mday);
  return 0;
}
