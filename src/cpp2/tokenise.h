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

		TOKEN_STRING,

		TOKEN_HASH_QUOTE,
		TOKEN_HASH_JOIN,

		TOKEN_OTHER
	} tok;
	char *w;
	int had_whitespace;
} token;

token **tokenise(char *line);
const char *token_str(token *t);

char *tokens_join(token **tokens);
char *tokens_join_n(token **tokens, int lim);

token **tokens_skip_whitespace(token **tokens);
int tokens_just_whitespace(token **tokens);
int tokens_count_skip_spc(token **tokens);

void tokens_free(token **);

#endif
