/*
    cShell - a simple UNIX shell in C supporting job control

    usage:

        chsell

        internal commands:
        cd - Change the current working directory

        envset - Sets or create the specified environment variable.
 
        envunset - Clears the specified environment variable.
 
        exit - Exits cShell.
 
        pause - Pauses operation of cSHell.
 
        print - Prints specified text.
 
        rbg - Attempts to move a job to the background and resume it.
 
        rfg - Attempts to move a job to the foreground and resume it.

*/
/*** END OF SECTION MARKER ***/

/************************************************************************************************************************

    ** Revision history **
 
    Current version: 2.0
    Date: 2 April 2016

    2.0: Finished final implementation.
    1.1: Fixed apostrophe issue, amended address, added section markers for function prototypes
    1.0: Original version
 
    Contributors:
    1. COMP3520 teaching staff
       Centre for Distributed and High Performance Computing
       School of Information Technologies
       The University of Sydney
       NSW 2006
       Australia
 
    2. Various people who contributed to various source code fragments at
       http://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html
 
    3. Various people who contributed to the source code at
       http://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html

    Copyright of this code and associated material is vested in the original contributors.

    This code is subject to the terms of the GNU General Public Licence Version 3.
    Licence terms are available at http://www.gnu.org/licenses/.
 
    ** DO NOT REMOVE THIS NOTICE. Just update the revision history whenever you make changes to the code. **
 
 ***********************************************************************************************************************/

/*** DO NOT CHANGE OR REMOVE ANY LINES ***/
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

/*** DO NOT CHANGE OR REMOVE ANY LINES ***/
#define DELIMITERS " \n\t"
#define MAX_ARGS 64
#define MAX_BUFFER_SIZE 1024
#define MAX_PATH 255

/* Custom data types */ /*** DO NOT CHANGE OR REMOVE ANY LINES ***/
typedef struct process /* Process control block */
    {
    struct process * next;      /* next process in pipeline */
    char ** argv;               /* for exec */
    pid_t pid;                  /* process ID */
    char completed;             /* true if process has completed */
    char stopped;               /* true if process has stopped */
    int status;                 /* reported status value */
    } process;

typedef struct job     /* Job control block */
    {
    struct job * next;          /* next active job */
    struct job * prev;          /* previous active job */
    char * command;             /* command line, used for messages */
    process * first_process;    /* list of processes in this job */
    pid_t pgid;                 /* process group ID */
    char notified;              /* true if user told about stopped job */
    struct termios tmodes;      /* saved terminal modes */
    int stdin, stdout, stderr;  /* standard i/o channels */
    } job;

/* Global variables */ /***DO NOT CHANGE OR REMOVE ANY LINES ***/
int fg_flag; /* Foreground execution flag */
    pid_t shell_pgid;
    struct termios shell_tmodes;
    int shell_terminal;
    int shell_is_interactive;
    job * job_list = NULL;

/*** START OF SECTION MARKER ***/
/***YOU MAY ADD LINES HERE BUT MAY NOT CHANGE OR REMOVE EXISTING LINES ***/

/* Function prototypes*/
    job * add_job(char *);
    process * add_process(job *, process *);
    int cmd_parser(char **, job *);
    void do_job_notification(void);
    job * find_job(pid_t);
    void format_job_info(job *, const char *);
    void free_job(job *);
    void init_shell(int);
    int job_is_stopped(job *);
    int job_is_completed(job *);
    void launch_job(job *, int);
    void launch_process(process *, pid_t, int, int, int, int);
    int mark_process_status(pid_t, int);
    void pause_func(void);
    void put_job_in_background(job *, int);
    void put_job_in_foreground(job *, int);
    void update_status(void);
    void wait_for_job(job *);

/*** END OF SECTION MARKER ***/

/*** NO CHANGES ARE PERMITTED BEYOND THIS POINT EXCEPT WHERE INDICATED ***/

