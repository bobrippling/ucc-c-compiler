// RUN: %ucc -nostdinc -I"$(dirname "%s")" -P -E %s | %output_check -w A B

#define YO "hi"
#include YO

#define YO2 <stdio.h>
#include YO2
