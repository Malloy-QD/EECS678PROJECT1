#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/socket.h>
#include<unistd.h>
#include<readline/readline.h>
#include<readline/history.h>

static char* env;
static char* dir;
static char* cmd;
static char* path;
static char* home;
//static int countJob = 0;
//static struct Job jobs[100];
/**
 * 
 * 
 * 
 * 
 *
 **/
char* removeSpaces(char* str){
	int count = 0;
	for (int i =0;str[i];i++)
		if(str[i]!=' ')
			str[count++]=str[i];

	str[count]='\0';
}

void parseCommand(char* command){
	//remove whitespace
	command = removeSpaces(command);

	//begin of cd command
	if(strncmp(command,"cd",2)==0){
		//skip cd
		command = command + 2;
		if(command==NULL){
			chdir(getenv("HOME"));
			dir = getcwd(NULL,1024);
		}
		else
		{
			if(chdir(command)==-1)
				printf("Not a valid path");
			dir = getcwd(NULL,1024);
		}

	}
	//end of cd command

	//set command	
	else if(strncmp(command,"set",3)==0){
		//skip set
		command = command+3;
		if(strncmp(command,"PATH",4)==0){
			//skip PATH
			command = command+4;
			if(strncmp(command,"=",1)==0){
				command=command+1;
				char* token = strstr(command,"/");
				char* path1;
				char* path2;
				for(int i = 0;i<sizeof(token);i++){
					if(token[i]==":"){
						for(int j=0;j<i-1;j++)
							path1[i]=token[i];
						for(int k=i+1;k<sizeof(token);k++)
							path2[k]=token[k];
					}
				}
				//path=[path1,path2];
				if(setenv("PATH",path,0)<0)
					printf("Wrong Path");
				else{
				path=getenv("PATH");
				printf("Path: %s\n", path);
				}
			
			}
		else
		{
			printf("Wrong Path Format or syntax\n");
		}
		}

	
	else if (strncmp(command,"HOME",4)==0){
		command = command+4;
		if(strncmp(command,"=",1)==0){
			command = command + 1;
			char* token = strstr(command,"/");
			printf("Home Path: %s\n",token);
			if(setenv("HOME",token,1)<0)
				printf("Wrong Home");
			else{
				home = getenv("HOME");
				printf("Home: %s\n",home);
			}
		}
		else
		{
			printf("Wrong Home Format or syntax\n");
		}
	}
	}

	
	//end of set PATH AND HOME
	//else if (strncmp(command,"jobs",4)==0)
	//{
		//printf("   JOBID   ","   PID   ","   COMMAND   \n");
		//for(int i= 0;i<countJob;i++){
			//if(kill(jobs[i].pid,0)==0){
				//printf("   ",jobs[i].id,"   ","   ",jobs[i].pid,"   ","   ",jobs.[i].cmd,"   ","\n");
			//}
		//}
	//}
	else if (strncmp(command,"quit",4)==0)
	{
		exit(0);
	}
	else if (strncmp(command, "exit", 4) == 0)
	{
		exit(0);
	}
}

struct Job
{
	int pid;
	int id;
	char* cmd;
};

int main(int argc, char** argv, char** envp){
	printf("-----------------------------------------------------\n");
	printf("-------------------Welcome to Quash------------------\n");
	printf("-----------------------------------------------------\n");
	char* cin;
	char* readLine[128];
	env = getenv("USER");
	dir = getcwd(NULL,1024);
	//countJob = 0;
		home = getenv("HOME");
		path = getenv("PATH");
		rl_bind_key('\t',rl_complete);
		while(true){
			snprintf(readLine,sizeof(readLine),"[%s:%s]$",env,dir);
			cin = readline(readLine);
			parseCommand(cin);
		}
		free(cin);
	return 0;
}
