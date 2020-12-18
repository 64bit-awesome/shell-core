/**
 * shell-core (sc)
 * An extendible Linux / Unix shell with pipe and redirection support.
 * 
 * @author Jordy Araujo <jordy.araujo@hotmail.com>
*/

#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include<sys/wait.h> 

#include <limits.h>

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define DEBUG 1
#define MAX_TOKENS 25

typedef struct // holds information about the current command-line.
{
    int npipes; // the number of pipes found in cmdline.
    int ntokens; // the number of tokens found in cmdline.
    char line[BUFSIZ]; // the full command-line.
    char* tokens[MAX_TOKENS]; // ->s to the tokens in cmdline.
} CmdLine;

void free_strings(char* ptrArray[], int length);
void spawn(CmdLine* cmdline, int* fdd, int pipes, int executableIndex);
void tokenize(CmdLine* cmdline);
void execute(CmdLine* cmdline);

int main (int argc, char* argv [])
{
    CmdLine* cmdline = malloc(sizeof(CmdLine)); // the current command-line.
    for(;;) // while the user wants to execute more commands:
    {
        char* currentFolder; // -> to the current folder in cwd.
        char cwd[PATH_MAX + 1]; // the current working directory (cwd).

        // get current working directory:
        if(getcwd(cwd, sizeof(cwd)) == NULL)
        {   // if here, getcwd failed:
            fprintf(stderr, "internal-error: ");
            fprintf(stderr, "\n\tgetcwd: failed.\n");
            
            free(cmdline);
            exit(-1);
        }

        // get the last folder in the cwd:
        if((currentFolder = strrchr(cwd, '/')) == NULL)
        {   // if here, could not find / in directory:
            fprintf(stderr, "internal-error: ");
            fprintf(stderr, "\n\tstrrchr: failed to find current folder in %s.\n", cwd);
            
            free(cmdline);
            exit(-1);
        }

        char* homeDirectory; // home directory of current user.

        // get the home directory of the user:
        if ((homeDirectory = getenv("HOME")) == NULL) 
        {
            homeDirectory = getpwuid(getuid())->pw_dir;
        }

        uid_t euid = geteuid(); // the effective user id of this process.
        
        printf("[shell-core %s] %s ", 
            ((!strcmp(homeDirectory, cwd)) ? "~" : ++currentFolder), // display ~ or last folder in cwd.
            ((euid == 0) ? "#" : "$")); // display # if super user or $ if regular user.
        
        if(fgets(cmdline->line, BUFSIZ, stdin) == NULL) 
        {
            fprintf(stderr, "internal-error: ");
            fprintf(stderr, "\n\tfgets: failed.\n");
            
            free(cmdline);
            exit(-1);
        }

        execute(cmdline);
    }
    
    free(cmdline);
    return 0; 
}

/**
 * Executes a command-line.
 * @param cmdline the current command-line information.
*/
void execute(CmdLine* cmdline)
{
    tokenize(cmdline);

    if(cmdline->ntokens <= 0) return; // nothing to execute.
    
    // check if user wants to leave the shell:
    if(!strcmp(cmdline->tokens[0], "quit") || !strcmp(cmdline->tokens[0], "exit")) 
    {
        free(cmdline);
        exit(0); // clean exit.
    }
    
    int i; // index of the current token being processed.
    int pipes = 0; // number of pipes processed.
    int executableIndex = 0; // index of the executable program in cmdline.
    int lastPipeIndex = -1; // index of the last pipe processed.

    int fdd = STDIN_FILENO; // copy of one end of pipe.

    for(i = 0; i < cmdline->ntokens; i++)
    {
        if(pipes != 0 && pipes == cmdline->npipes) // last executable:
        {
            executableIndex = lastPipeIndex + 1; // maybe can just = i ?
            spawn(cmdline, &fdd, pipes, executableIndex);
            
            i = cmdline->ntokens; // we're done; that was the last command to execute.
        }
        else if(!strcmp(cmdline->tokens[i], "|")) // more executables:
        {
            lastPipeIndex = i;

            spawn(cmdline, &fdd, pipes, executableIndex);
            
            // allow first stmt in pipe-chain to execute:
            pipes++;
            executableIndex = lastPipeIndex + 1;
        }
        else if (cmdline->npipes == 0) // no pipes:
        {
            spawn(cmdline, &fdd, pipes, executableIndex);
            i = cmdline->ntokens; // we're done; that was the last command to execute.
        }
        
    }
}

