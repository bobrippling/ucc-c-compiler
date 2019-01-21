// RUN: %ucc -I"$(dirname "%s")" -P -E %s | %output_check -w A B C

#include <">
#include "\""
#include <\>>
