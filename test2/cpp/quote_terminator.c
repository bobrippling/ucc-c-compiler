// RUN: %ucc -P -E %s | %output_check -w "'\"' 5" "\"'\" 5"

#define abc 5

'"' abc
"'" abc
