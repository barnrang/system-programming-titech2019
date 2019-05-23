#include "mysh.h"
#include <signal.h> /* signal */
#include <errno.h>    /* errno */
#include <stdbool.h>  /* bool */
#include <string.h>   /* strcmp */
#include <sys/wait.h> /* wait */
#include <unistd.h>   /* fork */
#include <stdlib.h>
#include <string.h>

/* うまく表示されない場合は，下記の 1 を 0 に切り替えて下さい */
#if 1
#define FIRE "\xF0\x9F\x94\xA5\n"
#else
#define FIRE "FIRE\n"
#endif

/**
 * 引数の2文字列が等しければ真．NULLの場合は両方ともNULLであれば真．
 */
bool streql(const char *lhs, const char *rhs) {
    return (lhs == NULL && rhs == NULL) ||
           (lhs != NULL && rhs != NULL && strcmp(lhs, rhs) == 0);
}

void async_handler(int sig){
  int status;
  waitpid(-1, &status, WNOHANG);
  write(STDERR_FILENO, FIRE, 5);
}

pid_t pid;
pid_t pause_pid=0;

void pause_handler(int sig){
  // int pid = getpid();
  LOG("%d", pid);
  pause_pid = pid;
  kill(pid, SIGSTOP);
}

bool check_fg_bg(char* argv[]){
    return ((strcmp(argv[0], "bg") == 0) || 
        (strcmp(argv[0], "fg") == 0));
}

int invoke_node(node_t *node) {
    LOG("Invoke: %s", inspect_node(node));
    // pid_t pid;

    LOG("paused pid = %d", pause_pid);

    bool is_fg_or_bg = check_fg_bg(node->argv);

    if (strcmp(node->argv[0], "bg") == 0) node->async=true;

    /* & 付きで起動しているか否か */
    if (node->async) {
        LOG("{&} found: async execution required");
    }

    /* 子プロセスの生成 */
    fflush(stdout);
    if (is_fg_or_bg) {
        pid = pause_pid;
        pause_pid = 0;
        kill(pid, SIGCONT);
    }
    else pid = fork();
    if (pid == -1) {
        PERROR_DIE(node->argv[0]);
    }

    if (pid == 0) {
        /* 子プロセス側 */
        if (execvp(node->argv[0], node->argv) == -1) {
            PERROR_DIE(node->argv[0]);
        }
        return 0; /* never happen */
    } else {
        /* 親プロセス側 */

        /* 子に独立したプロセスグループを割り振る */
        if (setpgid(pid, pid) == -1 && !is_fg_or_bg) {
            PERROR_DIE(NULL);
        }

        /* 生成した子プロセスを待機 */
        int status;
        int options = WUNTRACED;
        if (node->async){
          signal(SIGCHLD, async_handler);
          return pid;
        } else {
          // pid_t pid_kill;
            LOG("yehehhe");
            signal(SIGTSTP, pause_handler);
            pid_t waited_pid = waitpid(pid, &status, options);
            if (waited_pid == -1) {
                if (errno == ECHILD) {
                    /* すでに成仏していた（何もしない） */
                } else {
                    PERROR_DIE(NULL);
                }
              } else {
                  LOG("Waited: pid=%d, status=%d, exit_status=%d", waited_pid, status,
                      WEXITSTATUS(status));
                  if (WIFEXITED(status)) {
                      write(STDERR_FILENO, FIRE, 5);
                  }
              }
              return pid;
          // kill(pid, SIGTSTP);
          }
        }

}

/* A hook point to initialize shell */
void init_shell(void) {
    LOG("Initializing mysh2...");
}