/**
 * Tokenizes the cmdline by newlines, tabs, and spaces.
 * @param cmdline the cmdline sent by the user.
*/
void tokenize(CmdLine* cmdline) 
{
    int count = 0; // number of tokens found.
    int pipeCount = 0; // number of pipes found.

    if((cmdline->tokens[0] = strtok(cmdline->line, "\n\t ")) == NULL)
    {
        cmdline->ntokens = 0; // no tokens found.
        return;
    }

    // tokenize by newlines, tabs, and spaces:
    while((cmdline->tokens[++count] = strtok(NULL, "\n\t ")) != NULL)
    {
        if(count >= MAX_TOKENS) // too many tokens.
        {
            fprintf(stderr, "internal-error: \n\t");
            fprintf(stderr, "too many tokens found. \n\t");
            fprintf(stderr, "maximum number of tokens is %d.\n", MAX_TOKENS);
            cmdline->ntokens = 0; // do not process any tokens.
        }

        if(!strcmp(cmdline->tokens[count], "|")) pipeCount++; // count pipes.
    } // the last one is always NULL.
    
    cmdline->ntokens = count; // store number of tokens found.
    cmdline->npipes = pipeCount; // store number of pipes found.

    if(DEBUG)
    {
        printf("\ntokens-detected: \n\t");
        for(count = 0; count < cmdline->ntokens; count++)
        {
            printf("[%s] \n\t", cmdline->tokens[count]);
        }
        printf("\n");
    }
}

