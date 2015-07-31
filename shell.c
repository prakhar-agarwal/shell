#include<sys/types.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<string.h>
#include<sys/stat.h>

#define MAX_LINE 10000
#define MAX_COMMANDS 100
#define MAX_ARGS 100
#define MAX_LEN 100
#define checkDelim input[i] == ' ' || input[i] == '|' || input[i] == ',' || input[i] == '\n' || input[i] == '\t' || input[i] == '"'
#define NOPIPE 0
#define RPIPE 1
#define WPIPE 2
#define RWPIPE 3
#define OUT_REDIRECTED 4
#define DPIPE 5
#define TPIPE 6

void strip(char *s) {
    char *p2 = s;
    while(*s != '\0') {
    	if(*s != '\t' && *s != '\n' && *s != '\r') {
    		*p2++ = *s++;
    	} else {
    		++s;
    	}
    }
    *p2 = '\0';
}

struct command
{
    int pipe[MAX_COMMANDS];
    int  input_fd[MAX_COMMANDS]; 
    int output_fd[MAX_COMMANDS]; 
    int num_commands;
    int num_args[MAX_COMMANDS]; 
    char* command_list[MAX_COMMANDS]; 
    char* args_list[MAX_COMMANDS][MAX_ARGS]; 
    int append;
};

void print_prompt(){
	
	printf(">>");
	fflush(stdout);
}

int read_input(char *input){
	int size;
	
	if((size = read(STDIN_FILENO,input,MAX_LINE)) > 0){
		return size;
	}
	return -1;
}


int findNextState(char *input,int i,int *state,int length){
	
	
	while(i < length){
		
		if(input[i] == '\n'){
			i+=1;
			return i;
		}
		else if(input[i] == ' '){
			i+=1;
			continue;
		}else if(input[i] == ','){
			state[0] = 10;
			return i;
		}else if(input[i] == '>'){
			if(input[i+1] == '>'){
				state[0] = 2;
				i += 2;
				return i;
			}else{
				state[0] = 3;
				i += 1;
				return i;
			}
		}else if(input[i] == '<'){
			state[0] = 4;
			i += 1;
			return i;
		}else if(input[i] == '|'){
			if(input[i+1] == '|'){
				if(input[i+2] == '|'){
					state[0] = 9;
					i+=3;
					return i;
				}else{
					state[0] = 8;
					i+=2;
					return i;
				}
			}else{
				state[0] = 7;
				i+=1;
				return i;
			}
		}else if(state[0] == 5){
			state[0] = 6;
			return i;
		}else if(input[i] == '"'){
			i+=1;
			continue;
		}else if(state[0] == 7 || state[0] == 8 || state[0] == 9){
			state[0] = 5;
			return i;
		}
	}
	return i;
}

