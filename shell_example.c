#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXARGS 128
#define MAXLINE 8192

extern char **environ; /* defined by libc */
/* function prototypes */
void eval(char *cmdline);
int phraseArgv(char *buf, char ***argvList);
void phraseIO(char *buf, char *cmdline, char *input, char *output);
void removeSingleSymbol(char *line, int lineSize, int removePos);
int builtin_command(char **argv);
void runArgv(char ***argvs, int argvSize, char *input, char *output);
void exeProc(char **argv,int fdIn, int fdOut,int pipesCount, int pfds[][2]);

int main()
{
    char cmdline[MAXLINE]; /* Command line */

    while (1)
    {
	    printf("myshell> "); /* Read */
	    fgets(cmdline, MAXLINE, stdin);
	    if (feof(stdin))
        {
	        exit(0);
        }
	    eval(cmdline); /* Evaluate */
    }
}

/* Evaluate a command line */
void eval(char *cmdline)
{
    char ***argv = (char***)malloc(MAXARGS*sizeof(char**));/* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    char input[MAXLINE];   /* Holds input path*/
    char output[MAXLINE];   /* Holds output path*/
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    phraseIO(buf, cmdline, input, output);
    
    /*
    printf("Input: %s\n", input);
    printf("Output: %s\n", output);
    printf("buf: %s\n",buf);
    */
    //strcpy(buf, cmdline);
    int argvSize = phraseArgv(buf, argv);
    /*
    int i=0;
    while(i<argvSize){
        printf("argv%d: %s\n",i,argv[i][0]);
        i++;
    }
    */
    if (argv[0] == NULL)
    {
	    return;   /* Ignore empty lines */
    }
    runArgv(argv, argvSize, input, output);
    /*
    if ((pid = fork()) == 0) 
    {
        if (execvp(argv[0], argv) < 0)
        {
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }
    }
    int status;
    if (waitpid(pid, &status, 0) < 0)
    {
        fprintf(stderr, "waitfg: waitpid error: %s\n",
                strerror(errno));
    }
    */
    return;
}

void phraseIO(char *buf, char *cmdline, char *input, char *output){

    *input = '\0';
    *output = '\0';
    cmdline = strtok(cmdline, "\n");
    char* inPtr,  *outPtr, *inputReader, *outputReader;
    //char inputReader[MAXLINE], outputReader[MAXLINE];
    inPtr = strrchr(cmdline, '<');
    if (inPtr){
        inputReader = strtok(cmdline, "<"); 
        strcpy(input,inputReader);
        cmdline = strtok(NULL, "<"); 
    }
    outPtr = strrchr(cmdline, '>');
    if (outPtr){
        outputReader = strtok(cmdline, ">"); 
        outputReader = strtok(NULL, ">"); 
        strcpy(output,outputReader);
    }
    strcpy(buf,cmdline);
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "exit")) /* exit command */
    {
        exit(0);
    }
    if (!strcmp(argv[0], "&"))
    {
        return 1;
    }
    return 0; /* Not a builtin command */
}

