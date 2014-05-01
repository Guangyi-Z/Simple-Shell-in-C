#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include"util.h"

char*
searchcmd(char *cmd, int len, char* name){
	char* env= getenv("PATH");
	
	char* end= env + strlen(env);
	char *spath, *epath;
	while( scan(&env, end, ":", &spath, &epath)){
		env++;
		if(spath == epath){
			break;
		}
		memset(cmd, 0, len);
		if(epath-spath+strlen(name)+2 > len){ // 1 for '/', 1 for '\0'
			fprintf(stderr, "cmd abs path too long\n");
		}
		strncat(cmd, spath, epath-spath);
		strncat(cmd, "/", 1);
		strncat(cmd, name, strlen(name));
		if( 0 == access(cmd, F_OK|R_OK|X_OK) ){
			return cmd;
		}
	}
	return NULL;
}


