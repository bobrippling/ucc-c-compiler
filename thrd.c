#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
  const char *s = "Hello, World!\n";
  while (*++s ? fork() : wait(NULL) >= 0 && s--);
  putchar(*--s);
}
