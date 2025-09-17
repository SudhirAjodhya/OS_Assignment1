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
void execute_command(char** arguments, int num_args, char* redirection_file);
void process_command(char* commands);

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
	bool running = 1;
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
    char* input_copy = strdup(input);
    clean_user_input(input_copy);

    if(strlen(input_copy) == 0){
        free(input_copy);
        return;
    }

    char* temp_input = strdup(input_copy);
    int num_args = 0;
    char** arguments = get_arguments(temp_input, &num_args);

    if (num_args > 0){
		// Exit 
        if (strcmp(arguments[0], "exit") == 0){
            if (num_args == 1) {
                free(arguments);
                free(temp_input);
                free(input_copy);

                for (int i= 0; i < num_paths; i++){
					free(paths[i]);	
				} 
                free(paths);
                exit(0);
            } 
			else{
				print_error();
            }

            free(arguments);
            free(temp_input);
            free(input_copy);
            return;
        } 
		// CD
		else if(strcmp(arguments[0], "cd") == 0){
            if (num_args == 2){
				if (chdir(arguments[1]) != 0){
					print_error();
				}
            } 
			else{
                print_error();
            }

            free(arguments);
            free(temp_input);
            free(input_copy);
            return;
        } 
		// PATH
		else if(strcmp(arguments[0], "path") == 0){
            for (int i = 0; i < num_paths; i++){
				free(paths[i]);
			}
            free(paths);


            num_paths = num_args - 1;
            paths = malloc((num_paths + 1) * sizeof(char*));

            for (int i = 0; i < num_paths; i++){
				paths[i] = strdup(arguments[i + 1]);	
			} 

            paths[num_paths] = NULL;
            
            free(arguments);
            free(temp_input);
            free(input_copy);
            return;
        }
    }
    free(arguments);
    free(temp_input);


	// Parallel processing
    char* commands[128];
    int num_commands = 0;
    char* command = strtok(input_copy, "&");
    while (command != NULL){
        commands[num_commands++] = command;
        command = strtok(NULL, "&");
    }

    pid_t ids[num_commands];
    for (int i = 0; i < num_commands; i++){
		
        pid_t id = fork();
        if (id < 0) {
            print_error();
            exit(1);
        } 
		else if (id == 0){ 
            process_command(commands[i]);
            exit(0); 
        }
		else{ 
            ids[i] = id;
        }
    }

	// wait for children to complete
    for (int i = 0; i < num_commands; i++){
        waitpid(ids[i], NULL, 0);
    }
    
    free(input_copy);
}

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
			// allocated too little space, increase the memory size for the arguments
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

void execute_command(char** arguments, int num_args, char* redirection_file){
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

	if(redirection_file != NULL){
		int file = open(redirection_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

		if(file == -1){
			print_error();
			_exit(1);
		}
		dup2(file, STDOUT_FILENO);
		dup2(file, STDERR_FILENO);
		close(file);
	}
	execv(constructed_path, arguments);
	print_error();
	_exit(1);
}

void process_command(char* commands){
	char* redirection_file = NULL;
	char* redirection_symbol = strchr(commands, '>');

	if(redirection_symbol != NULL){
		*redirection_symbol = '\0';
		char* file = redirection_symbol + 1;

		while(*file == ' ' || *file == '\t'){
			file ++;	
		} 

		char* end = file + strlen(file) - 1;
		while (end > file && (*end == ' ' || *end == '\t')){
			end --;
		}

		*(end + 1) = '\0';

		if(strlen(file) == 0 || strchr(file, ' ') || strchr(file, '\t') || strchr(file, '>')){
			print_error();
			return;
		}
		redirection_file = file;
	}

	int num_args = 0;
	char** arguments = get_arguments(commands, &num_args);
	if(num_args == 0 ){
		if(redirection_symbol != NULL){
			print_error();
		}
		free(arguments);
		return;
	}

	execute_command(arguments, num_args, redirection_file);
	free(arguments);
}

void print_error(){
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}