/* Main function */
    int main (int argc, char ** argv)
    {
        char ** argp;
        char * cmd_args[MAX_ARGS];
        char buf[MAX_BUFFER_SIZE];
        char * prompt = " ==> ";

    /*** INSERT CALL TO init_shell() HERE ***/
        init_shell(argc);

    /*** INSERT YOUR CODE HERE for setting SHELL environment variable ***/
        realpath(argv[0],buf);
        setenv("SHELL",buf,1);


    /* keep reading commands until the "exit" command or EOF is triggered */

        while (1)
        {
        /* get command line from input */

        printf("%s%s", getenv("PWD"),prompt); /*** EDIT THIS LINE ***/


        if (fgets(buf, MAX_BUFFER_SIZE, stdin))                 // get next command line from user or batchfile
        {

            do_job_notification();                              // ensure zombie processes are reaped
            /* extract tokens from command line */

            argp = cmd_args;
            *argp++ = strtok(buf, DELIMITERS);              // fetch the first token
            while ((*argp = strtok(NULL, DELIMITERS)))      // keep fetching tokens until reaching the NULL token
            {
                argp++;
            }
                if (cmd_args[0])                                // run if and only if a command has been entered
                {
                /* check and respond to command */


                    //Check if the directory and valid and change to it.
                    if (!strcmp(cmd_args[0], "cd"))
                    {
                        if (cmd_args[1]) {
                            if (chdir(cmd_args[1]) < 0) {
                                fprintf(stderr,"Error: Can't change directory to: %s\n",cmd_args[1]);
                            } else {
                                getcwd(buf,MAX_BUFFER_SIZE);
                                setenv("PWD",buf,1);
                            }
                        }
                    }

                    //Checks that two arguments are supplied then sets the variable.
                    else if (!strcmp(cmd_args[0], "envset"))
                    {
                       if (cmd_args[1] == NULL || cmd_args[2] == NULL) {
                          puts("Error: expected two arguments");
                        } else {

                        setenv(cmd_args[1], cmd_args[2], 1);
                        }
                    }

                    //Unsets the variable if it exists.
                    else if (!strcmp(cmd_args[0], "envunset"))
                    {
                        char* var = getenv(cmd_args[1]);
                        if (var == NULL) {
                            puts("Variable does not exist.");
                        } else {
                        unsetenv(cmd_args[1]);
                        }
                    }
                    //Exits program.
                    else if (!strcmp(cmd_args[0], "exit"))
                    {
                        exit(EXIT_SUCCESS); 
                    }

                    //Pauses the program.
                    else if (!strcmp(cmd_args[0], "pause"))
                    {
                        pause_func();
                    }

                    //Prints the provided arguments.
                    else if (!strcmp(cmd_args[0], "print"))
                    {
                    int index = 1;
                    char line[MAX_BUFFER_SIZE] = "";

                    while (cmd_args[index] != NULL) {
                        strcat(line, cmd_args[index]);
                        strcat(line, " ");
                        index++;
                    }
                        puts(line);
                    }

                    //Attempts to place a job in the background.
                    else if (!strcmp(cmd_args[0], "rbg"))
                    {
                        pid_t PGID = (pid_t)cmd_args[1];
                        job * j = find_job(PGID);
                        if (j) {
                            if (j->first_process->stopped == 0) {
                                puts("Job already running.");
                            } else {
                            put_job_in_background(j,1);
                            }
                        } else {
                            puts("Unable to find job with matching PGID.");
                        }
                        
                    }

                    //Attempts to place job in foreground.
                    else if (!strcmp(cmd_args[0], "rfg"))
                    {
                        pid_t PGID = (pid_t)cmd_args[1];
                        job * j = find_job(PGID);
                        if (j) {
                            put_job_in_foreground(j,1);
                        } else {
                            puts("Unable to find job with matching PGID.");
                        }
                    }

                    //Run external command.
                    else
                    {
                        job * j = add_job(buf);
                        int foreground = cmd_parser(cmd_args,j);
                        if (foreground < 0) {
                            puts("Malformed command.  Check background symbols and pipes.");
                        } else {
                        launch_job(j,foreground);
                    }
                }
            }
        }

        else                            // user presses control-D or EOF has been reached
        {
            puts("");                   // ensure that there is a newline after shell exits
            break;                      // break out of "while" loop to quit
        }
    }
    exit(EXIT_SUCCESS);
}

/* Create a job and add it to the job list. Return the job */
job * add_job(char * cmd)
{
    job * j = job_list; /* Pointer to global job list */
    job * k = NULL;
    
    /* Initialise a new job control block */
    k = (job *)malloc(sizeof(job));
    k->next = NULL;
    k->command = strdup(cmd);
    k->first_process = NULL;
    k->pgid = 0;
    k->notified = 0;
    k->stdin = STDIN_FILENO;
    k->stdout = STDOUT_FILENO;
    k->stderr = STDERR_FILENO;
    tcgetattr(shell_terminal, &k->tmodes);

    //Initialise job list if it is empty.
    if (!job_list) {
        job_list = k;
        job_list->prev = NULL;
    } else {
        //Loop until we find the last item.
        while (!j->next) {
            j = j->next;
        }
        j->next = k;
        k->prev = j;
    }
    return k;
}

