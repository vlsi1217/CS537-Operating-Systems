#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int in;

void printError()
{
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}

void runBinCommand(char * argv[], const char* loc)
{
	execvp(loc, argv);
	printError();
	exit(0);
}

void runCd(char* argv[], int size)
{
	if(size == 1)
	{	
		char* homeDir = getenv("HOME");
		if(chdir(homeDir) != 0)
		{
			printError();	
		}				
	}
	else if(size == 2)
	{
		if(chdir(argv[1]) != 0)
		{
			printError();	
		}		
	}
	else
	{
		printError();
	}	
}

int isFilePython(char* fileName)
{
	if(strstr(fileName, ".py") != NULL)
	{
		return 1;
	}
	return 0;
}

void runPwd(char* argv[], int size, int redirect)
{
	if(size != 1 && !redirect)
	{
		printError();
		return;
	}
	char cwd[1000];
	if(getcwd(cwd, 1000) == NULL)
	{
		printError();
		return;
	}
	int length = strlen(cwd);
	cwd[length] = '\n';
	cwd[length + 1] = '\0';
	write(STDOUT_FILENO, cwd, strlen(cwd));
}


int isSpecialExist(char* str)
{
	char* target = strchr(str, '>');
	if(target != NULL)
	{
		return 1;
	}
	return 0;
}

int countSpecialChar(char* args[1000], int size)
{
	char* target;
	int i;
	int count = 0;
	for(i = 0; i < size; i++)
	{
		target = strchr(args[i], '>');
		if(target != NULL)
		{
			count++;
		}
	}
	return count;
}


void runCommand(char* args[10000], int size, int redirect, int background, int runPy)
{
	char* argv[size + 1];
	int i;
	for(i = 0; i < size; i++)
	{
		argv[i] = args[i];
	}
	argv[i] = NULL;
	fpos_t pos;
	fgetpos(stdout, &pos);
	int d = dup(fileno(stdout));
	if(redirect)
	{
		
		close(STDOUT_FILENO);
		int fd = open(argv[i - 1], O_CREAT | O_TRUNC | O_RDWR, 0777);
		argv[i - 1] = NULL;
		if(fd == -1)
		{
			printError();
			return;
		}
	}
	if(strcmp(args[0], "exit") == 0)
	{
		if(size > 1)
		{
			printError();
		}
		exit(0);
	} 
	else if(strcmp(args[0], "cd") == 0)
	{
		runCd(argv, size);
	}
	else if (strcmp(args[0], "pwd") == 0)
	{
		runPwd(argv, size, redirect);
	}
	else if(strcmp(args[0], "wait") == 0)
	{
		if(size > 1)
		{
			printError();
			return;
		}
		pid_t pid;
		while (pid = waitpid(-1, NULL, 0)) {
   			if (errno == ECHILD) {
      				break;
   			}
		}
	}
	else
	{
		int pid = fork();
		if(pid >= 0)
		{
			//child process
			if(pid == 0)
			{
				if(runPy)
				{
					runBinCommand(argv, "/usr/bin/python");
				}
				else
				{
					runBinCommand(argv, args[0]);
				}
				
			}	
			else		//parent process
			{
				
				if(!background)
				{
					wait(NULL);		
				}
			}
		}	
		else
		{
			printError();
			exit(0);
		}
	}
	//restore the stdout
	if(redirect)
	{
		fflush(stdout);
		dup2(d, fileno(stdout));
		close(d);
		clearerr(stdout);
		fsetpos(stdout, &pos);	
	}
}

int main(int argc, char const *argv[])
{
	
	if(argc != 2 && argc != 1)
	{
		printError();
		exit(1);
	}
	if(argc == 2)
	{
		close(fileno(stdin));
		FILE* in = fopen(argv[1], "r");
		if(in == NULL)
		{
			printError();
			exit(1);
		}
		dup2(fileno(in), STDIN_FILENO);
	}
	while(1)
	{
		char prompt[7] = "" ;
		if(argc != 2)
		{
			strcat(prompt, "mysh> ");
		}
		char line[10000];
		write(STDOUT_FILENO, prompt, strlen(prompt));
		if(fgets(line, 10000, stdin) != NULL)
		{	
			if(strcmp(line, "\n") == 0)
			{
				break;
			}
			if(argc == 2)
			{
				write(STDOUT_FILENO, line, strlen(line));
			}
			//check the length requirement
			if(strlen(line) > 513)
			{
				printError();
				continue;
			}
			char* cleandLine = strtok(line, "\n");
			if(cleandLine == NULL)
			{
				continue;
			}
			char* special = strchr(cleandLine, '>');
			int redirect, background = 0, runPy = 0;
			if(special != NULL)
			{
				*(special++) = ' ';
				redirect = 1;
				if(isSpecialExist(special))
				{
					printError();
					continue;
				}
				char* r = strtok(special, " ");
				if(r == NULL)
				{
					printError();
					continue;
				}
				int count = 0;
				while(r != NULL)
				{
					if(strcmp(r, "&") != 0)
					{
						count++;
					}
					r = strtok(NULL, " ");
				}
				if(count > 1)
				{
					printError();
					continue;		
				}
				
			}
			else	
			{
				redirect = 0;
			}
			char delims[]= " ";
			char tabDelims[] = "\t";
			char* space, *clean;
			char* result = strtok_r(cleandLine, delims, &space);
			char* args[10000];
			int size = 0;
			while(result != NULL)
			{
				char* tab;
				clean = strtok_r(result, tabDelims, &tab);
				while(clean != NULL)
				{	
					args[size++] = clean;
					clean = strtok_r(NULL, tabDelims, &tab);
				}
				result = strtok_r(NULL, delims, &space);
			}
			char* gb = strchr(args[size - 1], '&');
			if(gb != NULL)
			{
				background = 1;
				if(strcmp(gb, "&"))
				{
					size--;
				}
				else
				{		
					*gb = '\0';
				}	
			}
			if(isFilePython(args[0]))
			{
				int i;
				for(i = size; i > 0; i--)
				{
					args[i] = args[i - 1];
				}
				args[0] = "python";
				runPy = 1;
				size++;
			}
			runCommand(args, size, redirect, background, runPy);		
		}
		else
		{
			break;
		}
	}
	return 0;
}
