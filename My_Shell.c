/*
 * Production-Level Mini Shell
 * Features:
 * - Builtins: cd, exit, history, export
 * - Pipes
 * - Redirection (<, >, >>)
 * - Background execution (&)
 * - Variable expansion
 * - Script execution
 * - Signal handling (Ctrl+C)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 128
#define MAX_HISTORY 1000

char *history[MAX_HISTORY];
int history_count = 0;

/*-------------------- Utility --------------------*/
void add_history(char *line) {
    if (history_count < MAX_HISTORY)
        history[history_count++] = strdup(line);
}

void show_history() {
    for (int i = 0; i < history_count; i++)
        printf("%d %s\n", i + 1, history[i]);
}

void trim_newline(char *line) {
    line[strcspn(line, "\n")] = 0;
}

/*----------------- Signal Handling ----------------*/
void sigint_handler(int sig) {
    write(STDOUT_FILENO, "\nmyshell> ", 10);
}

void init_signals() {
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, SIG_IGN);
}

/*----------------- Variable Expansion ----------------*/
void expand_variables(char *line) {
    char buffer[MAX_INPUT] = {0};
    char var[128];
    int i = 0;

    while (line[i]) {
        if (line[i] == '$') {
            i++;
            int k = 0;
            while (isalnum(line[i]) || line[i]=='_')
                var[k++] = line[i++];
            var[k] = '\0';
            char *val = getenv(var);
            if (val)
                strcat(buffer, val);
        } else {
            strncat(buffer, &line[i], 1);
            i++;
        }
    }
    strcpy(line, buffer);
}

/*----------------- Built-in Commands ----------------*/
int handle_builtin(char **args) {
    if (!args[0]) return 1;

    if (strcmp(args[0], "exit") == 0)
        exit(0);

    if (strcmp(args[0], "cd") == 0) {
        if (!args[1])
            chdir(getenv("HOME"));
        else if (chdir(args[1]) != 0)
            perror("cd");
        return 1;
    }

    if (strcmp(args[0], "history") == 0) {
        show_history();
        return 1;
    }

    if (strcmp(args[0], "export") == 0) {
        if (args[1]) {
            char *key = strtok(args[1], "=");
            char *val = strtok(NULL, "=");
            if (key && val)
                setenv(key, val, 1);
        }
        return 1;
    }

    return 0;
}

/*----------------- Parse Arguments ----------------*/
void parse_args(char *line, char **args) {
    int i = 0;
    args[i] = strtok(line, " ");
    while (args[i] != NULL && i < MAX_ARGS-1)
        args[++i] = strtok(NULL, " ");
}

/*----------------- Redirection ----------------*/
void handle_redirection(char **args) {
    for (int i = 0; args[i]; i++) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            int fd = open(args[i+1], O_RDONLY);
            if (fd < 0) perror("open");
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
        }
    }
}

/*----------------- Pipeline Execution ----------------*/
void execute_pipeline(char *line) {
    char *commands[10];
    int num_cmds = 0;
    commands[num_cmds++] = strtok(line, "|");
    while ((commands[num_cmds++] = strtok(NULL, "|")) != NULL);

    int in_fd = 0;

    for (int i=0; i<num_cmds-1; i++) {
        int pipefd[2];
        pipe(pipefd);

        if (fork() == 0) {
            dup2(in_fd, 0);
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            char *args[MAX_ARGS];
            parse_args(commands[i], args);
            handle_redirection(args);
            execvp(args[0], args);
            perror("exec");
            exit(1);
        }

        wait(NULL);
        close(pipefd[1]);
        in_fd = pipefd[0];
    }

    // Last command
    char *args[MAX_ARGS];
    parse_args(commands[num_cmds-2], args);
    if (fork() == 0) {
        dup2(in_fd, 0);
        handle_redirection(args);
        execvp(args[0], args);
        perror("exec");
        exit(1);
    }
    wait(NULL);
}

/*----------------- Main Shell Loop ----------------*/
void shell_loop(FILE *input) {
    char line[MAX_INPUT];

    while (1) {
        if (input == stdin) printf("myshell> ");
        fflush(stdout);

        if (!fgets(line, MAX_INPUT, input)) break;
        trim_newline(line);
        if (strlen(line) == 0) continue;

        add_history(line);
        expand_variables(line);

        // Pipeline
        if (strchr(line, '|')) {
            execute_pipeline(line);
            continue;
        }

        char *args[MAX_ARGS];
        parse_args(line, args);

        // Background execution
        int background = 0;
        int i=0;
        while (args[i]) i++;
        if (i>0 && strcmp(args[i-1], "&")==0) {
            background = 1;
            args[i-1] = NULL;
        }

        if (!handle_builtin(args)) {
            pid_t pid = fork();
            if (pid == 0) {
                handle_redirection(args);
                execvp(args[0], args);
                perror("exec");
                exit(1);
            } else if (!background) {
                waitpid(pid, NULL, 0);
            }
        }
    }
}

/*----------------- Main ----------------*/
int main(int argc, char *argv[]) {
    init_signals();

    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) { perror("script"); return 1; }
        shell_loop(file);
        fclose(file);
    } else {
        shell_loop(stdin);
    }

    return 0;
}
