# MyShell - Custom Linux Shell

#Description
MyShell is a custom Linux shell created as part of a project for CIS-3207. It provides an interface for users to execute built-in commands, external commands, and manage processes. The shell supports various features such as input/output redirection, pipes, and background process execution.

This shell was developed using C, and implements system calls like fork(), execv(), pipe(), and wait(), among others.

# Features
Built-in Commands

help: Displays the list of supported commands and their usage.
exit: Exits the shell.
pwd: Prints the current working directory.
cd: Changes the current directory.
wait: Waits for all background processes to complete.
Basic Command Execution

Execute external commands using full or relative paths.
Supports execution of commands like ls, cat, echo, etc.
Custom my_grep function to search for patterns within files.
Input/Output Redirection

Redirect standard input (<) or standard output (>) to/from files.
Example:
echo "Hello" > output.txt
cat < input.txt
Background Process Execution

Run processes in the background using the & operator.
Example:
sleep 10 &
wait command to wait for all background processes to finish.
Pipe Operations

Supports piping of commands to pass the output of one command as input to another.
Example:
cat input.txt | grep "Hello" | wc -l


# Requirements
GCC (GNU Compiler Collection)
Linux environment

# Project Structure
Main.c: The main source code file containing all the logic for the shell.
debug.txt: debug example file.
README.md: This documentation file.
