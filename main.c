#include <stdio.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#define SHARED_MEM_NAME_F_FUNC "/shared_mem_function_f"
#define SHARED_MEM_NAME_G_FUNC "/shared_mem_function_g"
#define SIZE sizeof(int)

int f(int x)
{
    return (x % 2 == 0);
}

int g(int x)
{
    return (x > 0);
}

void *f_routine(void *arg)
{
    int fd = -1;
    int *result = NULL;
    int x = *(int *)arg;
    fd = shm_open(SHARED_MEM_NAME_F_FUNC, O_RDWR, 0600);
    result = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    *result = f(x);

    munmap(result, SIZE);
    close(fd);
}

void *g_routine(void *arg)
{
    int fd = -1;
    int *result = NULL;
    int x = *(int *)arg;

    fd = shm_open(SHARED_MEM_NAME_G_FUNC, O_RDWR, 0600);
    result = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    *result = g(x);

    munmap(result, SIZE);
    close(fd);
}

int operation_or(int result_f, int result_g)
{
    if (result_f == 1 || result_g == 1)
    {
        return 1;
    }

    if (result_g == 0 && result_g == 0)
    {
        return 0;
    }
}

int main(int, char **)
{
    int flags = O_CREAT | O_RDWR;
    int fd_f = -1, fd_g = -1;
    mode_t mode = 0600;
    int *result_f = NULL, *result_g = NULL;
    pthread_t tid_f, tid_g;
    int x = 0;
    int result = -1;
    char answer = 'y';

    // create shared memory for results of g() and f() functions
    umask (0000);
    fd_f = shm_open(SHARED_MEM_NAME_F_FUNC, flags, mode);
    fd_g = shm_open(SHARED_MEM_NAME_G_FUNC, flags, mode);
    ftruncate(fd_f, SIZE);
    ftruncate(fd_g, SIZE);
    result_f = (int *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_f, 0);
    result_g = (int *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_g, 0);

    *result_f = *result_g = -1; // undefined state

    printf("\nInput x: ");
    scanf("%d", &x);

    pthread_create(&tid_f, NULL, f_routine, &x);
    pthread_create(&tid_g, NULL, g_routine, &x);

    //usleep(100);
    while (*result_f == -1 || *result_g == -1)
    {
        printf("\nNo result returned. Continue calculation? <y/n>");
        scanf(" %c", &answer);
        if ((answer = tolower(answer)) == 'n')
        {
            break;
        }
        answer = 'y';
    }

    pthread_join(tid_f, NULL);
    pthread_join(tid_g, NULL);

    if (answer == 'y')
    {
        result = operation_or(*result_f, *result_g);
        printf("\nf(%d) || g(%d) = %d\n", x, x, result);
    }

    // delete shared memory
    munmap(result_f, SIZE);
    munmap(result_g, SIZE);
    close(fd_f);
    close(fd_g);
    shm_unlink(SHARED_MEM_NAME_F_FUNC);
    shm_unlink(SHARED_MEM_NAME_G_FUNC);

    return 0;
}