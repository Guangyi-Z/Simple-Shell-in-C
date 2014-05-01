#include<assert.h>
#include "util.h"

char whitespace[] = " \t\r\n\v";

/*
 * make [q, eq] the first substr before any char in toks.
 * return 1 if any char in toks exists, 0 otherwise.
 */
int scan(char **ps, char *es, const char *toks, char **q, char **eq){

	skip_whitespace(ps, es);
	char **s= ps;
	*q= *s;

	while( *s<es && !strchr(toks, **s) ){
		*s= *s+1;
	}

	*eq= *s;
	return *s==es? 0 : 1;
}

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char *
mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

/*
 *  * make *ps point to the first non-whitespace char or the null of the end
 *   */
void
skip_whitespace(char **ps, char *es)
{
	  char *s;
	    
	  s = *ps;
      while(s < es && strchr(whitespace, *s))
		    s++;
	  *ps = s;
}

/*
 * skip the whitespaces till the next char, which is tested to see whether it occurs in toks char stream
 */
int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

int
istoks(char *s, char *toks)
{
	  return *s && strchr(toks, *s);
}

