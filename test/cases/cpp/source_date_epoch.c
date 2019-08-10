// RUN: SOURCE_DATE_EPOCH=0 %ucc -E -P -o %t %s
// RUN: %stdoutcheck %s < %t
//      STDOUT: timestamp "Thu Jan 01 00:00:00 1970"
// STDOUT-NEXT: date "Jan 01 1970"
// STDOUT-NEXT: time "00:00:00"

timestamp __TIMESTAMP__
date __DATE__
time __TIME__
