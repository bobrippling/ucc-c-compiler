// RUN: %check -e %s

int i = (int)(int){5}; // CHECK: /error:.*not constant/
