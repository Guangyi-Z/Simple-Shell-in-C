#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "util.h"
#include "cmd.h"

// Simplifed xv6 shell.

#define CMD_MAXLEN 100
#define die(e) do { fprintf(stderr, e); exit(EXIT_FAILURE); } while (0);

struct cmd* parsepipe(char **ps, char *es);
struct cmd* parseexec(char **ps, char *es);
struct cmd* parseredirs(struct cmd *cmd, char **ps, char *es);

// Execute cmd.  Never returns.
void 
runcmd(struct cmd *cmd)
{
	int p[2], r;
	struct execcmd *ecmd;
	struct pipecmd *pcmd;
	struct redircmd *rcmd;

	if(cmd == 0)
		exit(0);

	switch(cmd->type){
		default:
			fprintf(stderr, "unknown runcmd\n");
			exit(-1);

		case ' ':
			ecmd = (struct execcmd*)cmd;
			if(ecmd->argv[0] == 0)
				exit(0);
			char abscmd[CMD_MAXLEN];
			if(NULL == searchfile(abscmd, CMD_MAXLEN, ecmd->argv[0], F_OK|R_OK|X_OK)){
				fprintf(stderr, "file not exist or permission denied\n");
				exit(-1);
			}
			if(-1 == execv(abscmd, ecmd->argv) ){
				fprintf(stderr, "exec failed\n");
			}
			break;

		case '>':
		case '<':
			rcmd = (struct redircmd*)cmd;
			char absfile[CMD_MAXLEN];
			memset(absfile, 0, sizeof(absfile));
			int newfd;
			if(rcmd->fd == 0){
				if(-1 == access(rcmd->file, F_OK|R_OK)){
					newfd= open(rcmd->file, rcmd->mode);
				}
				else {
					if(NULL == searchfile(absfile, sizeof(absfile), rcmd->file, F_OK|R_OK)){
						die("redirction file not exist or permission denied");
					}
					else{
						newfd= open(absfile, rcmd->mode);
					}
				}
			}
			else{
				if('/' == (rcmd->file)[0]){
					newfd= open(rcmd->file, rcmd->mode);
				}
				else{
					char* pwd= getenv("PWD");
					strcat(absfile, pwd);
					strncat(absfile, "/", 1);
					strncat(absfile, rcmd->file, sizeof(rcmd->file));
					newfd= open(absfile, rcmd->mode, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
				}
			}
			int bakfd= dup(rcmd->fd);
			dup2(newfd, rcmd->fd);
			close(newfd);
			runcmd(rcmd->cmd);
			dup2(bakfd, rcmd->fd);
			break;

		case '|':
			pcmd = (struct pipecmd*)cmd;

			int pipes[2];
			if(-1 == pipe(pipes)){
				die("pipe");
			}


			int pid= fork();
			if(-1 == pid){
				die("fork");
			}
			if(0 == pid){
				// left cmd
				dup2(pipes[1], STDOUT_FILENO);
				close(pipes[0]);
				runcmd(pcmd->left);
			}
			else{
				// right cmd
				dup2(pipes[0], STDIN_FILENO);
				close(pipes[1]);
				waitpid(pid);
				runcmd(pcmd->right);
			}
			break;
	}
	exit(0);
}

struct cmd* 
parsecmd(char *s)
{
	char *es;
	struct cmd *cmd;

	es = s + strlen(s);
	cmd = parsepipe(&s, es);
	if(s != es){
		fprintf(stderr, "leftovers: %s", s);
		exit(-1);
	}
	return cmd;
}

struct cmd* 
parsepipe(char **ps, char *es)
{
	struct cmd *cmd;

	char *q, *eq;
	if( 1 == scan(ps, es, "|", &q, &eq) ){
		cmd = parseexec(&q, eq);
		(*ps)++;
		cmd = pipecmd(cmd, parsepipe(ps, es));
	}
	else {
		cmd = parseexec(&q, eq);
	}
	return cmd;
}

/*
 * if '<' or '>' exists, wrap the cmd into redircmd, otherwise return cmd directly
 */
struct cmd* 
parseredirs(struct cmd *cmd, char **ps, char *es)
{
	int tok;
	char *q, *eq;

	while(peek(ps, es, "<>")){
		tok = gettoken(ps, es, 0, 0); // get the single '<' or '>'
		if(gettoken(ps, es, &q, &eq) != 'a') {
			fprintf(stderr, "missing file for redirection\n");
			exit(-1);
		}
		switch(tok){
			case '<':
				cmd = redircmd(cmd, mkcopy(q, eq), '<');
				break;
			case '>':
				cmd = redircmd(cmd, mkcopy(q, eq), '>');
				break;
		}
	}
	return cmd;
}

struct cmd* 
parseexec(char **ps, char *es){
	char *q, *eq;
	int tok, argc;
	struct execcmd *cmd;
	struct cmd *ret;

	ret = execcmd();
	cmd = (struct execcmd*)ret;

	argc = 0;
	ret = parseredirs(ret, ps, es);
	while(*ps < es){
		if((tok=gettoken(ps, es, &q, &eq)) == 0)
			break;
		if(tok != 'a') {
			fprintf(stderr, "syntax error\n");
			exit(-1);
		}
		cmd->argv[argc] = mkcopy(q, eq);
		argc++;
		if(argc >= MAXARGS) {
			fprintf(stderr, "too many args\n");
			exit(-1);
		}
		ret = parseredirs(ret, ps, es);
	}
	cmd->argv[argc] = 0;
	return ret;
}

