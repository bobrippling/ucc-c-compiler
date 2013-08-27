// RUN: %ucc %s -E | grep 'not gone'
// RUN: %ucc %s -E | grep 'disappear'; [ $? -ne 0 ]

#define YO //disappear

YO tim

// not gone
