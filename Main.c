#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Function Declarations
void handle_builtin_command(char *command, char **arguments);
void handle_external_command(char *command, char **arguments);
int is_builtin_command(char *command);
char **split(char *input, const char *delimiter);
void trim(char *str);
void display_help();
char *find_command_in_path(char *command);
void handle_redirection(char **arguments);
void execute_pipe_commands(char **commands);
void my_grep(char *pattern, char *filename);

// Main shell loop
int main() {
    char input[1024];
    char current_directory[PATH_MAX];

    printf("Welcome to MyShell!\n");

    while (1) {
        getcwd(current_directory, sizeof(current_directory));
        printf("%s$ ", current_directory);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // Handle Ctrl+D (EOF)
        }

        trim(input);

        if (strlen(input) == 0) {
            continue;
        }

        // Split input by semicolon into multiple commands
        char **commands = split(input, ";");

        for (int i = 0; commands[i] != NULL; i++) {
            trim(commands[i]);
            
            // Check if the command contains a pipe
            if (strchr(commands[i], '|') != NULL) {
                // Split the command by pipe
                char **pipe_commands = split(commands[i], "|");
                // Trim each pipe command
                for (int j = 0; pipe_commands[j] != NULL; j++) {
                    trim(pipe_commands[j]);
                }
                execute_pipe_commands(pipe_commands);
                free(pipe_commands);
            } else {
                char **arguments = split(commands[i], " \n");
                char *command = arguments[0];
                
                if (command == NULL) {
                    free(arguments);
                    continue;
                }

                if (is_builtin_command(command)) {
                    handle_builtin_command(command, arguments);
                } else if (strcmp(command, "my_grep") == 0) {
                    if (arguments[1] != NULL && arguments[2] != NULL) {
                        my_grep(arguments[1], arguments[2]);
                    } else {
                        printf("Usage: my_grep <pattern> <filename>\n");
                    }
                } else {
                    handle_external_command(command, arguments);
                }
                free(arguments);
            }
        }
        free(commands);
    }
    return 0;
}

