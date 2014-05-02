#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "sh.h"
#include "util.h"
#include "parser.h"

#define MAX_BUF_LEN 100
#define die(e) do { fprintf(stderr, e); exit(EXIT_FAILURE); } while (0);

int getcmd(char *buf, int nbuf);

int 
main(void)
{
	static char buf[MAX_BUF_LEN];
	char pipebuf[4096];
	int fd, r;

	// Read and run input commands.
	while(getcmd(buf, sizeof(buf)) >= 0){
		// cd command
		if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
			// Chdir has no effect on the parent if run in the child.
			buf[strlen(buf)-1] = 0;  // chop \n
			if(chdir(buf+3) < 0)
				fprintf(stderr, "cannot cd %s\n", buf+3);
			continue;
		}
		// new process for new command
		int pipes[2]; // create a pipe throught parent and child process
		if(pipe(pipes) == -1){
			die("err: pipe");
		}
		int pid;
		pid = fork();
		if(pid == -1)
			die("err: fork");
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
			// print the output from child process
			while(0 != (n= read(pipes[0], pipebuf, sizeof(pipebuf)-1))){
				pipebuf[n]= '\0';
				printf("%s", pipebuf);
			}
		}
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
