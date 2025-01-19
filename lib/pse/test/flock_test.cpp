#include <sys/file.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
int main()
{
    if (fork() != 0)
    {
        FILE *f = fopen("a.txt", "a");
        sleep(3);
        flock(fileno(f), LOCK_EX);
        fprintf(f, "hello\n");
        fflush(f);
        flock(fileno(f), LOCK_UN);
        printf("sleep 5\n");
        fflush(stdin);
        sched_yield();
        sleep(10);
        flock(fileno(f), LOCK_EX);
        fprintf(f, "hello\n");
        fflush(f);
        printf("sleep 10\n");
        fflush(stdin);
        flock(fileno(f), LOCK_UN);
    }
    else
    {
        FILE *f = fopen("a.txt", "a");
        sleep(7);
        flock(fileno(f), LOCK_EX);
        printf("sleep 7\n");
        fflush(stdin);
        ftruncate(fileno(f), 0);
        fprintf(f, "noooooooo\n");
        fflush(f);
        flock(fileno(f), LOCK_UN);
    }
}