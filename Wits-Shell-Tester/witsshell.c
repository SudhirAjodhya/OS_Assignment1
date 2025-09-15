#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

void interactive_mode();
void batch_mode();
void print_error();


int main(int MainArgc, char *MainArgv[]){
	
	if (MainArgc == 1){
		// Running in interactive mode
		interactive_mode();
	}
	else if (MainArgc == 2){
		// Running in batch mode
		batch_mode();
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

	printf("witsshell>\n");
	while (running){
		
	}
}

void batch_mode(){

}

void print_error(){
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}
