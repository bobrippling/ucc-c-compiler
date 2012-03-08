#define __socketcall_socket    1
#define __socketcall_bind      2
#define __socketcall_listen    4
#define __socketcall_accept    5

/* implemented in socket.c */
int socketcall(int call, unsigned long *args);
