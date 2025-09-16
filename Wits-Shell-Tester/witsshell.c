#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

void interactive_mode();
void batch_mode(char* file_name);
void handle_line_input(char* input);
void print_error();
void clean_user_input(char* input);

int main(int MainArgc, char *MainArgv[]){
	
	if (MainArgc == 1){
		// Running in interactive mode
		interactive_mode();
	}
	else if (MainArgc == 2){
		// Running in batch mode
		batch_mode(MainArgv[1]);
	}
	else{
		// Too many args throw an error
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
	_Bool running =1;
	FILE *input_steam = NULL;

	while (running){

	}

}

void handle_line_input(char* input){
	// remove whitespace and newline
	clean_user_input(input);
		
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

void print_error(){
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}
