#include "mysh.h"
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/** Run a node and obtain an exit status. */

void execute_task(char *argv[]){
    pid_t pid = fork();
    if (pid == 0)
    {
        fflush(stdout);
        execvp(argv[0], argv);
        exit(errno);
    }
}

int get_exit_status(){
    int status;
    wait(&status); 
    if (WIFEXITED(status)) {
        LOG("status %d", WEXITSTATUS(status));
        return WEXITSTATUS(status);
    }
}

int invoke_node_noexit(node_t *node, int *count) {
    LOG("Invoke: %s", inspect_node(node));
    switch (node->type) {
    int return_status, status;
    case N_COMMAND:

        for (int i = 0; node->argv[i] != NULL; i++) {
            LOG("node->argv[%d]: \"%s\"", i, node->argv[i]);

            int status;
            if (i == 0)
            {
                execute_task(node->argv);
                *count++;
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
        invoke_node_noexit(node->lhs, count);
        dup2(save_stdout, 1);
        dup2(fd[0], 0);

        invoke_node_noexit(node->rhs, count);
        dup2(save_stdin, 0);

        return status;

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
            return_status = invoke_node_noexit(node->lhs, count);
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
            return_status = invoke_node_noexit(node->lhs, count);
            dup2(save_stdout, 1);
            close(save_stdout);
        }
        return return_status;

    case N_SEQUENCE: /* foo ; bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));
        int l_status = invoke_node_noexit(node->lhs, count);
        int r_status = invoke_node_noexit(node->rhs, count);
        return l_status | r_status;

    case N_AND: /* foo && bar */
    case N_OR:  /* foo || bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));
        l_status = invoke_node_noexit(node->lhs, count);
        if (l_status == 0) {
            if (node->type == N_AND) return invoke_node_noexit(node->rhs, count);
            else return l_status;
        } else {
            if (node->type == N_AND) return l_status;
            else return invoke_node_noexit(node->rhs, count);
        }

    case N_SUBSHELL: /* ( foo... ) */
        LOG("node->lhs: %s", inspect_node(node->lhs));

        pid_t pid;
        if ((pid = fork()) == 0) {
            int return_status;
            return_status = invoke_node_noexit(node->lhs, count);
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
                execute_task(node->argv);
                return get_exit_status();
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
        int count = 0;
        invoke_node_noexit(node->lhs, &count);
        dup2(save_stdout, 1);

        dup2(fd[0], 0);
        close(fd[0]);
        invoke_node_noexit(node->rhs, &count);
        dup2(save_stdin, 0);
        get_exit_status(); // LHS
        status = get_exit_status(); // RHS

        return status;

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
        } else return get_exit_status();

        return 0;

    default:
        return 0;
    }
}

