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
int phraseArgv(char **argvStringList, char ***argvList,int argvSize, char *input[], char *output[]);
int phrasePipe(char *buf, char **argvStringList);
void phraseIO(char *buf, char *cmdline, char *input, char *output);
void removeSingleSymbol(char *line, int lineSize, int removePos);
int builtin_command(char **argv);
void runArgv(char ***argvs, int argvSize, char *input[], char *output[]);
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
    char ** argvStringList =  (char**)malloc(MAXARGS*sizeof(char*));
    char buf[MAXLINE];   /* Holds modified command line */
    char *input[MAXARGS];   /* Holds input path*/
    char *output[MAXARGS];   /* Holds output path*/
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */

    strcpy(buf,cmdline);
    
    
    //strcpy(buf, cmdline);
    int argvSize = phrasePipe(buf, argvStringList);
    phraseArgv(argvStringList, argv, argvSize, input, output);
    /*
    int i=0;
    while(i<argvSize){
        printf("argvString %d: %s\n",i,argvStringList[i]);
        printf("argv %d: %s\n",i,argv[i][0]);
        printf("input %d: %s\n",i,input[i]);
        printf("output %d: %s\n",i,output[i]);
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
int phrasePipe(char *buf, char **argvStringList){
    char *cmd = strtok(buf, "\n"); 
    int argvSize = 0;
    cmd = strtok(buf, "|"); 
    while (cmd!=NULL){
        argvStringList[argvSize] = (char*)malloc(MAXLINE*sizeof(char));
        sprintf(argvStringList[argvSize], cmd);
        cmd = strtok(NULL, "|"); 
        argvSize++;
    }
    return argvSize;
}
void phraseIO(char *buf, char *cmdline, char *input, char *output){
    char* inPtr,  *outPtr, *outputReader;
    char *tmpBuf = (char*)malloc(MAXARGS*sizeof(char*));
    int charTmp, isInitstart = 0;
    strcpy(tmpBuf, cmdline);
    *input = '\0';
    *output = '\0';
    //cmdline = strtok(cmdline, "\n");
    /*
    char *readPointer=NULL, *startPointer=NULL;
    int state = 0; //0: cmd, 1: input, 2: output
    for (readPointer = tmpBuf; *readPointer != '\0'; readPointer++) {
        charTmp= (unsigned char) *readPointer;
        if (!isInitstart) {
            isInitstart = 1;
            startPointer = readPointer;
        }
        switch (state) {
            case 0:
                if  (charTmp == '<' || charTmp == '>') {
                    *readPointer = 0;
                    printf("startPointer1: %s\n", startPointer);
                    buf = startPointer;
                    if (charTmp == '<'){
                        state = 1;
                    }
                    if (charTmp == '>'){
                        state = 2;
                    }
                    isInitstart = 0;
                    printf("startPointer2: %s\n", startPointer);
                }
                continue;
            case 1:
                if  (charTmp == '>') {
                    *readPointer = 0;
                    printf("startPointer3: %s\n", startPointer);
                    strcpy(input, startPointer);
                    state = 2;
                    isInitstart = 0;
                    printf("startPointer4: %s\n", startPointer);
                }
                continue;

        }
    }
    switch(state){
        case 0:
            printf("startPointer case0: %s\n", startPointer);
            strcpy(buf, startPointer);
            buf = strtok(buf, "\n");
            break;
        case 1:
                    printf("startPointer case1: %s\n", startPointer);
            strcpy(input, startPointer);
            input = strtok(input, "\n");
            break;
        case 2:
                    printf("startPointer case2: %s\n", startPointer);
            strcpy(output, startPointer);
            output = strtok(output, "\n");
            break;

    }
    */
    //printf("==phraseIO Start\n");
    //printf("Input: %s\n",tmpBuf);
    inPtr = strrchr(tmpBuf, '<');
    outPtr = strrchr(tmpBuf, '>');
    if (inPtr){
        tmpBuf = strtok(tmpBuf, "<"); 
        strcpy(buf, tmpBuf);
        //printf("inPtr buf: %s\n", buf);
        tmpBuf = strtok(NULL, "<"); 
        //printf("inPtr tmpBuf: %s\n", tmpBuf);
        if (outPtr)
            tmpBuf = strtok(tmpBuf, ">"); 
        strcpy(input,tmpBuf);
    }
    if (outPtr){
        if (!inPtr)
            outputReader = strtok(tmpBuf, ">"); 
            outputReader = strtok(NULL, ">"); 
            //printf("outputReader: %s\n", outputReader);
            strcpy(output,outputReader);
    }
    if (!inPtr){
        strcpy(buf,tmpBuf);
    }
    //printf("==phraseIO End\n");
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
int phraseArgv(char **argvStringList, char ***argvList, int argvSize, char *input[], char *output[])
{
    //printf("==phraseArgv start\n");//DEBUG
    //printf("argvSize: %d\n", argvSize);
    char *cmd = NULL; 
    int argvCounter = 0;
    for (; argvCounter<argvSize; argvCounter++){
        //printf("cmd: %s\n", argvStringList[argvCounter]);
        int state = 0; //0: Null, 1: in normal word, 2: in double-quote
        char *readPointer=NULL, *startPointer=NULL;
        int charTmp;
        int count = 0;
        argvList[argvCounter] = (char**)malloc(MAXARGS*sizeof(char*));
        char *cmdBuff;
        cmdBuff = (char*)malloc(MAXLINE*sizeof(char));
        input[argvCounter] = (char*)malloc(MAXLINE*sizeof(char));
        output[argvCounter] = (char*)malloc(MAXLINE*sizeof(char));
        phraseIO(cmdBuff, argvStringList[argvCounter], input[argvCounter], output[argvCounter]);
        //printf("cmdBuff: %s\n", cmdBuff);
        //printf("input[%d]: %s\n", argvCounter, input[argvCounter]);
        //printf("output[%d]: %s\n", argvCounter, output[argvCounter]);
        cmdBuff = strtok(cmdBuff, " \t");
        
        while (cmdBuff!=NULL && count<MAXARGS ){
            //printf("cmdBuff in argv: %s\n", cmdBuff);
            argvList[argvCounter][count] = (char*)malloc(MAXLINE*sizeof(char));
            strcpy(argvList[argvCounter][count], cmdBuff);
            //printf("argvList[%d][%d]: %s\n", argvCounter, count, argvList[argvCounter][count]);
            count++;
            cmdBuff = strtok(NULL, " \t"); 
        }
        
    }
    
    //execvp(argv[0], argv);
    return argvSize;
}
void runArgv(char ***argvs, int argvSize, char *input[], char *output[])
{
    int procCounter, pipeCounter;
    //int cmd_count = 0;
    //while (argvs[cmd_count]) { ++cmd_count; }
    /*
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
     */
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
        if (input[procCounter][0]!='\0'){
            input[procCounter] = strtok(input[procCounter], " \t");
            fd_in = open(input[procCounter], O_RDONLY);
        }
        int fd_out = STDOUT_FILENO;
        if (output[procCounter][0]!='\0'){
            output[procCounter] = strtok(output[procCounter], " \t");
            fd_out = open(output[procCounter], O_RDWR|O_CREAT);
        }
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
    //line[lineSize-1] = '\0';
    return;
}


