// RUN: ! %ucc -'###' -xc /dev/null          2>&1 | grep -- _REENTRANT
// RUN:   %ucc -'###' -xc /dev/null -pthread 2>&1 | grep -- -D_REENTRANT=1
// RUN: ! %ucc -'###' -xc /dev/null          2>&1 | grep -- -lpthread
// RUN:   %ucc -'###' -xc /dev/null -pthread 2>&1 | grep -- -lpthread
