// RUN: %ucc -P -E %s | %output_check -w '"CONCATTHREE(PATH, FILNAME, .h)";'
#define CONCAT_(a, b) a ## b
#define CONCAT(a, b) CONCAT_(a, b)
#define CONCAT3(a, b, c) CONCAT(CONCAT(a, b), c)
#define STRINGIFY(a) # a

#define PATH    "path/to/dir/"
#define FILNAME "fnam"

//#include STRINGIFY(CONCATTHREE(PATH  ,FILNAME  ,.h));

STRINGIFY(CONCATTHREE(PATH, FILNAME, .h));
