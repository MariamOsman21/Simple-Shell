#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h> 
#include <sys/wait.h>
#include <signal.h>
#include <sys/resource.h>
#include <ctype.h>
#define MAX_BUFFER_SIZE 1000

char VARIABLES[MAX_BUFFER_SIZE][500];
char VALUES[MAX_BUFFER_SIZE][500];
int BUFFERSIZE = 0;
char command[100][500];
int commands_num = 0;
int fileFlag = 0;
void on_child_exit(int sig);
void execute_command( int is_background_process);
void cd(char *path);
void handle_dolar_sign();
void parse_input();
void export();
void echo();
void shell();
int shell_builtin();
void setup_environment();
void write_to_log_file();

int main(int argc, char* argv[]){
    //signal termination of the child
    signal(SIGCHLD, on_child_exit);
    setup_environment();
    shell();  
    return 0;  
}
void setup_environment(){
    char directory[200];
    chdir(getcwd(directory, 200));
}
void cd(char *path);
void handle_dolar_sign();
void parse_input(){
   char input[500];
   fgets(input,500, stdin);
    char* token = strtok(input, " ");
    commands_num = 0;
    while (token != NULL){
        strcpy(command[commands_num], token);
        //remove trailing line
        command[commands_num][strcspn(command[commands_num], "\n")] = 0;
        token = strtok(NULL, " ");
        commands_num++;
    }
    // remove quotations in case of echo
    if(command[1][0] == '"'){
        memmove(command[1], command[1]+1, strlen(command[1]));
        if(commands_num == 1){
            int size = strlen(command[1]);
            command[1][size-1] = '\0';
        }else{
            int size = strlen(command[commands_num-1]);
            command[commands_num-1][size-1] = '\0';
        }
    }
    handle_dolar_sign();  
}
void shell(){
    //char *user = getlogin();
    printf("user@user:~$ ");
    while(1){
        parse_input();
        if(strcmp(command[0], "exit")==0) break;
        if(shell_builtin())
            continue;
        else{
            if(strcmp(command[commands_num-1], "&") == 0){
                execute_command(1);

            }else{
                execute_command(0);
            }
        }
    }
}
void handle_dolar_sign(){
    for(int i = 1; i < commands_num; i++){
        int found = 0;
        
        if(command[i][0] == '$'){
            memmove(command[i], command[i]+1, strlen(command[i]));
            for(int j = 0; j < MAX_BUFFER_SIZE; j++){
                // replace dolar sign if found
               if(strcmp(VARIABLES[j],command[i]) == 0){
                   strcpy(command[i],VALUES[j]);
                   found = 1;
                   break;
                }
            }
            if(!found) 
                strcpy(command[i], "");
        }
    }
    
}
void export(){
    if(strchr(command[1], '=') == NULL) return;
    char variable[500];
    strcpy(variable, command[1]);
    //printf("%s", variable);
    char *token = strtok(variable ,"=");
    strcpy(VARIABLES[BUFFERSIZE], token);
   // printf("%s \n", VARIABLES[BUFFERSIZE]);
    token = strtok(NULL, "=");
    strcpy(VALUES[BUFFERSIZE] , token);
    for(int i = 2; i < commands_num; i++){
        strcat(VALUES[BUFFERSIZE], " ");
        strcat(VALUES[BUFFERSIZE], command[i]);
       
    }
    if(VALUES[BUFFERSIZE][0] == '"'){
        memmove(VALUES[BUFFERSIZE], VALUES[BUFFERSIZE]+1, strlen(VALUES[BUFFERSIZE]));
        int size = strlen(VALUES[BUFFERSIZE]);
        VALUES[BUFFERSIZE][size-1] = '\0';
    }
    //printf("%s",VALUES[BUFFERSIZE]);
    
    BUFFERSIZE++;    
}
void echo(){
    if(commands_num > 1){ 
    }
    for(int i = 1; i < commands_num; i++){
        printf("%s ", command[i]);
    }
    printf("\n");
    
}
int shell_builtin(){
    if(strcmp(command[0], "cd") == 0){
        cd(command[1]);
        return 1;
    }
    else if(strcmp(command[0], "export") == 0){
        export();
        return 1;

    }else if(strcmp(command[0], "echo") == 0){
        echo();
        return 1;
    }
    return 0;
}

void on_child_exit(int sig){
    int status;
    int pid;
    // reap zombie process
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    }
   write_to_log_file(); 
}
void execute_command(int is_background_process){
    char *argumentlist[500];
        int i =0;
    while (i != 500){
        argumentlist[i++] = NULL;
    }
    int n = commands_num;
    // remove & char from end
    if(is_background_process) n = n-1;
    int j = 0;
    for (int i = 0; i < n; i++){
        if(command[i] != NULL){
            char *token = strtok(command[i], " ");
            while(token != NULL){
            argumentlist[j] = token;
            token = strtok(NULL, " ");
            j++;
            }
           // printf("%s ", argumentlist[j]);};
        
        }
        else break;
    }
    int pid = fork();
    if(pid == 0){
        if(execvp(command[0], argumentlist) == -1){
            printf("Error\n");
            exit(EXIT_FAILURE);
        }  
        exit(0);    
    } 
    else{
        if(is_background_process == 0){
            waitpid(pid, NULL, 0);
            return;
        }       
    }
}
void cd(char *path){
    //home directory
    if(strcmp("~", path) == 0 || commands_num == 1){
        chdir(getenv("HOME"));
    //other directories
    }else {
        if(chdir(path) == -1){
            perror("No such directory"); 
        }
    }
}
void write_to_log_file(){
    FILE *fptr;
    if(!fileFlag){
        fptr = fopen("children.txt","w");
        fileFlag = 1;
    }
   fptr = fopen("children.txt","a");
   if(fptr == NULL)
   {
      printf("Error!");   
      exit(1);             
   }
   fputs("Child Terminated.\n",fptr);
   fclose(fptr);
}
