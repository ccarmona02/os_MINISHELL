//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>


// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param)
{
	printf("****  Exiting MSH **** \n");
	//signal(SIGINT, siginthandler);
	exit(0);
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
	//reset first
	for(int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for ( i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
	/**** Do not delete this code.****/
	int end = 0; 
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];

	if (!isatty(STDIN_FILENO)) {
		cmd_line = (char*)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF){
			if(strlen(cmd_line) <= 0) return 0;
			cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush (stdin);
			fflush(stdout);
		}
	}

	/*********************************/

	char ***argvv = NULL;
	int num_commands;


	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		// Prompt 
		write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));
		
		// Get command
		//********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
		executed_cmd_lines++;
		if( end != 0 && executed_cmd_lines < end) {
			command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
		}
		else if( end != 0 && executed_cmd_lines == end) {
			return 0;
		}
		else {
			command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
		}
		//************************************************************************************************
		// We start declaring some variables that we will be using
		int pid, fd[2], fdr, stdin_fd, stdout_fd, stderr_fd, sum, tot;
		getCompleteCommand(argvv, num_commands);
		// Now we check if the user is calling the mycalc internal command
		if ((strcmp(argv_execvp[0], "mycalc")==0)&&(command_counter==1)){
			//Check that the structure of the command is correct
			if ((argv_execvp[4]== NULL)&&(argv_execvp[1]!= NULL)&&(argv_execvp[3]!= NULL)){
				// Select the between the add or mod operation
				if (strcmp(argv_execvp[2], "add")==0){
					// Extract the numbers to be added and compute the sum
					int operand1= atoi(argv_execvp[1]);
					int operand2= atoi(argv_execvp[3]);
					sum=operand1 + operand2;
					tot+= sum;
					// Create the environment value
					char arr[20];
					sprintf(arr, "%d", tot);
					const char *var=arr;
					if (setenv("Acc", var, 1)<0){
						perror("Error in the environment value");
					}
					char add[100];
					// Print the result
					snprintf(add, 100, "[OK] %d + %d = %d; Acc %s\n", operand1, operand2, sum, getenv("Acc"));
					int nwrite=write(STDERR_FILENO, add, strlen(add));
					if (nwrite < 0){
						perror("Error in writing result");
					}
				}else if (strcmp(argv_execvp[2], "mod")==0){
					// Extract the numbers and compute the division
					int operand1= atoi(argv_execvp[1]);
					int operand2= atoi(argv_execvp[3]);
					int divide= operand1%operand2;
					int quotient= abs(operand1/operand2);
					char add[100];
					// Print the result
					snprintf(add, 100,"[OK] %d %% %d = %d; Quotient %d \n", operand1, operand2, divide, quotient);
					int nwrite=write(STDERR_FILENO, add, strlen(add));
					if (nwrite < 0){
						perror("Error in writing result");
					}
				}else{ // WRONG SYNTAX, there is no add/mod operator
					char add[100];
					snprintf(add,100,"[ERROR] The structure of the command is mycalc <operand_1> <add/mod> <operand_2> \n");
					int nwrite = write(STDERR_FILENO, add, strlen(add));
					if (nwrite < 0){
						perror("Error in writing result");
					}
				}
			}else{ // WRONG SYNTAX, incorrect arguments
				if((write(STDERR_FILENO, "[ERROR] The structure of the command is: mycalc <operand_1> <add/mod> <operand_2> \n", strlen("[ERROR] The structure of the command is: mycalc <operand_1> <add/mod> <operand_2> \n")))< 0){
						perror("Error in writing result");
				}
				
			}
		// Check if the user is calling the mycp internal command
		}else if ((strcmp(argv_execvp[0], "mycp")==0)&&(command_counter==1)) {
			// Verify the command structure is valid
			if ((argv_execvp[1]!= NULL)&&(argv_execvp[2]!= NULL)){
				int fd,fd2, nread;
				char buffer[1024];
				// Open both files and check if there is an error while opening
				fd= open(argv_execvp[1], O_RDONLY, 0666);
				fd2= open(argv_execvp[2], O_WRONLY|O_CREAT, 0644);
				if ((fd<0) || (fd2<0)){
					if((write(STDOUT_FILENO, "[ERROR] Error opening original file\n", strlen("[ERROR] Error opening original file\n")))< 0){
						perror("Error in writing result");
					}
					return -1;
				}	
				// Copy the content of the original file and write it in the new one
				while ((nread=read(fd, buffer, 1024))>0){
					if (write(fd2, buffer, nread) < nread){
						if (close(fd) < 0){
							perror("[ERROR]Error when closing the copied file");
							return -1;
						}
					}
				}
				// Check if there has been an error
				if (nread < 0){
					perror("Error when reading the file");
					return -1;
				}
				if (close(fd) < 0){
					perror("[ERROR]Error when closing the copied file");
					return -1;
				}
				if (close(fd2) < 0){
					perror("[ERROR]Error when closing the copied file");
					return -1;
				}
				// If no error occured, print the result
				char add[300];
				snprintf(add, 300,"[OK] Copy has been successful between %s and %s \n", argv_execvp[1], argv_execvp[2]);
				if ((write(STDERR_FILENO, add, strlen(add))) < 0){
					perror("Error in writing result");
				}

			}else{	// WRONG SYNTAX, incorrect arguments
				if((write(STDOUT_FILENO, "[ERROR] The structure of the command is mycp <original_file> <copied_file> \n", strlen("[ERROR] The structure of the command is mycp <original_file> <copied_file> \n")))< 0){
						perror("Error in writing result");
					}
			}
		// If no internal command is called, proceed to execute commands or a sequence of them
		}else{
			// Start by saving the original standard file descriptors in order to restore them in the future
			stderr_fd = dup(STDERR_FILENO);
			if (stderr_fd < 0){
				perror("Error while duplication");
				exit(-1);
			}
			stdin_fd = dup(STDIN_FILENO);
			if (stdin_fd < 0){
				perror("Error while duplication");
				exit(-1);
			}
			stdout_fd = dup(STDOUT_FILENO);
			if (stdout_fd < 0){
				perror("Error while duplication");
				exit(-1);
			}
			// If introduced an error redirection, redirect it for all commands
			if(strcmp(filev[2],"0")!=0){
				if((close(STDERR_FILENO)) < 0 ){
					perror("Error while closing file descriptor");
					exit(-1);
				}
				fdr = open(filev[2], O_CREAT | O_WRONLY, 0666);
			}
			// Enter the loop which will run every command
			for (int i = 0; i < command_counter; i++){
				// If introduced an input redirection, redirect it for only the first command
				if ((i == 0) && (strcmp(filev[0],"0")!=0)){
					if((close(STDIN_FILENO)) < 0 ){
						perror("Error while closing file descriptor");
						exit(-1);
					}
					fdr = open(filev[0], O_RDONLY);
				}
				// If introduced an output redirection, redirect it for only the last command
				if ((i == command_counter-1) && (strcmp(filev[1],"0")!=0)){
					if((close(STDOUT_FILENO)) < 0 ){
						perror("Error while closing file descriptor");
						exit(-1);
					}
					fdr = open(filev[1], O_CREAT | O_WRONLY, 0666);
				}
				// Create the pipe and check if it fails
				if (pipe(fd)<0){
					perror("Error pipe");
					exit(-1);
				}
				// Use fork in order to create a child process
				pid = fork();
				switch (pid){
					case -1: // In case an error occurs
						perror ("Error in fork");
						return -1;
					case 0: // Child process
						// Redirect the output to the next command and check for errors
						if ((i < command_counter-1)&&(command_counter > 1)){ 
							if((close(STDOUT_FILENO)) < 0 ){
								perror("Error while closing file descriptor");
								exit(-1);
							}
							if((dup(fd[STDOUT_FILENO])) < 0){
								perror("Error while duplication");
								exit(-1);
							}
							if((close(fd[STDOUT_FILENO])) < 0 ){
								perror("Error while closing file descriptor");
								exit(-1);
							}
							if((close(fd[STDIN_FILENO])) < 0 ){
								perror("Error while closing file descriptor");
								exit(-1);
							}
						}
						// Execute the command and go exit child process
						execvp(argvv[i][0], argvv[i]);
						perror ("Error in exec");
						exit(pid);
					default: // Parent process
						// Parent process will wait for child in order of in_background value
						if (in_background == 0){
							while ( wait(&status) != pid );
						}
						// Redirect the input argument and check for errors
						if ((i < command_counter-1)&&(command_counter > 1)){  
							if((close(STDIN_FILENO)) < 0 ){
								perror("Error while closing file descriptor");
								exit(-1);
							}
							if((dup(fd[STDIN_FILENO])) < 0){
								perror("Error while duplication");
								exit(-1);
							}
							if((close(fd[STDIN_FILENO])) < 0 ){
								perror("Error while closing file descriptor");
								exit(-1);
							}
							if((close(fd[STDOUT_FILENO])) < 0 ){
								perror("Error while closing file descriptor");
								exit(-1);
							}
						}	
				}
			}
			// Restore all original standard file descriptors
			dup2(stdin_fd, 0);
			dup2(stdout_fd, 1);
			dup2(stderr_fd, 2);
		}
	}
	
	return 0;
}
