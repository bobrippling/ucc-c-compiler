// RUN: %ucc -P -E %s | %output_check -w /cpp\/base_file.c/ /cpp\/base_file.c/ /cpp\/base_file.c/ /cpp\/base_file.h/
__BASE_FILE__
__FILE__

#include "base_file.h"
