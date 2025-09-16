#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

char** paths = NULL;
int num_paths = 0;

void interactive_mode();
void batch_mode(char* file_name);
void handle_line_input(char* input);
void print_error();
void clean_user_input(char* input);
char** get_arguments(char* input, int* num_args);
void execute_command(char** arguments, int num_args);

int main(int MainArgc, char *MainArgv[]){
	
	// Initialise paths to just bin
	paths = malloc(2 * sizeof(char*));
	if(paths == NULL){
		print_error();
		exit(1);
	}

	num_paths = 1;
	paths[0] = strdup("/bin");
	paths[1] = NULL;
	

	if (MainArgc == 1){
		// Running in interactive mode
		interactive_mode();
	}
	else if (MainArgc == 2){
		// Running in batch mode
		batch_mode(MainArgv[1]);
	}
	else{
		// Too many args 
		print_error();
		return 1;
	}

	return(0);
}

void interactive_mode(){
	_Bool running = 1;
	char *line = NULL;
	size_t len = 0;

	while (running){
		printf("witsshell> ");
		fflush(stdout);
		
		if (getline(&line, &len, stdin) == -1){
			free(line);
			exit(0);

		}
		else{
			handle_line_input(line);
		} 
		
	}

	free(line);
}

void batch_mode(char* file_name){
	FILE *file = fopen(file_name, "r");
	if (file == NULL){
		print_error();
		exit(1);
	}

	char *line = NULL;
	size_t len = 0;

	while (getline(&line, &len, file) != -1){
		handle_line_input(line);
	}
	free(line);
	fclose(file);

}

void handle_line_input(char* input){
	// remove whitespace and newline
	clean_user_input(input);
		
	// Get list of arguments
	int num_args = 0;
	char** arguments = get_arguments(input, &num_args);

	if(num_args > 0){
		// Exit instruction
		if (strcmp(arguments[0], "exit") == 0){
			if(num_args == 1){
				free(arguments);
				
				// free paths
				for(int i = 0; i < num_paths; i++){
					free(paths[i]);
				}	
				free(paths);

				exit(0);
			}
			else{
				print_error();
			}
		}
		// cd .... instruction
		else if (strcmp(arguments[0], "cd") == 0){
			if(num_args == 2){
				if(chdir(arguments[1]) != 0){
					print_error(); // chdir failed
				}
			}
			else{
				print_error(); // incorrect number of arguments for cd
			}
		}
		// path ... instruction
		else if(strcmp(arguments[0], "path") == 0){
			for(int i = 0;  i < num_paths; i++){
				free(paths[i]);
			}
			free(paths);

			// new path list:
			num_paths = num_args - 1; // -1 for "path"
			paths = malloc((num_paths + 1) * 2 * sizeof(char*));
			if(paths == NULL){
				print_error();
				exit(1);
			}

			// copy new paths
			for(int i = 0; i < num_paths; i++){
				paths[i] = strdup(arguments[i+1]);
			}
			paths[num_paths] = NULL; // null terminator	
		}
		else{
			execute_command(arguments, num_args);
		}


	}
	else{
		free(arguments);
		return;
	}

	free(arguments);
}

// Clean user input by removing newline and white spaces
void clean_user_input(char* input){
	// remove new line
	input[strcspn(input, "\n")] = 0;
	//printf("Line before processing: [%s]\n", input);

	// Remove leading whitespace 
	size_t begin_iterator = 0, end_iterator = strlen(input) - 1;
	while (input[begin_iterator] == ' ' || input[begin_iterator] == '\t'){
		begin_iterator ++;
	}

	// Remove trailing whitespace 
	while (end_iterator > begin_iterator && (input[end_iterator] == ' ') || (input[end_iterator] == '\t')){
		end_iterator --;
	}

	size_t trimmed_length = end_iterator - begin_iterator + 1;
	memmove(input, input + begin_iterator, trimmed_length);
	
	input[trimmed_length] = '\0'; // update null terminating character to new sting length

	//printf("Line after processing: [%s]\n", input);
}

char** get_arguments(char* line, int* num_args){
	int max_arg_num = 10;
	char ** arguments = malloc(max_arg_num * sizeof(char*));

	// doubt this will ever happen - maybe remove this
	if (arguments == NULL){
		print_error();
		exit(1);
	}

	int argument_counter = 0;
	char* argument = strtok(line, " \t"); // maybe change delim to just " " -> noone would separate args by a tab

	while (argument != NULL){

		arguments[argument_counter] = argument;
		argument_counter ++;

		if(max_arg_num <= argument_counter){
			// we allocated too little space, increase the memory size for the arguments
			max_arg_num *= 2;	
			arguments = realloc(arguments, max_arg_num * sizeof(char*));

			// doubt this will ever happen, maybe remove this
			if (arguments == NULL){
				print_error();
				exit(1);
			}
		}
		argument = strtok(NULL, " \t");
	}

	arguments[argument_counter] = NULL;
	*num_args = argument_counter;
	return arguments;
}

void execute_command(char** arguments, int num_args){
	char* instruction = arguments[0];
	char constructed_path[2048];
	bool found = false;

	for(int i = 0; i < num_paths; i++){
		snprintf(constructed_path, sizeof(constructed_path), "%s/%s", paths[i], instruction);
	
		// Check if file exits and if we can execute
		if(access(constructed_path, X_OK) == 0){
			found = true;
			break;
		}
	}

	if(!found){
		print_error();
		return;
	}

	pid_t id = fork();
	if (id < 0){
		print_error();
		return;
	}
	else if(id == 0){
		execv(constructed_path, arguments);

		// if exec returns then we have an error
		print_error();
		_exit(1); // _exit for child process
	}
	else{
		int status;
		waitpid(id, &status, 0);
	}
}

void print_error(){
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}
