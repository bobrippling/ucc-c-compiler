// RUN: %ucc -E %s | grep yo
// RUN: %ucc -E %s 2>&1 | grep 'warning: endif directive with extra tokens'

#define YO

#if defined(YO      )
yo
#endif hi
