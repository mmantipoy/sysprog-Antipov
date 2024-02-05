#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"
#include <time.h>
#include <limits.h>

void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

int partition(int arr[], int low, int high)
{
    int pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++)
    {
        if (arr[j] <= pivot)
        {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void quickSort(int arr[], int low, int high, struct timespec *time, int max)
{
    if (low < high)
    {

        int pi = partition(arr, low, high);

        quickSort(arr, low, pi - 1, time, max);
        quickSort(arr, pi + 1, high, time, max);

        struct timespec cur;
        clock_gettime(CLOCK_MONOTONIC, &cur);
        long long elapsed = ((cur.tv_sec - time->tv_sec) * 1000000000 + (cur.tv_nsec - time->tv_nsec)) / 1000;

        if (elapsed > max)
        {
            coro_yield();
            clock_gettime(CLOCK_MONOTONIC, time);
        }
    }
}

int *readAndWriteNumbers(const char *inputFileName, int *count)
{
    FILE *inputFile = fopen(inputFileName, "r");
    if (inputFile == NULL)
    {
        printf("Ошибка открытия входного файла");
        exit(EXIT_FAILURE);
    }

    int *numbers = NULL;
    int capacity = 1;
    *count = 0;

    numbers = (int *)malloc(capacity * sizeof(int));
    if (numbers == NULL)
    {
        printf("Ошибка выделения памяти");
        exit(EXIT_FAILURE);
    }

    int num;
    while (fscanf(inputFile, "%d", &num) == 1)
    {
        if (*count == capacity)
        {
            capacity += 1;
            numbers = (int *)realloc(numbers, capacity * sizeof(int));
            if (numbers == NULL)
            {
                printf("Ошибка перевыделения памяти");
                exit(EXIT_FAILURE);
            }
        }
        numbers[(*count)++] = num;
    }

    fclose(inputFile);

    return numbers;
}

struct my_context
{
    char *name;
    char **file;
    int **numbers;
    int *counts;
    int i;
    struct timespec time;
    int maxTime;
    int *que;
};

static struct my_context *my_context_new(char *name, char **file, int **numbers, int *counts, int i, int maxTime, int *que)
{
    struct my_context *ctx = malloc(sizeof(*ctx));
    ctx->name = strdup(name);
    ctx->file = file;
    ctx->numbers = numbers;
    ctx->counts = counts;
    ctx->i = i;
    clock_gettime(CLOCK_MONOTONIC, &ctx->time);
    ctx->maxTime = maxTime;
    ctx->que = que;

    return ctx;
}

static void my_context_delete(struct my_context *ctx)
{
    free(ctx->name);
    free(ctx);
}

static int coroutine_func_f(void *context)
{

    struct coro *this = coro_this();
    struct my_context *ctx = context;
    char *name = ctx->name;

    while (*ctx->que < ctx->i)
    {
        printf("Начало сортировки файла %s корутиной %s\n", ctx->file[*ctx->que + 3], name);
        int copy = *ctx->que;
        ctx->numbers[*ctx->que] = readAndWriteNumbers(ctx->file[*ctx->que + 3], &ctx->counts[*ctx->que]);
        *ctx->que = *ctx->que + 1;
        quickSort(ctx->numbers[copy], 0, ctx->counts[copy] - 1, &ctx->time, ctx->maxTime);
    }

    printf("Запуск корутины %s\n", name);
    printf("%s: количество переключений %lld\n", name, coro_switch_count(this));

    my_context_delete(ctx);
    return 0;
}

int main(int argc, char **argv)
{
    int **allNumbers = (int **)malloc((argc - 3) * sizeof(int *));
    int *counts = (int *)malloc((argc - 3) * sizeof(int));
    int que = 0;

    coro_sched_init();
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < atoi(argv[2]); ++i)
    {
        char name[16];
        sprintf(name, "coro_%d", i);
        coro_new(coroutine_func_f, my_context_new(name, argv, allNumbers, counts, argc - 3, atoi(argv[1]) / (argc - 3), &que));
    }

    struct coro *c;
    while ((c = coro_sched_wait()) != NULL)
    {
        printf("Закончена %d\n", coro_status(c));
        coro_delete(c);
    }

    const char *outputFileName = "out.txt";
    FILE *outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL)
    {
        printf("Ошибка открытия выходного файла");
        exit(EXIT_FAILURE);
    }

    int *mins = (int *)malloc((argc - 3) * sizeof(int));
    int *pos = (int *)malloc((argc - 3) * sizeof(int));
    int cap = 0;

    for (int i = 0; i < argc - 3; i++)
    {

        pos[i] = 0;
        mins[i] = allNumbers[i][0];
        cap += counts[i];
    }

    int min = INT_MAX;
    int io = argc;
    int c2 = 0;
    int f = 0;

    while (1)
    {

        for (int i = 0; i < argc - 3; i++)
        {
            if (mins[i] < min)
            {
                min = mins[i];
                io = i;
            }
        }
        fprintf(outputFile, "%d ", min);

        min = INT_MAX;
        pos[io] += 1;
        if (pos[io] == counts[io])
        {
            mins[io] = INT_MAX;
            f++;
        }
        else
        {
            mins[io] = allNumbers[io][pos[io]];
        }
        c2++;

        if (c2 == cap)
        {
            break;
        }

        if (f == argc - 3)
        {
            for (int i = 0; i < argc - 3; i++)
            {
                if (mins[i] != INT_MAX)
                {

                    while (pos[i] != counts[i])
                    {
                        fprintf(outputFile, "%d ", mins[pos[i]]);
                        pos[io] += 1;
                    }

                    break;
                }
            }

            break;
        }
    }

    free(mins);
    free(pos);

    for (int i = 0; i < argc - 3; i++)
    {
        free(allNumbers[i]);
    }

    free(allNumbers);
    free(counts);

    fclose(outputFile);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long long elapsed_ns = ((end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec)) / 1000;

    printf("Общее время работы программы: %lld микросекунд\n", elapsed_ns);
    printf("Програма успешно завершена\n");
    return 0;
}
