#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_CMD_ARG 10
#define MAX_CMD_GRP 10

#define FOREGROUND 1
#define BACKGROUND 0

void fatal(char *str);
void execute_cmdline(char* cmdline);
int makelist(char *s, const char *delimiters, char** list, int MAX_LIST);
void changeDIR();

const char *prompt = "Command> ";

char* cmdgrps[MAX_CMD_GRP];
char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];
char  copycmd[BUFSIZ];

void fatal(char *str){
	perror(str);
	exit(1);
}

void changeDIR(){
	if(cmdvector[1] == '\0' || strcmp(cmdvector[1], "~") == 0){
		char *homePath = getenv("HOME");
		chdir(homePath);
	}
	else if(chdir(cmdvector[1]) == -1)
		perror("Wrong path\n");
}

void redirection(char* cmdgrps){
	int i = 0;
	int fd = 0;
	int length = 0;
	char* filename = NULL;
	length = strlen(cmdgrps);

	for(i=length-1; i>=0; i--){
		if(cmdgrps[i] == '>'){
			filename = strtok(&cmdgrps[i+1], " \t");
			fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC,0644);
			dup2(fd, 1);
			close(fd);
			cmdgrps[i] = '\0';
		}
		if(cmdgrps[i] == '<'){
			filename = strtok(&cmdgrps[i+1], " \t");
			fd = open(filename, O_CREAT|O_RDONLY,0644);
			dup2(fd, 0);
			close(fd);
			cmdgrps[i] = '\0';
		}
	}
}


void execute_cmd(char* cmd, int type){
	int count;
	redirection(cmd);
	count = makelist(cmd, " \t", cmdvector, MAX_CMD_ARG);
	if(type == BACKGROUND){
		cmdvector[count-1] = '\0';
	}
	execvp(cmdvector[0], cmdvector);
	fatal("exec error");
}


void execute_cmdgrp(char* cmdgrp, int type){
	int i;
	int p[2];
	int cnt_pipe;
	setpgid(0,0);

	cnt_pipe = makelist(cmdgrp, "|", cmdvector, MAX_CMD_GRP);
	for(i=0; i<cnt_pipe-1; i++){
		pipe (p);
		if(fork()){
			dup2(p[0], 0);
			close(p[0]);
			close(p[1]);
		}else{
			dup2(p[1], 1);
			close(p[0]);
			close(p[1]);
			break;
		}
	}
	execute_cmd(cmdvector[i], type);
}

void execute_cmdline(char* cmdline)
{
	int type=FOREGROUND;
  	int count = 0;
	pid_t pid, pid2;
	int status;
	int i=0;
	count = makelist(cmdline, ";", cmdgrps, MAX_CMD_GRP);
	int vecCount = 0;

	for(i=0; i<count; ++i)
	{
		strcpy(copycmd, cmdgrps[i]);
		vecCount = makelist(copycmd, " \t", cmdvector, BUFSIZ);
		if(cmdvector[0] == NULL){
			return;
		}
		if(strcmp(cmdvector[0], "cd") == 0){
			changeDIR();
			return;
		}
		if(strcmp(cmdvector[0], "exit") == 0){
			exit(0);
		}
	    if(strcmp(cmdvector[vecCount-1], "&") == 0){
			cmdvector[vecCount-1]=NULL;
			type = BACKGROUND;
		}
		pid = fork();
		if(pid == 0){
			if(type){
				setpgid(0,0);
				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
				tcsetpgrp(STDIN_FILENO, getpgid(0));
				execute_cmdgrp(cmdgrps[i], type);

				fatal("run_cmd\n");
				break;
    			}
    			else{
				switch(pid2 = fork())
				{
					case -1: fatal("fork error");
					case  0:
						execute_cmdgrp(cmdgrps[i], type);
						fatal("exec error"); 
					default:	
						exit(0);
				}
			}
		}
		else{
			if(type == FOREGROUND){
				waitpid(pid, &status, WUNTRACED);
				tcsetpgrp(STDIN_FILENO, getpgid(0));
			}
			else
				wait(NULL);
				
			fflush(NULL);
		} 
	}
}

int makelist(char *s, const char *delimiters, char** list, int MAX_LIST){
	int numtokens = 0;
	char *snew = NULL;
 
	if( (s==NULL) || (delimiters==NULL) ) return -1;
 
	snew = s + strspn(s, delimiters);    /* delimiters¸¦ skip */
	if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
		return numtokens;
 
	numtokens = 1;

	while(1){
		if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
			break;
		if(numtokens == (MAX_LIST-1)) return -1;
			numtokens++;
	}
	return numtokens;
}

int main(int argc, char**argv){
	struct sigaction act;
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	act.sa_handler = 0;
	sigemptyset(&act.sa_mask);

	act.sa_flags = SA_NOCLDWAIT;
	sigaction(SIGCHLD, &act, NULL);

	while (1) {
		fputs(prompt, stdout);
		fgets(cmdline, BUFSIZ, stdin);
		cmdline[ strlen(cmdline) -1] ='\0';
		if(cmdline != NULL){
			execute_cmdline(cmdline);
		}		
	}
	return 0;
}