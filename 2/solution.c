#include "parser.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int
execute_command_line(const struct command_line *line, int *ptr)
{
    assert(line != NULL);

    int file_descriptor;

    const struct expr *e = line->head;

    int prev_pipe[2];

    pipe(prev_pipe);

    int fists = 0;
    int c = 0;

    while (e != NULL)
    {

        if (e->type == EXPR_TYPE_COMMAND)
        {

            char **array_govna;
            if (strcmp(e->cmd.exe, "cd") == 0)
            {
                chdir(e->cmd.args[0]);
            }
            else if (strcmp(e->cmd.exe, "exit") == 0 && line->tail == line->head)
            {
                *ptr = 1;
                if (e->cmd.arg_count > 0)
                {
                    return atoi(e->cmd.args[0]);
                }
                return 0;
            }
            else
            {
                int next_pipe[2];
                int state = 0;

                if (e->next != NULL && e->next->type == EXPR_TYPE_PIPE && e->next->next != NULL && e->next->next->type == EXPR_TYPE_COMMAND)
                {
                    pipe(next_pipe);
                    state = 1;
                }

                array_govna = malloc((e->cmd.arg_count + 2) * sizeof(char **));
                array_govna[0] = e->cmd.exe;
                for (uint32_t i = 0; i < e->cmd.arg_count; i++)
                {
                    array_govna[i + 1] = e->cmd.args[i];
                }
                array_govna[e->cmd.arg_count + 1] = NULL;

                pid_t pid = fork();

                if (pid == 0)
                {
                    if (state == 1)
                    {
                        dup2(next_pipe[1], STDOUT_FILENO);
                        close(next_pipe[0]);
                    }

                    if (fists != 0)
                    {
                        dup2(prev_pipe[0], STDIN_FILENO);
                        close(prev_pipe[1]);
                    }

                    if (e->next == NULL && line->out_type == OUTPUT_TYPE_FILE_NEW)
                    {
                        file_descriptor = open(line->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (file_descriptor == -1)
                        {
                            perror("Не удалось открыть файл");
                            exit(EXIT_FAILURE);
                        }
                        dup2(file_descriptor, STDOUT_FILENO);
                    }

                    if (e->next == NULL && line->out_type == OUTPUT_TYPE_FILE_APPEND)
                    {
                        file_descriptor = open(line->out_file, O_WRONLY | O_APPEND, 0644);
                        if (file_descriptor == -1)
                        {
                            perror("Не удалось открыть файл");
                            exit(EXIT_FAILURE);
                        }
                        dup2(file_descriptor, STDOUT_FILENO);
                    }

                    int f = execvp(array_govna[0], array_govna);

                    if (f == -1)
                        execlp("sh", "sh", "-c", "exit", "45", NULL);
                }
                else
                {
                    if (fists > 0)
                    {
                        close(prev_pipe[0]);
                        close(prev_pipe[1]);
                    }

                    if (state == 1)
                    {
                        prev_pipe[0] = next_pipe[0];
                        prev_pipe[1] = next_pipe[1];
                    }
                }

                free(array_govna);

                if (fists == 0)
                {
                    fists = 100;
                }
                state = 0;
                c++;
            }
        }
        e = e->next;
    }

    int exit_status = 0;

    for (int i = 0; i < c; i++)
    {
        wait(NULL);
    }

    close(file_descriptor);

    return exit_status;
}

int main(void)
{
    const size_t buf_size = 1024;
    char buf[buf_size];
    int rc;
    int flag = 0;
    int num = 0;
    int *ptr = &num;

    struct parser *p = parser_new();
    while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0)
    {
        parser_feed(p, buf, rc);
        struct command_line *line = NULL;
        while (true)
        {
            enum parser_error err = parser_pop_next(p, &line);
            if (err == PARSER_ERR_NONE && line == NULL)
            {
                break;
            }
            if (err != PARSER_ERR_NONE)
            {
                printf("Error: %d\n", (int)err);
                continue;
            }
            flag = execute_command_line(line, ptr);
            command_line_delete(line);
            if (num == 1)
            {
                parser_delete(p);
                return flag;
            }
        }
    }
    parser_delete(p);
    return flag;
}