// RUN: %ucc -nostdinc -I"$(dirname "%s")" -P -E %s | %stdoutcheck %s

#define YO "hi"
#include YO

#define YO2 <stdio.h>
#include YO2

// STDOUT: A
// STDOUT: B
