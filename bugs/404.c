// RUN: %ucc -o %t %s
// RUN: %t | %output_check '404'

#define v putchar
# define print(x) main(){v(4+v(v(52)-4));return 0;}/*
#>+++++++4+[>++++++<-]>+++++.----.++++.*/
print(202*2);exit();
#define/*>.@*/exit()
