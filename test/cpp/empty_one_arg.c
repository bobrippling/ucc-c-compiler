// RUN: %ucc -E %s -P | %output_check -w '><' '><' '><' '><'
// RUN: %check %s -E -fmessage-length=0

#define F(a) >a<
#define G(...) >__VA_ARGS__<

F()   // CHECK: /warning: empty argument list to single-argument macro "F"/
F( )

G() // this is fine
G( )