int parse_input(char *input,struct command *cmd){
	
	int state[] = {-1};
	int length = strlen(input);
	int cmd_num = 0,arg_num = 0;
	
	char input_filename[MAX_LEN];
	char output_filename[MAX_LEN];
	
	char *t_cmd;
	char *t_args;
	
	int i = 0;
	
	if(input[0] == '>'){
		if(input[1] == '>'){
			state[0] = 2;
				i = 2;
		}else{
			state[0] = 3;
				i = 1;
		}
	}else if(input[0] == '<'){
		state[0] = 4;
			i = 1;
	}else{
		state[0] = 5;
	}
	
	//state = 5 => a command is being read;
	//state = 6 => an argument to a command is being read
	//state = 7 => it is an ending double quote
	//state = 8,9,10 => single,double and triple pipes
	//state = -1 => error
	/*
	struct command
{
    
    int  input_fd[MAX_COMMANDS]; 
    int output_fd[MAX_COMMANDS]; 

    int num_commands;
    int num_args[MAX_COMMANDS]; 
    char* command_list[MAX_COMMANDS]; 
    char* args_list[MAX_COMMANDS][MAX_ARGS]; 
    int append;
}
	
	*/
	

	
while(i < length){
			if(state[0] == 2){
				int count = 0;
				memset(output_filename,'\0',MAX_LEN);
				 int s = -1;
				while(1){
				if(s == -1 && input[i] == ' '){
								i+=1;
                                continue;
                }else if( s == 2 && checkDelim){
							int flags = O_CREAT | O_WRONLY | O_APPEND;
							int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
							cmd->output_fd[cmd_num] = open(output_filename,flags,filePerms);
							//printf("state = %d str = %s\n",state[0],output_filename);
							i = findNextState(input,i,state,length);
							cmd->pipe[cmd_num] = OUT_REDIRECTED;
							break;
					}else{
						s = 2;
						output_filename[count] = input[i];
						i+=1;
						count+=1;
					}
				}
			}else if(state[0] == 3){
			
				int count = 0;
				memset(output_filename,'\0',MAX_LEN);
				int s = -1;
				while(1){
					if(s == -1 && input[i] == ' '){
								i+=1;
                                continue;
                }else if( s == 2 && checkDelim){
							
							int flags = O_CREAT | O_WRONLY | O_TRUNC;
							int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
							cmd->output_fd[cmd_num] = open(output_filename,flags,filePerms);
							//printf("state = %d str = %s\n",state[0],output_filename);
							i = findNextState(input,i,state,length);
							cmd->pipe[cmd_num] = OUT_REDIRECTED;
							break;
							
					}else{
						s = 2;
						output_filename[count] = input[i];
						i+=1;
						count+=1;
					}
				}
			}else if(state[0] == 4){
				int count = 0;
				memset(input_filename,'\0',MAX_LEN);
				int s = -1;
				while(1){
					if(s == -1 && input[i] == ' '){
								i+=1;
                                continue;
                }else if( s == 2 && checkDelim){
							int flags = O_CREAT | O_RDONLY;
							int filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
							cmd->input_fd[cmd_num] = open(input_filename,flags,filePerms);
						//	printf("state = %d str = %s\n",state[0],input_filename);
							i = findNextState(input,i,state,length);
							
							
							break;
					}else{
						s = 2;
						input_filename[count] = input[i];
						i+=1;
						count+=1;
					}
				}
			}else if(state[0] == 5){
			
				int count = 0;
				t_cmd = (char *) malloc(sizeof(char)*MAX_LEN);
				int s = -1;
				while(1){
				if(s == -1 && input[i] == ' '){
					i+=1;
                    continue;
                }else if( s == 2 && checkDelim){
							t_cmd[count] = '\0';
							cmd->command_list[cmd_num] = t_cmd;
							cmd->args_list[cmd_num][arg_num] = t_cmd;
							arg_num+=1;
							cmd->num_args[cmd_num]+=1;
							cmd->num_commands+=1;
						//	printf("state = %d str = %s\n",state[0],t_cmd);
							i = findNextState(input,i,state,length);
							break;
					}else{
						s = 2;
						t_cmd[count] = input[i];
						i+=1;
						count+=1;
					}
				}
			}else if(state[0] == 6){
				int count = 0;
				t_args = (char *) malloc(sizeof(char)*MAX_LEN);
				int s = -1;
				if(input[i] == '"'){
					i+=1;
					s = 3;
				}
				while(1){
				if(s == -1 && input[i] == ' '){
								i+=1;
                                continue;
                }else if( s == 2 && checkDelim){
							t_args[count] = '\0';
							cmd->args_list[cmd_num][arg_num] = t_args;
							arg_num +=1;
							cmd->num_args[cmd_num]+=1;
						//	printf("state = %d str = %s\n",state[0],t_args);
							i = findNextState(input,i,state,length);
							break;
					}else if(s == 3 && input[i] == '"'){
							i+=1;
							t_args[count] = '\0';
							cmd->args_list[cmd_num][arg_num] = t_args;
							arg_num +=1;
							cmd->num_args[cmd_num]+=1;
					//		printf("state = %d str = %s\n",state[0],t_args);
							i = findNextState(input,i,state,length);
							break;
					}else if(s == 3){
						t_args[count] = input[i];
						i+=1;
						count+=1;
					}else{
						s = 2;
						t_args[count] = input[i];
						i+=1;
						count+=1;
					}
				}
			}else if(state[0] == 7){ // state with single pipe
				int pipefd[2];
				if(pipe(pipefd) < 0){
					perror("pipe");
					exit(-1);
				}else{
					
					if(cmd->pipe[cmd_num] == NOPIPE){
						cmd->pipe[cmd_num] = WPIPE;
						cmd->pipe[cmd_num+1] = RPIPE;
					}else if(cmd->pipe[cmd_num] == RPIPE){
						cmd->pipe[cmd_num] = RWPIPE;
						cmd->pipe[cmd_num+1] = RPIPE;
					}
					if(cmd->pipe[cmd_num] != OUT_REDIRECTED){
						cmd->output_fd[cmd_num] = pipefd[1];
						
					}else{
						close(pipefd[1]);
					}
					cmd_num+=1;
					arg_num = 0;
				
					cmd->input_fd[cmd_num] = pipefd[0];
					i = findNextState(input,i,state,length);
				}
			}else if(state[0] == 8){ // state with double pipes
				int pipefd[2];
				if(pipe(pipefd) < 0){
					perror("pipe");
					exit(-1);
				}else{
					if(cmd->pipe[cmd_num] != OUT_REDIRECTED){
						cmd->output_fd[cmd_num] = pipefd[1];
						cmd->pipe[cmd_num] = DPIPE;
					}else{
						close(pipefd[1]);
					}
					cmd->input_fd[cmd_num+1] = pipefd[0];
					cmd->input_fd[cmd_num+2] = pipefd[0];
					cmd_num+=1;
					arg_num = 0;
					i = findNextState(input,i,state,length);
				}
			}else if(state[0] == 9){ // state with triple pipes
				int pipefd[2];
				if(pipe(pipefd) < 0){
					perror("pipe");
					exit(-1);
				}else{
					if(cmd->pipe[cmd_num] != OUT_REDIRECTED){
						cmd->output_fd[cmd_num] = pipefd[1];
						cmd->pipe[cmd_num] = TPIPE;
					}else{
						close(pipefd[1]);
					}
					cmd->input_fd[cmd_num+1] = pipefd[0];
					cmd->input_fd[cmd_num+2] = pipefd[0];
					cmd->input_fd[cmd_num+3] = pipefd[0];
					cmd_num+=1;
					arg_num = 0;
					i = findNextState(input,i,state,length);
				}
			}else if(state[0] == 10){ // state with comma
					state[0] = 5;
					i+=1;
					cmd_num+=1;
					arg_num = 0;
			}
	}
	
	
}

