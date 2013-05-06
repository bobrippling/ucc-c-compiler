#!/usr/bin/perl -p
use warnings;

s%"([^"]+)"%"(char[]){" . join(', ', map { "'" . $_ . "'" } (split(//, $1), '\0')) . "}"%ge
