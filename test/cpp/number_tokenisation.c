int i = 0x3ffe+63;
int x = 0xe-0xa;

/*

6.4.8 pp-number:

pp-number e sign
pp-number E sign
pp-number identifier-nondigit

/3:
Preprocessing number tokens lexically include all floating and integer constant
tokens.

/4:
A preprocessing number does not have type or a value; it acquires both after a
successful conversion (as part of translation phase 7) to a floating constant
token or an integer constant token.

Not a successive token to either a floating point token or an integer constant
token -> rejected

*/