/* Add a pre-initialised process control block and add it to the specified job list. */
/* Return the process control block */
process * add_process(job * j, process * p)
{
    process * current_process = j->first_process;

    //Initialise list if it is empty.
    if (j->first_process == NULL) {
        j->first_process = p;
    } else {
        //Loop until we find the last process then append it.
        while (current_process->next != NULL) {
            current_process = current_process->next;
        }
        current_process->next = p;
    }

    return p;
}

/* Command line parser */
int cmd_parser(char ** cmd_args, job * j) /*** YOU MAY MODIFY THIS FUNCTION FOR EXERCISES 2 & 3 ***/
{
    char ** argp = cmd_args; // Working variable for command line tokens
    char ** argvp = NULL;    // Working variable for array of program arguments for process control block */

    //Loop through and check for malformed commands before we start adding processes.
    int index = 0;
    while(*argp) {
        //Check if pipe is at the end.
        if (!*(argp+1) && !strcmp(*argp,"|")) {
            return -1;
        }
        //Check if pipe is at the start.
        if (index == 0 && !strcmp(*argp,"|")) {
            return -1;
        }
        //Check if pipe is anywhere except the end.
        if (!strcmp(*argp,"&") && *(argp+1)) {
            return -1;
        }
        index++;
        argp++;
    }

    argp = cmd_args;
    fg_flag = 1;

    //Loop through each process.
    while (*argp) {
        //Initialise a new PCB>
        process * p = (process *)malloc(sizeof(process));
        p->next = NULL;
        p->completed = 0;
        p->stopped = 0;
        p->status = 0;
        p->argv = (char **)malloc(sizeof(char *)*MAX_ARGS);
        argvp = p->argv;

        int index = 0;
        while(*argp) {
            //Check if there is a & symbol at the end.
            if (!strcmp(*argp,"&") && *(argp+1) == NULL) {
                    fg_flag = 0;
                    argp++;
                    break;
            }
            //Check if there is a pipe symbol.
            if (!strcmp(*argp,"|")) {
                argp++;
                break;
            }
            //Otherwise, add the argument to the list.
            argvp[index] = (char*)malloc(sizeof(char*));
            strcpy(argvp[index],*argp);
            index++;
            argp++;
        }
        //Add the PCB to the job.
        argvp[index] = NULL;
        add_process(j,p);
    }


    return fg_flag;

}

/* Notify the user about stopped or terminated jobs.
 Delete terminated jobs from the active job list. */
void do_job_notification(void)
{
    job *j, *jlast, *jnext;
    
    /* Update status information for child processes. */
    update_status();
    

    jlast = NULL;
    for (j = job_list; j; j = jnext)
    {
        jnext = j->next;
        
        /* If all processes have completed, tell the user the job has
         completed and delete it from the list of active jobs. */
        if (job_is_completed(j))
        {
            format_job_info(j, "completed");
            if (jlast)
            {
                jlast->next = jnext;
            }
            else
            {
                job_list = jnext;
            }
            free_job(j);
        }
        
        /* Notify the user about stopped jobs,
         marking them so that we will not do this more than once. */
        else if (job_is_stopped(j) && !j->notified)
        {
            format_job_info(j, "stopped");
            j->notified = 1;
            jlast = j;
        }
        
        /* Do not say anything about jobs that are still running. */
        else
        {
            jlast = j;
        }
    }
}

/* Find the active job with the indicated pgid. */
job * find_job(pid_t pgid)
{
    job * j;
    
    for (j = job_list; j; j = j->next)
    {
        if (j->pgid == pgid)
        {
            return j;
        }
    }
    return NULL;
}

/* Format information about job status for the user to look at. */
void format_job_info(job *j, const char *status)
{
    fprintf(stderr, "%ld (%s): %s\n", (long)j->pgid, status, j->command);
}

void free_job(job * j)
{
    if (!j)
    {
        return;
    }
    if (j->next)
    {
        j->next->prev = j->prev;
    }
    if (j->prev)
    {
        j->prev->next = j->next;
    }
    else
    {
        job_list = j->next;
    }
    for (process * p = j->first_process; p;)
    {
        process * q = NULL;
        for (char ** argvp = p->argv; *argvp; argvp++)
        {
            free(*argvp);
        }
        free(p->argv);
        q = p;
        p = p->next;
        free(q);
    }
    if (j->command)
    {
     free(j->command);
 }
 else
 {
    fprintf(stderr, "WARNING: NULL pointer encountered for job name while trying to free job.\n");
}
free(j);
}

