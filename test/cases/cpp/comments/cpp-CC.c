// RUN: %ucc %s -E -P -CC | sed 1,2d | grep -F '/*kerment*/ tim'

// -CC keeps comments in # lines too

#define YO //kerment

YO tim
