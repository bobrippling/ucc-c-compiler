// RUN: %ucc -P -E %s | %output_check -w '"\"yo\""' '"\"\\\"hi\\there\\\"\""'
#define QUOTE(x) #x

QUOTE("yo")
QUOTE("\"hi\there\"")
