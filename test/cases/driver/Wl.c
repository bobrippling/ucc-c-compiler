// RUN: %ucc -Wl,-abc,hi a.o -'###' 2>&1 | grep -- '-abc hi'
