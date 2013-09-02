// RUN: %ucc -o %t %s -E
// RUN: %output_check '/*kerment*/ tim' < %t

// -CC keeps comments in # lines too

#define YO //kerment

YO tim