void execute_cmd(struct command *cmd,int i,char **environ,char *path){
	
	int std_out = dup(1);
	int std_in = dup(0);
	int cpid;
	
	if((cpid = fork()) == 0){
			//printf("%d %d\n",cmd->output_fd[i],cmd->input_fd[i]);
			dup2(cmd->output_fd[i],1);	
			dup2(cmd->input_fd[i],0);
			if(cmd->pipe[i] == WPIPE){
				close(cmd->output_fd[i]);
			}else if(cmd->pipe[i] == RPIPE){
				close(cmd->input_fd[i]);
			}else if(cmd->pipe[i] == RWPIPE){
				close(cmd->input_fd[i]);
				close(cmd->output_fd[i]);
			}
		int ret;
		
		int length = strlen(path);
		char append_path[100];
		int j = 0;
		int count = 0;
		while(j < length){
				memset(append_path,'\0',100);
				count = 0;
				while(path[j] !=  ':' && j < length){
					append_path[count] = path[j];
					count+=1;
					j+=1;
				}
				j+=1;
				strcat(append_path,"/");
				strcat(append_path,cmd->command_list[i]);
			
				if((ret = execve(append_path,cmd->args_list[i],environ)) < 0){
					continue;
				}else{
					close(0);
					close(1);
					exit(0);
				}
			}
		
			if(ret < 0){
				perror("execve");
				exit(-1);
			}
		exit(0);
		}else if(cpid > 0){
			close(cmd->input_fd[i]);
			close(cmd->output_fd[i]);
			int status;
			wait(&status);
			dup2(std_out,1);
			dup2(std_in,0);
		}else{
	
		}
}

