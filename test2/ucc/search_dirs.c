// RUN: %ucc -nostdinc -isystem /system/include -Ilocal -I. -print-search-dirs | grep -F 'include: local:.:/system/include'
