// RUN: %check -e %s

f = ({return 3;}); // CHECK: /error: return outside a function/
