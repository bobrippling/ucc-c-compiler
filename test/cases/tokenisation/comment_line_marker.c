// RUN: %ucc -fsyntax-only %s

/* Previously, on hitting the final '/' of a comment, we'd parse the next
 * token. The line directive code wouldn't parse the line directive, thinking
 * we were still in a comment, then we'd finally falsify the in_comment
 * variable, but by then it was too late.
 */

/**/
# 0 ""
