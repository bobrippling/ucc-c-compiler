// RUN: %ucc -nostdinc -I"$(dirname "%s")" -E %s | grep -F 'empty;'

#define hello __hello_there
#include <hello.h>
