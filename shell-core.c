/**
 * shell-core (sc)
 * An extendible Linux / Unix shell with pipe and redirection support.
 * 
 * @author Jordy Araujo <jordy.araujo@hotmail.com>
*/

#include <stdio.h>
#include <unistd.h>
#include<sys/wait.h> 

#include <limits.h>

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_TOKENS 25

typedef struct // holds information about the current command-line.
{
    int ntokens; // the number of tokens found in cmdline.
    char line[BUFSIZ]; // the full command-line.
    char* tokens[MAX_TOKENS]; // ->s to the tokens in cmdline.
} CmdLine;

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
            fprintf(stderr, "\n\tgetcwd: failed.");
            
            free(cmdline);
            exit(-1);
        }

        // get the last folder in the cwd:
        if((currentFolder = strrchr(cwd, '/')) == NULL)
        {   // if here, could not find / in directory:
            fprintf(stderr, "internal-error: ");
            fprintf(stderr, "\n\tstrrchr: failed to find current folder in %s.", cwd);
            
            free(cmdline);
            exit(-1);
        }

        uid_t euid = geteuid(); // the effective user id of this process.
        
        printf("[shell-core %s] %s ", ++currentFolder, ((euid == 0) ? "#" : "$"));
        if(fgets(cmdline->line, BUFSIZ, stdin) == NULL) 
        {
            fprintf(stderr, "internal-error: ");
            fprintf(stderr, "\n\tfgets: failed.");
            
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
    
    // check if user wants to leave the shell:
    if(!strcmp(cmdline->tokens[0], "quit") || !strcmp(cmdline->tokens[0], "exit")) 
    {
        free(cmdline);
        exit(0); // clean exit.
    }
}

/**
 * Tokenizes the cmdline by newlines, tabs, and spaces.
 * @param cmdline the cmdline sent by the user.
*/
void tokenize(CmdLine* cmdline) 
{
    int count = 0; // number of tokens found.

    if((cmdline->tokens[0] = strtok(cmdline->line, "\n\t ")) == NULL) 
        cmdline->ntokens = 0; // no tokens found.

    // tokenize by newlines, tabs, and spaces:
    while((cmdline->tokens[++count] = strtok(NULL, "\n\t ")) != NULL)
    {
        if(count >= MAX_TOKENS) // too many tokens.
        {
            fprintf(stderr, "internal-error: \n\t");
            fprintf(stderr, "too many tokens found. \n\t");
            fprintf(stderr, "maximum number of tokens is %d.", MAX_TOKENS);
            cmdline->ntokens = 0; // do not process any tokens.
        }
    } // the last one is always NULL.
    
    cmdline->ntokens = count; // update number of tokens found.
}