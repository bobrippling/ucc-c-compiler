// RUN: %ucc -P -E %s | %stdoutcheck %s

#define L "cd"
s="ab" L "ef";
s="ab"L "ef";
s="ab" L"ef";
s="ab"L"ef";

// STDOUT: s="ab" "cd" "ef";
// STDOUT: s="ab""cd" "ef";
// STDOUT: s="ab" L"ef";
// STDOUT: s="ab"L"ef";