void init_shell(int argc)
{
    /* Set command input source */
    if (argc > 1)
    {
        fprintf(stderr, "cShell: Arguments are not supported.\n");
        exit(EXIT_FAILURE);
    }
    else if (argc != 1)
    {
        fprintf(stderr, "FATAL: Unable to process arguments\n");
        fprintf(stderr, "cShell: Unknown error\n");
        exit(EXIT_FAILURE);
    }
    
    /* See if we are running interactively. */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);
    
    if (shell_is_interactive)
    {
        /* Loop until we are in the foreground. */
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
        {
            kill(-shell_pgid, SIGTTIN);
        }
        
        /* Ignore interactive and job-control signals. */
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        //signal(SIGCHLD, SIG_IGN); // ignoring SIGCHLD would cause waitpid() to misbehave
        
        /* Put ourselves in our own process group. */
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0)
        {
            fprintf(stderr, "FATAL: Unable to put the shell in its own process group");
            perror("cShell");
            exit(EXIT_FAILURE);
        }
        
        /* Grab control of the terminal. */
        tcsetpgrp(shell_terminal, shell_pgid);
    }
    else
    {
        fprintf(stderr, "Sorry, this shell does not support non-interactive mode.\n");
        exit(EXIT_FAILURE);
    }
    
    /* Save default terminal attributes for shell. */
    tcgetattr(shell_terminal, &shell_tmodes);
}

/* Return true if all processes in the job have completed. */
int job_is_completed(job * j)
{
    process *p;
    
    for (p = j->first_process; p; p = p->next)
    {
        if (!p->completed)
        {
            return 0;
        }
    }
    return 1;
}

/* Return true if all processes in the job have stopped or completed. */
int job_is_stopped(job * j)
{
    process *p;
    
    for (p = j->first_process; p; p = p->next)
    {
        if (!p->completed && !p->stopped)
        {
            return 0;
        }
    }
    return 1;
}

void launch_job(job * j, int foreground)
{
    process *p;
    pid_t pid;
    int mypipe[2], infile, outfile;
    
    infile = j->stdin;
    for (p = j->first_process; p; p = p->next)
    {
        /* Set up pipes, if necessary.  */
        if (p->next)
        {
            if (pipe(mypipe) < 0)
            {
                fprintf(stderr, "FATAL: Unable to establish pipe\n");
                perror("External command");
                exit(EXIT_FAILURE);
            }
            outfile = mypipe[1];
        }
        else
            outfile = j->stdout;
        
        /* Fork the child processes.  */
        pid = fork();
        if (pid == 0)
        /* This is the child process.  */
            launch_process(p, j->pgid, infile,
             outfile, j->stderr, foreground);
        else if (pid < 0)
        {
            /* The fork failed.  */
            fprintf(stderr, "FATAL: Unable to fork process\n");
            perror("External command");
            exit(EXIT_FAILURE);
        }
        else
        {
            /* This is the parent process.  */
            p->pid = pid;
            if (shell_is_interactive)
            {
                if (!j->pgid)
                {
                    j->pgid = pid;
                }
                setpgid(pid, j->pgid);
            }
            else
            {
                j->pgid = getpid();
            }
        }
        
        /* Clean up after pipes.  */
        if (infile != j->stdin)
        {
            close(infile);
        }
        if (outfile != j->stdout)
        {
            close(outfile);
        }
        infile = mypipe[0];
    }
    
    format_job_info(j, "launched");
    
    if (!shell_is_interactive)
    {
        wait_for_job(j);
    }
    else if (foreground)
    {
        put_job_in_foreground(j, 0);
    }
    else
    {
        put_job_in_background(j, 0);
    }
}

