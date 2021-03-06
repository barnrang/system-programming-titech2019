#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void child() {
    printf("In child(): pid=%d, ppid=%d\n", getpid(), getppid());
    sleep(1);
    printf("In child(): Process %d terminated.\n", getpid());
    exit(0);
}

int main() {
    printf("In main():  pid=%d, ppid=%d\n", getpid(), getppid());

    /* 課題1A:
     * 子プロセスを生成し，次のchild()を子プロセスでのみ実行するようにしなさい
     */
    pid_t pid = fork();
    int status;
    if (pid == 0) child();

    /* 課題1B: ここで，生成した子プロセスの終了を待つようにしなさい */
    else wait(&status);

    printf("In main():  Process %d terminated.\n", getpid());
    return 0;
}
