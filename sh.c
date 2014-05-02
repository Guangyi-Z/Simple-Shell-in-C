#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sh.h"
#include "util.h"

// Simplifed xv6 shell.

char symbols[] = "<|>";
char pipebuf[4096];

int fork1(void);  // Fork but exits on failure.
struct cmd *parsecmd(char*);
struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

int
gettoken(char **ps, char *es, char **q, char **eq)
{
	char *s;
	int ret;

	s = *ps;
	while(s < es && strchr(whitespace, *s))
		s++;
	if(q)
		*q = s;
	ret = *s;
	switch(*s){
		case 0:
			break;
		case '|':
		case '<':
			s++;
			break;
		case '>':
			s++;
			break;
		default:
			ret = 'a';
			while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
				s++;
			break;
	}
	if(eq)
		*eq = s;

	while(s < es && strchr(whitespace, *s))
		s++;
	*ps = s;
	return ret;
}

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

int 
getcmd(char *buf, int nbuf)
{

	if (isatty(fileno(stdin)))
		fprintf(stdout, "$ ");
	memset(buf, 0, nbuf);
	fgets(buf, nbuf, stdin);
	if(buf[0] == 0) // EOF
		return -1;
	return 0;
}

int 
main(void)
{
	static char buf[100];
	int fd, r;

	// Read and run input commands.
	while(getcmd(buf, sizeof(buf)) >= 0){
		if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
			// Clumsy but will have to do for now.
			// Chdir has no effect on the parent if run in the child.
			buf[strlen(buf)-1] = 0;  // chop \n
			if(chdir(buf+3) < 0)
				fprintf(stderr, "cannot cd %s\n", buf+3);
			continue;
		}

		int pipes[2];
		if(pipe(pipes) == -1){
			die("err: pipe");
		}

		int pid;
		pid = fork();
		if(pid == -1)
			die("fork");

		if(0 == pid){
			dup2(pipes[1], STDOUT_FILENO);
			close(pipes[0]);
			struct cmd* cm= parsecmd(buf);
			runcmd(cm);
		}
		else{
			close(pipes[1]);
			wait(NULL);
			int n;
			while(0 != (n= read(pipes[0], pipebuf, sizeof(pipebuf)-1))){
				pipebuf[n]= '\0';
				printf("%s", pipebuf);
			}
		}
	}
	exit(0);
}

int 
fork1(void)
{
	int pid;

	pid = fork();
	if(pid == -1)
		perror("fork");
	return pid;
}

struct cmd* 
execcmd(void)
{
	struct execcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = ' ';
	return (struct cmd*)cmd;
}

struct cmd* 
redircmd(struct cmd *subcmd, char *file, int type)
{
	struct redircmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = type;
	cmd->cmd = subcmd;
	cmd->file = file;
	cmd->mode = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
	cmd->fd = (type == '<') ? 0 : 1;
	return (struct cmd*)cmd;
}

struct cmd* 
pipecmd(struct cmd *left, struct cmd *right)
{
	struct pipecmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = '|';
	cmd->left = left;
	cmd->right = right;
	return (struct cmd*)cmd;
}

// Parsing

struct cmd* 
parsecmd(char *s)
{
	char *es;
	struct cmd *cmd;

	es = s + strlen(s);
	cmd = parseline(&s, es);
	skip_whitespace(&s, es);
	if(s != es){
		fprintf(stderr, "leftovers: %s", s);
		exit(-1);
	}
	return cmd;
}

struct cmd* 
parseline(char **ps, char *es)
{
	struct cmd *cmd;
	/*
	   char* q, *eq;
	   while( getsubcmd(ps, es, q, eq, ';') ){
	   cmd = parsepipe(&q, eq);
	   }
	   */
	cmd = parsepipe(ps, es);
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

struct cmd* 
parseredirs(struct cmd *cmd, char **ps, char *es)
{
	int tok;
	char *q, *eq;

	while(peek(ps, es, "<>")){
		tok = gettoken(ps, es, 0, 0);
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