void launch_process(process *p, pid_t pgid,
    int infile, int outfile, int errfile,
    int foreground)
{
    pid_t pid;
    
    if (shell_is_interactive)
    {
        /* Put the process into the process group and give the process group
         the terminal, if appropriate.
         This has to be done both by the shell and in the individual
         child processes because of potential race conditions.  */
         pid = getpid();
         if (pgid == 0)
         {
            pgid = pid;
        }
        setpgid(pid, pgid);
        if (foreground)
        {
            tcsetpgrp(shell_terminal, pgid);
        }
        
        /* Set the handling for job control signals back to the default.  */
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
    }
    
    /* Set the standard input/output channels of the new process. */
    if (infile != STDIN_FILENO)
    {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if (outfile != STDOUT_FILENO)
    {
        dup2(outfile, STDOUT_FILENO);
        close(outfile);
    }
    if (errfile != STDERR_FILENO)
    {
        dup2(errfile, STDERR_FILENO);
        close(errfile);
    }
    
    /* Exec the new process.  Make sure we exit. */
    execvp(p->argv[0], p->argv);
    fprintf(stderr, "ERROR: Unable to run external command\n");
    perror(p->argv[0]);
    exit(EXIT_FAILURE);
}

/* Store the status of the process pid that was returned by waitpid.
 Return 0 if all went well, nonzero otherwise. */
int mark_process_status(pid_t pid, int status)
{


    job *j;
    process *p;
    if (pid > 0)
    {
        /* Update the record for the process. */
        for (j = job_list; j; j = j->next)
            for (p = j->first_process; p; p = p->next)
                if (p->pid == pid)
                {
                    p->status = status;
                    if (WIFSTOPPED(status))
                        p->stopped = 1;
                    else
                    {
                        p->completed = 1;
                        if (WIFSIGNALED(status))
                            fprintf(stderr, "%d: Terminated by signal %d.\n",
                                (int) pid, WTERMSIG (p->status));
                    }
                    return 0;
                }

                fprintf(stderr, "No child process %d.\n", pid);
                return -1;
            }
            else if (pid == 0 || errno == ECHILD)
            {
        /* No processes ready to report. */
                return -1;
            }
            else
            {
        /* Other weird errors. */
                perror("waitpid");
                return -1;
            }
        }

/* Pause the SHELL until the user presses 'ENTER' */
/* Code adapted from the GNU website at http://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
with permission.*/
        void pause_func()
        {
            //Save the terminal settings.
            struct termios saved_attributes;
            struct termios tattr;
            tcgetattr (STDIN_FILENO, &saved_attributes);

            /* Set the funny terminal modes. */
            tcgetattr (STDIN_FILENO, &tattr);
            tattr.c_lflag &= ~(ICANON|ECHO); 
            tattr.c_cc[VMIN] = 1;
            tattr.c_cc[VTIME] = 0;
            tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);

            //Read until user presses ENTER.
            char c;
            puts("Please press ENTER to continue");
            while(1){
                read (STDIN_FILENO, &c, 1);
                if (c == '\n') {
                    break;
                } 
            }
            //Restore terminal settings.
            tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
        }

/* Put a job in the background.  If the cont argument is true, send
 the process group a SIGCONT signal to wake it up. */
        void put_job_in_background(job *j, int cont)
        {
    j->notified = 0; // Enable job suspension notifications
    for (process * p = j->first_process; p; p = p->next)
    {
        p->completed = 0;
        p->stopped = 0;
        p->status = 0;
    }
    /* Send the job a continue signal, if necessary. */
    if (cont)
    {
        if (kill(-j->pgid, SIGCONT) < 0)
        {
            perror("kill (SIGCONT)");
        }
    }
}

/* Put job j in the foreground.  If cont is nonzero,
 restore the saved terminal modes and send the process group a
 SIGCONT signal to wake it up before we block. */
 void put_job_in_foreground(job * j, int cont)
 {
    j->notified = 0; // Enable job suspension notifications
    for (process * p = j->first_process; p; p = p->next)
    {
        p->completed = 0;
        p->stopped = 0;
        p->status = 0;
    }
    /* Put the job into the foreground. */
    tcsetpgrp(shell_terminal, j->pgid);
    /* Send the job a continue signal, if necessary. */
    if (cont)
    {
        tcsetattr(shell_terminal, TCSADRAIN, &j->tmodes);
        if (kill(-j->pgid, SIGCONT) < 0)
        {
            perror("kill (SIGCONT)");
        }
    }
    /* Wait for it to report. */
    wait_for_job(j);
    
    /* Put the shell back in the foreground. */
    tcsetpgrp(shell_terminal, shell_pgid);
    
    /* Restore the shell's terminal modes. */
    tcgetattr(shell_terminal, &j->tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* Check for processes that have status information available,
 without blocking.  */
void update_status(void)
{
    int status;
    pid_t pid;
    
    do
    {
        pid = waitpid(-1, &status, WUNTRACED|WNOHANG); //Had to change WAIT_ANY to -1 as it wasn't compiling on UCPU2.
    } while (!mark_process_status(pid, status));
}

/* Check for processes that have status information available,
 blocking until all processes in the given job have reported.  */
void wait_for_job(job * j)
{
    int status;
    pid_t pid;
    
    do
    {
        pid = waitpid(-j->pgid, &status, WUNTRACED);
    } while (!mark_process_status(pid, status)
     && !job_is_stopped(j)
     && !job_is_completed(j));
}

/*** IMPLEMENTATIONS OF ANY ADDITIONAL FUNCTIONS BELONG BELOW THIS LINE ***/
/*** Note: You might not need to use this section. ***/


/*** END OF ADDITIONAL FUNCTIONS ***/
/*** END OF CODE; DO NOT ADD MATERIAL BEYOND THIS POINT ***/