void spawn(CmdLine* cmdline, int* fdd, int pipes, int executableIndex)
{
    int stdOutFd = STDOUT_FILENO; // the file descriptor to be used as stdout in child processes.
    
    int pid; // the process id of the current process.
    int fd[2]; // ->s to pipe ends.
    
    // attempt to create a pipe:
    if(pipe(fd) < 0)
    {   // if here, pipe failed:
        fprintf(stderr, "internal-error: \n\t");
        fprintf(stderr, "pipe-creation: failed. \n");

        free(cmdline);
        exit(-1);
    }

    int nArgsToChild = cmdline->ntokens + 1;
    char* argsToChild[nArgsToChild]; // ->s to arguments for child process.
    int j; // index of current token in child argument array. 

    // initialize array:
    for(j = 0; j < nArgsToChild; j++)
    {
        argsToChild[j] = NULL;
    }

    FILE *inputFile = NULL; // FILE information for an input file.
    FILE *outputFile = NULL; // FILE information for an output file.

    int stmtExecIndex; // index of current token in current statement.

    // copy over arguments:
    for(j = 0; ((stmtExecIndex = executableIndex + j)) < cmdline->ntokens; j++)
    {
        // redirecting input from file:
        if(!strcmp(cmdline->tokens[stmtExecIndex], "<"))
        {
            inputFile = fopen(cmdline->tokens[stmtExecIndex + 1], "r"); // gets the file pointer for the input file.
            if(inputFile == NULL) // if here, unable to open file:
            {
                fprintf(stderr, "invalid-file: \n\t");
                fprintf(stderr, "unable to open %s to use as STDIN for %s.\n", 
                    cmdline->tokens[stmtExecIndex + 1],
                    cmdline->tokens[executableIndex]);
                return;
            }

            if((*fdd = fileno(inputFile)) == -1) // if here, unable to get file descriptor: 
            {
                fprintf(stderr, "internal-error: \n\t");
                fprintf(stderr, "unable to get file descriptor for %s to use as STDIN for %s.\n",
                    cmdline->tokens[stmtExecIndex + 1],
                    cmdline->tokens[executableIndex]);
               
                free_strings(argsToChild, nArgsToChild);
                free(cmdline);
                exit(-1);
            }
            
            argsToChild[j] = NULL; // NULL terminate; everything else is irrelevant to child process.
        }
        //redirect output to file:
        else if(!strcmp(cmdline->tokens[stmtExecIndex], ">") || !strcmp(cmdline->tokens[stmtExecIndex], ">>"))
        {
            // gets the file pointer for the output file:
            outputFile = fopen(cmdline->tokens[stmtExecIndex + 1], (!strcmp(cmdline->tokens[stmtExecIndex], ">")) ? "w" : "a");

            if(outputFile == NULL) // if here, unable to open file:
            {
               fprintf(stderr, "invalid-file: \n\t");
                fprintf(stderr, "unable to open %s to use as STDOUT for %s.\n", 
                    cmdline->tokens[stmtExecIndex + 1],
                    cmdline->tokens[executableIndex]);
                return;
            }
                    
            if((stdOutFd = fileno(outputFile)) == -1) // if here, unable to get file descriptor: 
            {
                fprintf(stderr, "internal-error: \n\t");
                fprintf(stderr, "unable to get file descriptor for %s to use as STDOUT for %s.\n",
                    cmdline->tokens[stmtExecIndex + 1],
                    cmdline->tokens[executableIndex]);
               
                free_strings(argsToChild, nArgsToChild);
                free(cmdline);
                exit(-1);
            }

            argsToChild[j] = NULL; // NULL terminate; everything else is irrelevant to child process.
        }
        else if(!strcmp(cmdline->tokens[stmtExecIndex], "|") 
            || !strcmp(cmdline->tokens[stmtExecIndex], " "))
        {
            j = cmdline->ntokens; // stop copying; everything else is irrelevant to child process.
        }
        else // copy over to child arguments:
        {
            argsToChild[j] = malloc(strlen(cmdline->tokens[stmtExecIndex]) + 1);
            strcpy(argsToChild[j], cmdline->tokens[stmtExecIndex]);
        }
    }

    if(DEBUG)
    {
        printf("\nargs-to-child: \n\t");
        for(j = 0; j < nArgsToChild; j++)
        {
            printf("[%s] \n\t", argsToChild[j]);
        }
        printf("\n");
    }

    // attempt to fork a child process:
    if((pid = fork()) < 0)
    {   // if here, fork failed:
        fprintf(stderr, "internal-error: \n\t");
        fprintf(stderr, "fork: failed. \n");

        free(cmdline); // TODO: fix memory leak in argsToChild.
        exit(-1);
    }
    else if(pid == 0) // child process:
    {
        if(cmdline->npipes > 0 && pipes != cmdline->npipes) // && outputfile != null
        {
            stdOutFd = fd[1];
        }
        
        dup2(*fdd, STDIN_FILENO); // set the input.
        dup2(stdOutFd, STDOUT_FILENO); // set the output.

        close(fd[0]);
        execvp(cmdline->tokens[executableIndex], argsToChild); // replace image in child process.

        // if here, something has gone horribly wrong:
        fprintf(stderr, "invalid-executable:\n\tunable to execute %s.\n\tmake sure it is in your path.\n", cmdline->tokens[executableIndex]);
            
        free_strings(argsToChild, nArgsToChild);
        free(cmdline);
        exit(-1);
    }
    else // parent:
    {
        wait(NULL);		
        close(fd[1]);
        *fdd = fd[0]; // save output from previous stmt in pipe-chain.

        free_strings(argsToChild, nArgsToChild); // free strings passed to child process; we don't need them anymore.
    }
}

/**
 * Releases strings that were allocated with malloc.
 * @param ptrArray an array of ->s.
 * @param length length of the array.
*/
void free_strings(char* ptrArray[], int length)
{
    int i; // the current ptr index.
    for(i = 0; i < length; i++)  free(ptrArray[i]);
}