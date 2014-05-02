
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "cmd.h"

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

