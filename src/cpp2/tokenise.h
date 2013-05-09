#ifndef TOKENISE_H
#define TOKENISE_H

typedef struct
{
	enum tok
	{
		TOKEN_WORD,
		TOKEN_OPEN_PAREN,
		TOKEN_CLOSE_PAREN,
		TOKEN_COMMA,
		TOKEN_ELIPSIS,
		TOKEN_OTHER
	} tok;
	char *w;
	int had_whitespace;
} token;

token **tokenise(char *line);
const char *token_str(token *t);

char *tokens_join(token **tokens);
char *tokens_join_n(token **tokens, int lim);

#endif
