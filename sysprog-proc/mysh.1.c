#include "mysh.h"
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/** Run a node and obtain an exit status. */

int invoke_node(node_t *node) {
    LOG("Invoke: %s", inspect_node(node));
    switch (node->type) {
    int return_status, status;
    case N_COMMAND:

        for (int i = 0; node->argv[i] != NULL; i++) {
            LOG("node->argv[%d]: \"%s\"", i, node->argv[i]);

            int status;
            if (i == 0)
            {
                pid_t pid = fork();
                if (pid == 0)
                {
                    fflush(stdout);
                    execvp(node->argv[0], node->argv);
                    exit(errno);
                }
                else {
                    wait(&status); 
                    if (WIFEXITED(status)) {
                        LOG("status %d", WEXITSTATUS(status));
                        return WEXITSTATUS(status);
                    }
                }
            }
        }
        return 0;

    case N_PIPE: /* foo | bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));

        int save_stdin = dup(0);
        int save_stdout = dup(1);

        int fd[2];
        pipe(fd);
        
        dup2(fd[1], 1);
        close(fd[1]);
        invoke_node(node->lhs);
        dup2(save_stdout, 1);

        dup2(fd[0], 0);
        close(fd[0]);
        invoke_node(node->rhs);
        dup2(save_stdin, 0);

        return 0;


        // node_t* lhs = node->lhs;
        // node_t* rhs = node->rhs;

        // int old_fd[2];//, status1;
        // pipe(old_fd);
        // pid_t pid1;//, pid2;
        // pid1 = fork();
        // int count_pipe = 2;
        // int return_status = 0;
        // if (pid1 == 0) {
        //     dup2(old_fd[1], 1);
        //     close(old_fd[0]);
        //     close(old_fd[1]);
        //     fflush(stdout);
        //     execvp(lhs->argv[0], lhs->argv);
        // } 

        // while(rhs->type != N_COMMAND) {
        //     count_pipe++;
        //     node_t* next = rhs->lhs;
        //     int fd[2];
        //     pipe(fd);
        //     if ((pid1 = fork()) == 0) {
        //         dup2(old_fd[0], 0);
        //         dup2(fd[1], 1);
        //         close(old_fd[0]);
        //         close(old_fd[1]);
        //         close(fd[0]);
        //         close(fd[1]);
        //         LOG("Child2 gonna exe");
        //         fflush(stdout);
        //         execvp(next->argv[0], next->argv);
        //     } else {
        //         close(old_fd[1]);
        //         LOG("Finish child1");
        //     }
        //     rhs = rhs->rhs;
        //     old_fd[0] = fd[0];
        //     old_fd[1] = fd[1];
        // }

        // // Last Process
        // int status;
        // if ((pid1 = fork()) == 0) {
        //     dup2(old_fd[0], 0);
        //     close(old_fd[1]);
        //     close(old_fd[0]);
        //     LOG("Child2 gonna exe");
        //     fflush(stdout);
        //     execvp(rhs->argv[0], rhs->argv);
        // } else {
        //     close(old_fd[0]);
        //     close(old_fd[1]);
        //     for (int i = 0; i < count_pipe; i++){
        //         wait(&status);
        //         if (WIFEXITED(status)) {
        //                 return_status = status | return_status;
        //                 LOG("status %d", WEXITSTATUS(return_status));
        //             }
        //     }
        // }

        // return return_status;

    case N_REDIRECT_IN:     /* foo < bar */
    case N_REDIRECT_OUT:    /* foo > bar */
    case N_REDIRECT_APPEND: /* foo >> bar */
        LOG("node->filename: %s", node->filename);
        // int return_status;
        if (node->type == N_REDIRECT_IN) {
            pid_t pid;
            int save_stdin = dup(0);
            int fd = open(node->filename, O_RDONLY);
            if (fd == -1) {
                perror("File error");
                return 1;
            }
            dup2(fd, 0);
            return_status = invoke_node(node->lhs);
            close(fd);
            // fflush(stdin);
            dup2(save_stdin, 0);
            close(save_stdin);
        }

        else {
            pid_t pid;
            int save_stdout = dup(1);
            int fd;
            if (node->type == N_REDIRECT_OUT) fd = open(node->filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            else fd = open(node->filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (fd == -1) {
                perror("File error");
                return 1;
            }
            dup2(fd, 1);
            close(fd);
            return_status = invoke_node(node->lhs);
            dup2(save_stdout, 1);
            close(save_stdout);
        }
        return return_status;

    case N_SEQUENCE: /* foo ; bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));
        int l_status = invoke_node(node->lhs);
        int r_status = invoke_node(node->rhs);
        return l_status | r_status;

    case N_AND: /* foo && bar */
    case N_OR:  /* foo || bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));
        l_status = invoke_node(node->lhs);
        if (l_status == 0) {
            if (node->type == N_AND) return invoke_node(node->rhs);
            else return l_status;
        } else {
            if (node->type == N_AND) return l_status;
            else return invoke_node(node->rhs);
        }

    case N_SUBSHELL: /* ( foo... ) */
        LOG("node->lhs: %s", inspect_node(node->lhs));

        pid_t pid;
        if ((pid = fork()) == 0) {
            int return_status;
            return_status = invoke_node(node->lhs);
            exit(return_status);
        } else {
            wait(&status);
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status);
                    LOG("status %d", WEXITSTATUS(return_status));
                }
            }

        return 0;

    default:
        return 0;
    }
}
