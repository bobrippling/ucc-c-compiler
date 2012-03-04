#include <stdlib.h>

#ifdef REAL_CURSES
# include <ctype.h>
# include <curses.h>
#else
# define true 1
typedef void WINDOW;
#endif

int main(void)
{
  WINDOW *window = initscr();
  int rows;
  int cols;

  cbreak();

  keypad(window, true);
  noecho();

  getmaxyx(window, rows, cols);

  curs_set(0);

  for(;;){
		int key;
    printw("Press a Key ");
    refresh();
    key = wgetch(window);
    printw("%d\n", key);
  }

  endwin();

  return 0;
}

/*
 * These functions need implementing:
access
btowc
calloc
cfgetospeed
__ctype_b_loc
__errno_location
fclose
fflush
fileno
fopen
__fprintf_chk
fread
fwrite
getenv
getmaxyx
getpgrp
gettimeofday
ioctl
_IO_putc
isatty
iswprint
mblen
mbrtowc
mbtowc
memcpy
memmove
memset
nanosleep
nl_langinfo
poll
realloc
setlocale
setvbuf
sigaction
sigaddset
sigemptyset
sigismember
sigprocmask
__sprintf_chk
sscanf
__stack_chk_fail
strchr
strcmp
strcpy
__strcpy_chk
__strdup
strncat
strncmp
strncpy
strrchr
strstr
strtol
tcflush
tcgetattr
tcgetpgrp
tcsetattr
__vfprintf_chk
__vsnprintf_chk
vsscanf
wcrtomb
wcslen
wcstombs
wctob
wcwidth
wmemchr
wmemcpy
__xstat
*/