/* Parse the command line and build the argv array */
int phraseArgv(char *buf, char ***argv)
{
    char *cmd = strtok(buf, "\n"); 
    //int fileInput = -1;
    printf("RunCmd: \n");
    //printf("Input: %s\n", input);
    //printf("Output: %s\n", output);
    printf("buf: %s\n",buf);
    /*
    if (input[0]!='\0'){
        printf("Opening Filr %s\n", input);
        fileInput= open(input, O_RDWR);
    }
    
    printf("fileInput: %d\n",fileInput);
    */
    int argvSize = 0;
    cmd = strtok(buf, "|"); 
    while (cmd!=NULL){
        argv[argvSize] = (char**)malloc(MAXARGS*sizeof(char*));
        //printf("cmd: %s\n", cmd);
        int state = 0; //0: Null, 1: in normal word, 2: in double-quote
        char *readPointer=NULL, *startPointer=NULL;
        int charTmp;
        int count = 0;
        pid_t pid;           /* Process id */

        //char currentCmd[MAXLINE];
        //strcpy(currentCmd,cmd);
        for (readPointer = cmd; count < MAXARGS && *readPointer != '\0'; readPointer++) {
            //printf("readPointer: %s\n",readPointer);
            charTmp= (unsigned char) *readPointer;
            switch (state) {
            case 0:
                if (isspace(charTmp)) {
                    continue;
                }

                if (charTmp== '"') {
                    state = 2;
                    startPointer = readPointer + 1; 
                    continue;
                }
                else if (charTmp== '\'') {
                    state = 3;
                    startPointer = readPointer + 1; 
                    continue;
                }
                state = 1;
                startPointer = readPointer;
                continue;

            case 1:
                //printf("chartmp: %c\n",charTmp);
                if  (charTmp== '\"') {
                    //printf("\" is found\n");
                    removeSingleSymbol(startPointer,sizeof(startPointer),readPointer-startPointer);
                }
                if  (charTmp== '\'') {
                    //printf("\' is found\n");
                    removeSingleSymbol(startPointer,sizeof(startPointer),readPointer-startPointer);
                }
                if (isspace(charTmp)) {
                    *readPointer = 0;
                    argv[argvSize][count] = startPointer;
                    count++;
                    state = 0;
                }
                continue;
            
            
            case 2:
                if (charTmp== '"') {
                    *readPointer = 0;
                    argv[argvSize][count] = startPointer;
                    count++;
                    state = 0;
                }
                continue;
            case 3:
                if (charTmp== '\'') {
                    *readPointer = 0;
                    argv[argvSize][count] = startPointer;
                    count++;
                    state = 0;
                }
                continue;
            }
        }
        if (state != 0 && count < MAXARGS){
            //printf("startPointer: %s",startPointer);
            argv[argvSize][count] = startPointer;
            count++;
        }
        cmd = strtok(NULL, "|"); 
        argvSize++;
        
    }
    //execvp(argv[0], argv);
    return argvSize;
}
void runArgv(char ***argvs, int argvSize, char *input, char *output)
{
    int procCounter, pipeCounter;
    int outFd = STDOUT_FILENO;
    //int cmd_count = 0;
    //while (argvs[cmd_count]) { ++cmd_count; }
    if (input[0]!='\0'){
        printf("Input: %s\n",input);
    }
    if (output[0]!='\0'){
        printf("Output: %s\n",output);
        if (output[0]==' '){
            removeSingleSymbol(output, sizeof(output), 0);
        }
        outFd = open(output, O_RDWR|O_CREAT);
    }
     
    int pfds[MAXARGS][2];

    for (pipeCounter = 0; pipeCounter < argvSize-1; ++pipeCounter)
    {
        if (pipe(pfds[pipeCounter]) == -1)
        {
            fprintf(stderr, "Error: Unable to create pipe. (%d)\n", pipeCounter);
            exit(EXIT_FAILURE);
        }
    }

    for (procCounter = 0; procCounter < argvSize; ++procCounter)
    {
        int fd_in = STDIN_FILENO;
        //int fd_out = STDOUT_FILENO;
        int fd_out = outFd;
        if (procCounter != 0) 
            fd_in = pfds[procCounter - 1][0];
        if (procCounter != argvSize - 1) 
            fd_out = pfds[procCounter][1];

        exeProc(argvs[procCounter], fd_in, fd_out, argvSize-1, pfds);
    }

    for (procCounter = 0; procCounter < argvSize-1; ++procCounter)
    {
        close(pfds[procCounter][0]);
        close(pfds[procCounter][1]);
    }

    for (procCounter = 0; procCounter < argvSize; ++procCounter)
    {
        int status;
        wait(&status);
    }
}
void exeProc(char **argv, int fdIn, int fdOut, int pipesCount, int pfds[][2])
{
    pid_t proc = fork();

    if (proc < 0)
    {
        fprintf(stderr, "Error: Unable to fork.\n");
        exit(EXIT_FAILURE);
    }
    else if (proc == 0)
    {
        if (fdIn != STDIN_FILENO) { dup2(fdIn, STDIN_FILENO); }
        if (fdOut != STDOUT_FILENO) { dup2(fdOut, STDOUT_FILENO); }

        int procCount;
        for (procCount = 0; procCount < pipesCount; ++procCount)
        {
            close(pfds[procCount][0]);
            close(pfds[procCount][1]);
        }

        if (execvp(argv[0], argv) == -1)
        {
            fprintf(stderr,
                    "Error: Unable to load the executable %s.\n",
                    argv[0]);

            exit(EXIT_FAILURE);
        }

        /* NEVER REACH */
        exit(EXIT_FAILURE);
    }
}
void removeSingleSymbol(char *line, int lineSize, int removePos){
    int i = removePos;
    while (i<lineSize){
        line[i] = line[i+1];
        i++;
    }
    line[lineSize-1] = '\0';
    return;
}


