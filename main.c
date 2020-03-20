#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/socket.h>
#include<unistd.h>

static char* env;
static char* dir;
static char* cmd;
static char* path;
static char* home;
char* removeSpaces(char* str){
	int count = 0;
	for (int i =0;str[i];i++)
		if(str[i]!=' ')
			str[count++]=str[i];

	str[count]='\0';
}

void parseCommand(char* command){
	command = removeSpaces(command);
	//set command	
	if(strncmp(command,"set",3)==0){
	command = command+4;
		if(strncmp(command,"PATH",4)==0){
		command = command+4;
		}
		printf("%s\n",command);
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
		if(setenv("PATH",path1,0)<0)
		//print();
		else{
		path1=getenv("PATH");
		}
		
			}
					
			
			}
	

		}
	}
}
void setPath(char* newPath){
	char* token = strtok(newPath,"=");
	char* type = token;
	token = strtok(NULL,"\0");
	char* rest = token;

	if(setenv(type,rest,1)== -1){
		printf("%s isn't a correct set\n",type);
	}
	
}
