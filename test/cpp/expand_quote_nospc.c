// RUN: %ucc -P -E %s | %output_check -w 's="ab" "cd" "ef";' 's="ab""cd" "ef";' 's="ab" L"ef";' 's="ab"L"ef";'

#define L "cd"
s="ab" L "ef";
s="ab"L "ef";
s="ab" L"ef";
s="ab"L"ef";
