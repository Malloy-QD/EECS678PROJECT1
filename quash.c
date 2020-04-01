#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_COMMAND_COUNT 40
#define MAX_STRING_SIZE 256
#define MAX_PARAMETERS 100
#define MAX_BACKGROUND_JOBS 2048

#define PROMPT_BEGIN \
    printf("\n");    \
    fflush(stdout);
#define PROMPT_END \
    printf("> ");  \
    fflush(stdout);

/*
    useful structure
*/
typedef struct CommandInner
{
    char parameters[MAX_PARAMETERS][MAX_STRING_SIZE];
    char input_file[MAX_STRING_SIZE];
    char output_file[MAX_STRING_SIZE];
    int parameters_count;
} Command;
typedef struct BackgroundJobInner
{
    int id;
    pid_t p_id;
    char original_str[MAX_STRING_SIZE];
} BackgroundJob;

/*
    global variables
*/
extern char **environ;
int stop_now = 0;
int unique_id = 1;
BackgroundJob background_jobs[MAX_BACKGROUND_JOBS];
int current_background_jobs_size = 0;

/*
    parse a line into the commands array, size and background will also be updated.
*/
void parseCommands(char *line, Command *commands, int *size, int *background);
/*
    initiate the command with the string
*/
void initCommand(char *str, Command *command);
/*
    catch the children's finishing signal
*/
void sigchildHandler(int signo);
/*
    run the already parsed commands
*/
void run_command(Command *commands, char *original_str, int size, int background);

int main(int argc, char **argv, char **envp)
{

    signal(SIGCHLD, sigchildHandler);

    FILE *input = stdin;

    //main loop
    do
    {
        if (stop_now)
            break;
        char buffer[MAX_STRING_SIZE] = {0};
        //get the line
        PROMPT_END;
        if (!fgets(buffer, MAX_STRING_SIZE, input))
        {
            break;
        }
        int len = strlen(buffer);
        if (len >= 1 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = 0;
        }
        char copy_buffer[MAX_STRING_SIZE] = {0};
        memcpy(copy_buffer, buffer, MAX_STRING_SIZE);
        len = strlen(copy_buffer);
        if (len >= 1 && copy_buffer[len - 1] == '&')
        {
            copy_buffer[len - 1] = 0;
        }
        {
            Command commands[MAX_COMMAND_COUNT] = {0};
            int size = 0;
            int background = 0;
            //parse the command
            parseCommands(buffer, commands, &size, &background);
            if (size == 0)
            {
                continue;
            }
            else if (size > 2)
            {
                printf("only 1 pipe are supported!\n");
                continue;
            }
            //execute the command
            run_command(commands, copy_buffer, size, background);
        }
    } while (1);
    while (current_background_jobs_size >= 1)
        ;
}

static char *process_str1(char *line)
{
    while (*line && (isspace(*line) || (*line == '|')))
    {
        line++;
    }
    int len = strlen(line);
    while (len > 0 && (isspace(line[len - 1]) || (line[len - 1] == '|')))
    {
        line[len - 1] = 0;
        len--;
    }
    return line;
}

void parseCommands(char *line, Command *commands, int *size, int *background)
{
    *size = 0;
    *background = 0;
    line = process_str1(line);
    if (!(line))
        return;

    int len = strlen(line);
    if (line[len - 1] == '&')
    {
        *background = 1;
        line[len - 1] = 0;
    }

    while (1)
    {
        char *current = strstr(line, "|");
        if (!current)
            break;
        char *old_line = line;
        *current = 0;
        line = current + 1;

        initCommand(process_str1(old_line), &commands[(*size)++]);
    }
    initCommand(process_str1(line), &commands[(*size)++]);
}
void initCommand(char *str, Command *command)
{
    char buffer[MAX_STRING_SIZE] = {0};
    int offset = 0;
    int current = 0;
    while (sscanf(str + offset, "%s%n", buffer, &current) == 1)
    {
        offset += current;

        if (strcmp(buffer, ">") == 0)
        {
            if (sscanf(str + offset, "%s%n", buffer, &current) == 1)
            {
                offset += current;
                strcpy(command->output_file, buffer);
            }
        }
        else if (strcmp(buffer, "<") == 0)
        {
            if (sscanf(str + offset, "%s%n", buffer, &current) == 1)
            {
                offset += current;
                strcpy(command->input_file, buffer);
            }
        }
        else
        {
            strcpy(command->parameters[command->parameters_count++], buffer);
        }
    }
}

