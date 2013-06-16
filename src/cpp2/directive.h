#ifndef PARSE_H
#define PARSE_H

void parse_directive(char *line);
void parse_internal_directive(char *line);
int  parse_should_noop(void);
void parse_end_validate(void);

#endif
