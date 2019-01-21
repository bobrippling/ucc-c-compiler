// RUN: %ucc %s -P -E -C | sed 1,2d | grep 'not gone'
// RUN: %ucc %s -P -E -C | sed 1,2d | grep 'disappear'; [ $? -ne 0 ]

#define YO //disappear

YO tim

// not gone