int is_built_in_command(Command *commands, int size)
{
    if (strcmp(commands[0].parameters[0], "set") == 0)
    {
        return 1;
    }
    else if (strcmp(commands[0].parameters[0], "quit") == 0)
    {
        return 1;
    }
    else if (strcmp(commands[0].parameters[0], "exit") == 0)
    {
        return 1;
    }
    else if (strcmp(commands[0].parameters[0], "cd") == 0)
    {
        return 1;
    }
    else if (strcmp(commands[0].parameters[0], "jobs") == 0)
    {
        return 1;
    }
    return 0;
}
void run_command_external(Command *commands, int size)
{
    int using_pipes[MAX_COMMAND_COUNT][2] = {0};
    pid_t pids[MAX_COMMAND_COUNT] = {-1};
    for (int i = 0; i < size; i++)
    {
        if (i + 1 < size)
        {
            pipe(using_pipes[i]);
        }
        pid_t p_id = fork();
        if (p_id < 0)
        {
            printf("fork error\n");
            break;
        }
        else if (p_id == 0)
        {
            if (i >= 1)
            {
                dup2(using_pipes[i - 1][0], 0);
            }
            else
            {
                if (commands[i].input_file[0])
                {
                    FILE *x = fopen(&(commands[i].input_file[0]), "r");
                    if (!x)
                    {
                        printf("%s\n", strerror(errno));
                        exit(errno);
                    }
                    else
                    {
                        dup2(fileno(x), 0);
                    }
                }
            }

            if (i + 1 < size)
            {
                dup2(using_pipes[i][1], 1);
            }
            else
            {
                if (commands[i].output_file[0])
                {
                    FILE *x = fopen(&(commands[i].output_file[0]), "w");
                    if (!x)
                    {
                        printf("%s\n", strerror(errno));
                        exit(errno);
                    }
                    else
                    {
                        dup2(fileno(x), 1);
                    }
                }
            }

            char *pars[MAX_PARAMETERS + 1] = {0};
            for (int j = 0; j < commands[i].parameters_count; j++)
            {
                pars[j] = commands[i].parameters[j];
            }
            if (execvpe(pars[0], pars, environ) != 0)
            {
                printf("%s\n", strerror(errno));
                exit(errno);
            }
        }
        else
        {
            if (i >= 1)
            {
                close(using_pipes[i - 1][0]);
            }
            if (i + 1 < size)
            {
                close(using_pipes[i][1]);
            }
            pids[i] = p_id;
        }
    }

    for (int i = 0; i < size; i++)
    {
        if (pids[i] != -1)
        {
            int status = 0;
            waitpid(pids[i], &status, 0);
            int cur = WEXITSTATUS(status);
            if (cur != 0)
            {
                errno = cur;
            }
        }
    }
}
void run_command_inner(Command *commands, int size)
{
    if (strcmp(commands[0].parameters[0], "quit") == 0 || strcmp(commands[0].parameters[0], "exit") == 0)
    {
        stop_now = 1;
    }
    else if (strcmp(commands[0].parameters[0], "set") == 0)
    {
        char *key = 0;
        char *value = 0;
        for (int i = 1; i < commands[0].parameters_count; i++)
        {
            char *cur = commands[0].parameters[i];
            if (strcmp(cur, "=") == 0)
            {
                continue;
            }
            else if (strstr(cur, "="))
            {
                char *x = strstr(cur, "=");
                key = cur;
                value = x + 1;
                *x = 0;
            }
            else if (!key)
            {
                key = cur;
            }
            else if (!value)
            {
                value = cur;
            }
            if (key && value)
            {
                setenv(key, value, 1);
                break;
            }
        }
    }
    else if (strcmp(commands[0].parameters[0], "cd") == 0)
    {
        const char *dest_dir = 0;
        if (commands[0].parameters_count > 1)
        {
            dest_dir = commands[0].parameters[commands[0].parameters_count - 1];
        }
        else
        {
            dest_dir = getenv("HOME");
        }
        if (dest_dir)
        {
            chdir(dest_dir);
        }
    }
    else if (strcmp(commands[0].parameters[0], "jobs") == 0)
    {
        for (int i = 0; i < current_background_jobs_size; i++)
        {
            printf("[%d] %d %s\n", background_jobs[i].id, (int)background_jobs[i].p_id, background_jobs[i].original_str);
        }
    }
    else
    {
        run_command_external(commands, size);
    }
}
void run_command(Command *commands, char *original_str, int size, int background)
{
    if (size == 0 || commands[0].parameters_count == 0)
        return;
    if (is_built_in_command(commands, size))
    {
        run_command_inner(commands, size);
        return;
    }
    if (background == 1)
    {
        if (current_background_jobs_size >= MAX_BACKGROUND_JOBS)
            return;
        pid_t p_id = fork();
        if (p_id < 0)
        {
            printf("fork error\n");
            return;
        }
        if (p_id > 0)
        {
            printf("[%d] %d running in background\n", unique_id, (int)p_id);
        }
        if (p_id == 0)
        {
            run_command_inner(commands, size);
            exit(errno);
        }
        else
        {
            background_jobs[current_background_jobs_size].id = unique_id++;
            strcpy(background_jobs[current_background_jobs_size].original_str, original_str);
            background_jobs[current_background_jobs_size].p_id = p_id;

            current_background_jobs_size++;
        }
    }
    else
    {
        run_command_inner(commands, size);
    }
}

void sigchildHandler(int signo)
{
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
    {
        int find_index = -1;
        for (int i = 0; i < current_background_jobs_size; i++)
        {
            if (background_jobs[i].p_id == pid)
            {
                find_index = i;
                break;
            }
        }
        if (find_index >= 0)
        {
            PROMPT_BEGIN;
            printf("[%d] %d finished %s\n", background_jobs[find_index].id, (int)pid, background_jobs[find_index].original_str);
            PROMPT_END;
            for (int i = find_index; i + 1 < current_background_jobs_size; i++)
            {
                background_jobs[i] = background_jobs[i + 1];
            }
            current_background_jobs_size--;
        }
    }
}