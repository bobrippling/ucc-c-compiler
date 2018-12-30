// RUN: %caret_check %s

char a[];int x=sizeof a;
// CARETS:
//             ^ error: sizeof incomplete type
