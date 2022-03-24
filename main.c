#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/resource.h>
#include <ctype.h>
#define MAX_BUFFER_SIZE 1000

char VARIABLES[MAX_BUFFER_SIZE][500];
char VALUES[MAX_BUFFER_SIZE][500];
int BUFFERSIZE = 0;
char command[500][500];
int commands_num = 0;
int fileFlag = 0;
void on_child_exit(int sig);
void execute_command(int is_background_process);
void cd(char *path);
void handle_dollar_sign();
void parse_input();
void export();
void echo();
void shell();
int shell_builtin();
void setup_environment();
void write_to_log_file();
void cd(char *path);
void handle_dolar_sign();

int main(int argc, char *argv[]){
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

/*
function to parse and format the command.
*/
void parse_input(){
    memset(command, '\0', sizeof(command));
    char input[500];
    fgets(input, 500, stdin);
    //split input by spaces.
    char *token = strtok(input, " ");
    commands_num = 0;
    while (token != NULL){
        strcpy(command[commands_num], token);
        //remove trailing line.
        command[commands_num][strcspn(command[commands_num], "\n")] = 0;
        token = strtok(NULL, " ");
        commands_num++;
    }
    //remove quotations if exists.
    if (command[1][0] == '"'){
        memmove(command[1], command[1] + 1, strlen(command[1]));
        if (commands_num == 1){
            int size = strlen(command[1]);
            command[1][size - 1] = '\0';
        }
        else{
            int size = strlen(command[commands_num - 1]);
            command[commands_num - 1][size - 1] = '\0';
        }
    }
    handle_dollar_sign();
}
/*
function to execute the shell.
*/
void shell(){ 
    while (1){
        printf("user@user:~$ ");
        parse_input();
        if(strcmp(command[0], "") == 0) continue; 
        if (strcmp(command[0], "exit") == 0)
            exit(0);
        if (shell_builtin())
            continue;
        else{
            if (strcmp(command[commands_num - 1], "&") == 0){
                //background processes.
                execute_command(1);
            }
            else{
                //foreground processes.
                execute_command(0);
            }
        }
    }
}

/*
function to replace the dollar signs 
by the values of the saved variables.
*/
void handle_dollar_sign(){
    //iterate all commands to find dollar sign.
    for (int i = 0; i < commands_num; i++){
        int found = 0;
        int indx =-1;
        //find the index of the $.
        for(int k = 0; k < strlen(command[i]); k++){
            if (command[i][k] == '$'){
                indx = k;
            }
        }
        if (indx != -1){
            char left_string[500];
            char right_string[500];
            //copy the string in the left of the dollar sign.
            memset(left_string, '\0', sizeof(left_string));
            strncpy(left_string, command[i], indx);
            memset(right_string, '\0', sizeof(right_string));
            //copy the string in the right of the dollar sign.
            strncpy(right_string, command[i] + indx + 1, strlen(command[i])-indx-1);
            for (int j = 0; j < MAX_BUFFER_SIZE; j++){
                // iterate through saved variable and compare them to the right string.
                if (strcmp(VARIABLES[j], right_string) == 0){
                    //copy the value to the right string.
                    strcpy(right_string, VALUES[j]);
                    found = 1;
                    //concatenate the right string to the left string.
                    strcat(left_string, right_string);
                    //copy the concatenated string to the command again.
                    strcpy(command[i], left_string);
                    break;
                }
            }
            if (!found){
                strcpy(right_string, "");
                //copy only the left string to the command.
                strcpy(command[i], left_string);
            }
        }
    }
}

/*
function that implement export command.
*/
void export(){
    //if the variable does not conatain "=" return.
    if (strchr(command[1], '=') == NULL)
        return;
    char variable[500];
    strcpy(variable, command[1]);
    // split the variable by the "=" sign.
    char *token = strtok(variable, "=");
    //copy the first element to variables.
    strcpy(VARIABLES[BUFFERSIZE], token);
    token = strtok(NULL, "=");
    // copy the second element to the values.
    strcpy(VALUES[BUFFERSIZE], token);
    //iterate through command array to concatenate the other parts of the value.
    for (int i = 2; i < commands_num; i++){
        strcat(VALUES[BUFFERSIZE], " ");
        strcat(VALUES[BUFFERSIZE], command[i]);
    }
    //remove qoutations.
    if (VALUES[BUFFERSIZE][0] == '"'){
        memmove(VALUES[BUFFERSIZE], VALUES[BUFFERSIZE] + 1, strlen(VALUES[BUFFERSIZE]));
        int size = strlen(VALUES[BUFFERSIZE]);
        VALUES[BUFFERSIZE][size - 1] = '\0';
    }
    //increase buffer size.
    BUFFERSIZE++;
}

/*
function that implement echo command and print values on the terminal.
*/
void echo(){
    for (int i = 1; i < commands_num; i++){
        printf("%s ", command[i]);
    }
    printf("\n");
}

/*
function that checks if the command is built in and then executes it.
return: 1 if built in , 0 otherwise.
*/
int shell_builtin(){
    if (strcmp(command[0], "cd") == 0){
        cd(command[1]);
        return 1;
    }
    else if (strcmp(command[0], "export") == 0){
        export();
        return 1;
    }
    else if (strcmp(command[0], "echo") == 0){
        echo();
        return 1;
    }
    return 0;
}

/*
signal handler function.
*/
void on_child_exit(int sig){
    int status;
    int pid;
    // reap zombie process
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0){}
    write_to_log_file();
}

/*
function to execute process for commands.
*/
void execute_command(int is_background_process){
    char *argumentlist[500];
    int i = 0;
    while (i != 500){
        argumentlist[i++] = NULL;
    }
    int n = commands_num;
    // if background process , remove & char from end.
    if (is_background_process)
        n = n - 1;
    int j = 0;
    for (int i = 0; i < n; i++){
        if (command[i] != NULL){
            char *token = strtok(command[i], " ");
            while (token != NULL){
                argumentlist[j] = token;
                token = strtok(NULL, " ");
                j++;
            }
        }
        else
            break;
    }
    //create child process.
    int pid = fork();
    if (pid == 0){
        if (execvp(command[0], argumentlist) == -1){
            printf("Error\n");
            exit(EXIT_FAILURE);
        }
        exit(0);
    }
    else{
        //wait on foreground process only.
        if (is_background_process == 0){
            waitpid(pid, NULL, 0);
            return;
        }
    }
}
/*
function that change the directory of the working space.
Parameters : string: directory path.
*/
void cd(char *path){
    // case home directory
    if (strcmp("~", path) == 0 || strcmp(command[1],"") == 0){
        chdir(getenv("HOME"));
    // other directories
    }
    else{
        if (chdir(path) == -1){
            perror("No such directory");
        }
    }
}
/*
function to check children termination into a file.
*/
void write_to_log_file(){
    FILE *fptr;
    if (!fileFlag){
        fptr = fopen("children.txt", "w");
        fileFlag = 1;
    }
    fptr = fopen("children.txt", "a");
    if (fptr == NULL){
        printf("Error!");
        exit(1);
    }
    fputs("Child Terminated.\n", fptr);
    fclose(fptr);
}
