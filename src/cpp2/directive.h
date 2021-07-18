#ifndef PARSE_H
#define PARSE_H

char *include_parse(
		char *include_arg_anchor, int *const is_lib,
		int may_expand_macros);

void parse_directive(char *line);
void parse_internal_directive(char *line);
int  parse_should_noop(void);
void parse_end_validate(void);

#endif
