#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define SHARED_MEM_NAME_F_FUNC "/shared_mem_function_F"
#define SHARED_MEM_NAME_G_FUNC "/shared_mem_function_G"

int f(int x)
{
    return (x % 2 == 0);
}

int g(int x)
{
    return (x > 0);
}

void *calc_f_routine(void *arg)
{
    int fd = -1;
    int *result = NULL;
    int x = *(int *)arg;
    fd = shm_open(SHARED_MEM_NAME_G_FUNC, O_RDWR, 0600);
    result = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    *result = f(x);

    munmap(result, sizeof(int));
    close(fd);
}

void *calc_g_routine(void *arg)
{
    int fd = -1;
    int *result = NULL;
    int x = *(int *)arg;

    fd = shm_open(SHARED_MEM_NAME_G_FUNC, O_RDWR, 0600);
    result = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    *result = g(x);

    munmap(result, sizeof(int));
    close(fd);
}

int main(int, char **)
{
    int flags = O_CREAT | O_RDWR;
    int fd_f = -1, fd_g = -1;
    mode_t mode = 0666;
    int *result_f = NULL, *result_g = NULL;
    pthread_t tid_f, tid_g;
    int x = 0;

    // create shared memory for results of g() and f() functions
    fd_f = shm_open(SHARED_MEM_NAME_F_FUNC, flags, mode);
    fd_g = shm_open(SHARED_MEM_NAME_G_FUNC, flags, mode);
    result_f = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_f, 0);
    result_g = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_g, 0);

    *result_f = *result_g = -1; // undefined state

    printf("\nInput x: ");
    scanf("%d", &x);

    pthread_create(&tid_f, NULL, calc_f_routine, &x);
    pthread_create(&tid_g, NULL, calc_g_routine, &x);

    while (*result_f == -1 || *result_g == -1)
    {
        printf ("No result returned. Continue calculation? <Y/n>");
        if (getchar() == 'n')
        {
            break;
        }
    }

    pthread_join(tid_f, NULL);
    pthread_join(tid_g, NULL);

    // delete shared memory
    munmap(result_f, sizeof(int));
    munmap(result_g, sizeof(int));
    close(fd_f);
    close(fd_g);
    shm_unlink(SHARED_MEM_NAME_F_FUNC);
    shm_unlink(SHARED_MEM_NAME_G_FUNC);

    return 0;
}