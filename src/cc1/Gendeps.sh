#!/bin/sh

[ $# -ne 0 ] && exit 1

d=Makefile.dep

cc -MM *.c > $d
cc -MM ops/*.c | sed 's#^\([^ ]\)#ops/\1#' >> $d
cc -MM out/*.c | sed 's#^\([^ ]\)#out/\1#' >> $d