void execute(struct command *cmd,char **environ){
	int std_out = dup(1);
	int std_in = dup(0);
	char var_name[5];
	memset(var_name,'\0',5);
	strcpy(var_name,"PATH");
	
	char *path = getenv(var_name);
	//printf("%s\n",path);
	
	pid_t cpid;
	int i;
	for(i = 0; i < cmd->num_commands; i+=1){
		
		if(cmd->pipe[i] == DPIPE){ // if the command has a double pipe
			char buffer[2048];
			memset(buffer,'\0',2048);
			execute_cmd(cmd,i,environ,path);
			int size;
			i+=1;
			int out_fd = cmd->input_fd[i];
			if((size = read(out_fd,buffer,2048)) < 0){
				perror("read");
				exit(-1);
			}
			int pipefd[2];
			if(pipe(pipefd) < 0){
				perror("Pipe");
				exit(-1);
			}
			write(pipefd[1],buffer,2048);
			close(pipefd[1]);
			cmd->input_fd[i] = pipefd[0];
			execute_cmd(cmd,i,environ,path);
			//Execute second command in double pipe
			i+=1;
			int pipefd2[2];
			if(pipe(pipefd2) < 0){
				perror("Pipe");
				exit(-1);
			}
			write(pipefd2[1],buffer,2048);
			close(pipefd2[1]);
			cmd->input_fd[i] = pipefd2[0];
			execute_cmd(cmd,i,environ,path);
					
	}else if(cmd->pipe[i] == TPIPE){ // if the command has a triple pipe
			char buffer[2048];
			memset(buffer,'\0',2048);
			execute_cmd(cmd,i,environ,path);
			int size;
			i+=1;
			int out_fd = cmd->input_fd[i];
			if((size = read(out_fd,buffer,2048)) < 0){
				perror("read");
				exit(-1);
			}
			int pipefd[2];
			if(pipe(pipefd) < 0){
				perror("Pipe");
				exit(-1);
			}
			write(pipefd[1],buffer,2048);
			close(pipefd[1]);
			cmd->input_fd[i] = pipefd[0];
			execute_cmd(cmd,i,environ,path);
			//Execute second command in triple pipe
			i+=1;
			int pipefd2[2];
			if(pipe(pipefd2) < 0){
				perror("Pipe");
				exit(-1);
			}
			write(pipefd2[1],buffer,2048);
			close(pipefd2[1]);
			cmd->input_fd[i] = pipefd2[0];
			execute_cmd(cmd,i,environ,path);
			//Execute third command in triple pipe
			i+=1;
			int pipefd3[2];
			if(pipe(pipefd3) < 0){
				perror("Pipe");
				exit(-1);
			}
			write(pipefd3[1],buffer,2048);
			close(pipefd3[1]);
			cmd->input_fd[i] = pipefd3[0];
			execute_cmd(cmd,i,environ,path);
			
			
					
	}else{
		execute_cmd(cmd,i,environ,path);
	}
	}
		
}



int main()
{
    char input[MAX_LINE];
	extern char **environ;
    while (1)
    {
		struct command *cmd;
        cmd = (struct command *)malloc(sizeof(struct command));
		int size;
		cmd->num_commands = 0;
        print_prompt();	
        memset(input,'\0',MAX_LINE);	
        size = read_input(input);
		
		int i = 0;
		for(i = 0; i < MAX_COMMANDS; i+=1){
			cmd->output_fd[i] = 1;
			cmd->input_fd[i] = 0;
			cmd->pipe[i] = NOPIPE;
		}
		
        parse_input(input,cmd);
		//printf("%d\n",cmd->num_commands);
		i = 0;
		while(i < cmd->num_commands){
			//printf("cmd = %s \n",cmd->command_list[i]);
			//printf("output = %d,input = %d\n",cmd->output_fd[i],cmd->input_fd[i]);
			int j = 0;
			while(j < cmd->num_args[i]){
				//printf("\targs = %s\n",cmd->args_list[i][j]);
				j+=1;
			}
			cmd->args_list[i][j] = NULL;
			i+=1;
		}
		execute(cmd,environ);
		cmd = NULL;
		free(cmd);
		
        
    }
}