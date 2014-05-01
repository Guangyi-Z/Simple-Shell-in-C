
char whitespace[];

int scan(char **ps, char *es, const char *toks, char **q, char **eq);
char* mkcopy(char *s, char *es);
void skip_whitespace(char **ps, char *es);
int peek(char **ps, char *es, char *toks);
int istoks(char *s, char *toks);
int gettoken(char **ps, char *es, char **q, char **eq);