// Check if command is a built-in command
int is_builtin_command(char *command) {
    const char *builtin_commands[] = {"help", "exit", "pwd", "cd", "wait"};
    for (int i = 0; i < 5; i++) {
        if (strcmp(command, builtin_commands[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Custom grep function to filter lines in a file
void my_grep(char *pattern, char *filename) {
    // Remove quotes from the pattern if they exist
    if (pattern[0] == '"' && pattern[strlen(pattern) - 1] == '"') {
        pattern[strlen(pattern) - 1] = '\0';
        pattern++;
    }

    printf("Searching for pattern: %s in file: %s\n", pattern, filename);

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char line[1024];
    int found = 0;
    int line_number = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;
        char *match = strstr(line, pattern);
        if (match != NULL) {
            found = 1;
            printf("Line %d: ", line_number);
            for (char *ptr = line; *ptr != '\0'; ptr++) {
                if (ptr == match) {
                    printf("\033[31m"); // Set text color to red
                }
                putchar(*ptr);
                if (ptr == match + strlen(pattern) - 1) {
                    printf("\033[0m"); // Reset color
                }
            }
        }
    }

    if (!found) {
        printf("No matches found for pattern: %s\n", pattern);
    }

    fclose(file);
}

// Handle built-in commands
void handle_builtin_command(char *command, char **arguments) {
    if (strcmp(command, "help") == 0) {
        display_help();
    } else if (strcmp(command, "exit") == 0) {
        printf("Exiting...\n");
        exit(0);
    } else if (strcmp(command, "pwd") == 0) {
        char current_directory[PATH_MAX];
        getcwd(current_directory, sizeof(current_directory));
        printf("Current directory: %s\n", current_directory);
    } else if (strcmp(command, "cd") == 0) {
        if (arguments[1] != NULL) {
            if (chdir(arguments[1]) != 0) {
                perror("Error: Directory does not exist");
            }
        } else {
            printf("Error: No directory provided\n");
        }
    } else if (strcmp(command, "wait") == 0) {
        while (wait(NULL) > 0);
    }
}

// Find command in PATH
char *find_command_in_path(char *command) {
    if (command[0] == '/') {
        struct stat buffer;
        if (stat(command, &buffer) == 0) {
            return strdup(command);
        }
    }

    char *path = getenv("PATH");
    if (path == NULL) {
        return NULL;
    }

    char *path_copy = strdup(path);
    char *token = strtok(path_copy, ":");

    while (token != NULL) {
        char full_path[PATH_MAX];
        if (token[strlen(token) - 1] == '/') {
            snprintf(full_path, sizeof(full_path), "%s%s", token, command);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", token, command);
        }

        struct stat buffer;
        if (stat(full_path, &buffer) == 0) {
            free(path_copy);
            return strdup(full_path);
        }

        token = strtok(NULL, ":");
    }

    free(path_copy);
    return NULL;
}

// Handle external commands
void handle_external_command(char *command, char **arguments) {
    int background = 0;
    int i;

    for (i = 0; arguments[i] != NULL; i++) {
        if (arguments[i + 1] == NULL && strcmp(arguments[i], "&") == 0) {
            background = 1;
            arguments[i] = NULL;
            break;
        }
    }

    char *full_path = find_command_in_path(command);
    if (full_path != NULL) {
        pid_t pid = fork();
        if (pid == 0) {
            handle_redirection(arguments);
            execv(full_path, arguments);
            perror("Error: Command execution failed");
            exit(1);
        } else if (pid > 0) {
            if (!background) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                printf("[%d] Running in background\n", pid);
            }
        } else {
            perror("Error: Fork failed");
        }
        free(full_path);
    } else {
        fprintf(stderr, "Error: Command not found: %s\n", command);
    }
}

// Handle redirection
void handle_redirection(char **arguments) {
    for (int i = 0; arguments[i] != NULL; i++) {
        if (strcmp(arguments[i], "<") == 0) {
            int fd = open(arguments[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("Error opening file for input redirection");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            arguments[i] = NULL;
        } else if (strcmp(arguments[i], ">") == 0) {
            int fd = open(arguments[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                perror("Error opening file for output redirection");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            arguments[i] = NULL;
        }
    }
}

// Execute pipe commands
void execute_pipe_commands(char **commands) {
    int num_commands = 0;
    while (commands[num_commands] != NULL) {
        num_commands++;
    }

    if (num_commands < 2) {
        fprintf(stderr, "Error: Pipe requires at least two commands\n");
        return;
    }

    int pipefds[2 * (num_commands - 1)];

    // Create all pipes
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe");
            return;
        }
    }

    // Execute each command
    for (int i = 0; i < num_commands; i++) {
        char **args = split(commands[i], " \n");
        if (args[0] == NULL) {
            fprintf(stderr, "Error: Empty command in pipe\n");
            free(args);
            continue;
        }

        char *cmd_path = find_command_in_path(args[0]);
        if (cmd_path == NULL) {
            fprintf(stderr, "Command not found: %s\n", args[0]);
            free(args);
            continue;
        }
        
        pid_t pid = fork();
        if (pid == 0) {  // Child process
            // Set up pipe input for all commands except first
            if (i > 0) {
                if (dup2(pipefds[(i-1)*2], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }
            
            // Set up pipe output for all commands except last
            if (i < num_commands - 1) {
                if (dup2(pipefds[i*2+1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
            }

            // Close all pipe fds in child
            for (int j = 0; j < 2*(num_commands-1); j++) {
                close(pipefds[j]);
            }

            // Execute command
            execv(cmd_path, args);
            perror("execv");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
        }
        
        free(cmd_path);
        free(args);
    }

    // Parent closes all pipe fds
    for (int i = 0; i < 2*(num_commands-1); i++) {
        close(pipefds[i]);
    }

    // Wait for all children
    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }
}

// Split input into command and arguments
char **split(char *input, const char *delimiter) {
    char **tokens = malloc(64 * sizeof(char*));
    char *token;
    int position = 0;

    token = strtok(input, delimiter);
    while (token != NULL) {
        if (token[0] == '"' && token[strlen(token) - 1] == '"') {
            token[strlen(token) - 1] = '\0';
            token++;
        }
        tokens[position++] = token;
        token = strtok(NULL, delimiter);
    }
    tokens[position] = NULL;

    return tokens;
}

// Trim leading and trailing whitespace
void trim(char *str) {
    char *end;

    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) {
        return;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    *(end + 1) = 0;
}

// Display help information
void display_help() {
    printf("Available commands:\n");
    printf("  help - Display this help information\n");
    printf("  exit - Exit the shell\n");
    printf("  pwd  - Print the current working directory\n");
    printf("  cd   - Change the current directory\n");
    printf("  wait - Wait for all background processes to complete\n");
    printf("  my_grep <pattern> <filename> - Search for pattern in file\n");
    printf("Supports pipes (|) and redirections (>, <)\n");
}
