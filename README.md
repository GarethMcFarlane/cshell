cShell

Description:
	cShell is a simple program written in the C programming language that is primarily used as an interactive shell to schedule and run various commands.  Any external commands not explicitly stated in this tutorial will be outsourced to the external shell and executed.

Pipelining:
	cShell allows the user to "pipe" two or more processes together.  When two or more processes are piped, the output of the first process is used as the input of the second process, and so on.  The symbol used to specify a pipe is "|" and can be placed between any two commands.

Foreground and background processes:
	cShell allows jobs to be run in the background or foreground.  By default, jobs are run in the foreground and the shell must wait for the job to finish before allowing the user to enter further commands.  If and only if the user appends the "&" symbol to their command, it will be marked for execution in the background.  When background jobs are executed, the user is immediately able to input further commands while the job processes in the background.

Job notification:
	Whenever a command is entered, the user will receive notifications about recently launched, completed and suspended jobs.

Compilation and execution:
	To compile the program, first navigate to the folder where myshell.c is located.  To compile the program, you must ensure you have a C compiler installed on your system.  The below instructions are for GCC but will be similar for other compilers.  Once you are in the folder, type the following:
	gcc -w -std=c99 -o cshell cshell.c

	A new file will be created called 'cshell'.  This is the program executable and can be launched by typing:
	./cshell

Command syntax:
	<command> [arg1 arg2 ... argN] [|] [arg1 arg2 ... argN] [&]

	< > - angle brackets represent required commands
	[ ] - square brackets represent optional commands
	|	- vertical line represents the pipe symbol
	&	- ampersand represents the background symbol

Commands:

The '==>' symbol means the shell is ready to accept user commands.  These commands are listed below:

	-	Change Directory - cd [directory]
	Allows you to change the current working directory.  This command allow relative and absolute pathnames.  For example "cd .." will move to the parent directory.  "cd /users/user" will navigate to your home directory.
	-	Set Variable - envset [var_name] [value]
	Allows you to create or modify existing environment variables.  For example, to modify the LANG variable, type "envset LANG en-UK.UTF8"
	-	Unset Variable - envunset [var_name]
	Allows you to unset an existing environment variable.  If the variable doesn't exist you will see an error.  To unset the LANG variable, type "envunset LANG".
	-	exit
	Exits the cShell program.
	-	pause
	Pauses execution of the shell.  When the program is paused the user will be unable to input any commands until they press the 'Enter' key.  Background jobs will not be affected by this.
	-	print [arg1]...[argN]
	Prints all arguments provided to the command.  If no arguments are provided, a blank line is printed. 
	-	Resume Background - rbg [PGID]
	Attempts to place a job in the background and resume it if the PGID is known.  If the job is already running in the background or if a job doesn't exist with the specified PGID, an error will be returned.
	-	Resume Foreground - rfg [PGID]
	Attempts to place a job with the specified PGID in the foreground and resume it if it is suspended.  If no matching job is found, an error is returned.  

External commands:
	All external commands supported by your native shell can be executed by the program.  These will be launched as jobs and their status will be displayed whenever a command is entered.
