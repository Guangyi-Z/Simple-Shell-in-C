
char* searchfile(char *file, int len, char* name, int mode);
int scan(char **ps, char *es, const char *toks, char **q, char **eq);
char* mkcopy(char *s, char *es);
int peek(char **ps, char *es, char *toks);
int gettoken(char **ps, char *es, char **q, char **eq);
