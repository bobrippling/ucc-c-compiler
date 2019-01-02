// RUN: SOURCE_DATE_EPOCH=0 %ucc -E -P -o %t %s
// RUN: %output_check -w 'timestamp "Thu Jan 01 00:00:00 1970"' 'date "Jan 01 1970"' 'time "00:00:00"' <%t

timestamp __TIMESTAMP__
date __DATE__
time __TIME__
