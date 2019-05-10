#include "mysh.h"
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/** Run a node and obtain an exit status. */

pid_t execute_task(char *argv[]){
    fflush(stdout);
    pid_t pid = fork();
    if (pid == 0)
    {
        fflush(stdout);
        execvp(argv[0], argv);
        exit(errno);
    } else return pid;
}

int get_exit_status(pid_t *pid){
    int status, return_status=0;
    waitpid(*pid, &status,WUNTRACED);
    LOG("PID %d", pid); 
    if (WIFEXITED(status)) {
        LOG("status %d", WEXITSTATUS(status));
        return_status = WEXITSTATUS(status);
    }
    return return_status;
}

int invoke_node(node_t *node) {
    LOG("Invoke: %s", inspect_node(node));
    switch (node->type) {
    int return_status=0;
    pid_t pid;
    pid_t pid1, pid2;
    case N_COMMAND:

        for (int i = 0; node->argv[i] != NULL; i++) {
            LOG("node->argv[%d]: \"%s\"", i, node->argv[i]);
            if (i == 0)
            {
                pid_t pid_proc = execute_task(node->argv);
                return get_exit_status(&pid_proc);
            }
        }
        return 0;

    case N_PIPE: /* foo | bar */
        LOG("node->lhs: %s", inspect_node(node->lhs));
        LOG("node->rhs: %s", inspect_node(node->rhs));

        int fd[2];
        pipe(fd);

        if ((pid1 = fork()) == 0) {
            dup2(fd[1], 1);
            close(fd[1]);
            close(fd[0]);
            exit(invoke_node(node->lhs));
        } 
        
        if ((pid2 = fork()) == 0) {
            dup2(fd[0], 0);
            close(fd[1]);
            close(fd[0]);
            exit(invoke_node(node->rhs));
        }
        else {
            int return_status1=0, return_status2=0;
            close(fd[1]);
            close(fd[0]);
            LOG("Before close pipe1");
            return_status1 = get_exit_status(&pid1);
            LOG("Before close pipe2");
            return_status2 = get_exit_status(&pid2);

            // Return status of RHS
            return return_status2;
        }

    case N_REDIRECT_IN:     /* foo < bar */
    case N_REDIRECT_OUT:    /* foo > bar */
    case N_REDIRECT_APPEND: /* foo >> bar */
        LOG("node->filename: %s", node->filename);
        if (node->type == N_REDIRECT_IN) {
            pid_t pid=0;
            if ((pid == fork()) == 0){
                int fd = open(node->filename, O_RDONLY);
                if (fd == -1) {
                    perror("File error");
                    return 1;
                }
                dup2(fd, 0);
                close(fd);
                exit(invoke_node(node->lhs));
            } else return_status = get_exit_status(&pid);
        } else {
            pid_t pid=0;
            if ((pid == fork()) == 0) {
                int fd;
                if (node->type == N_REDIRECT_OUT) fd = open(node->filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                else fd = open(node->filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
                if (fd == -1) {
                    perror("File error");
                    return 1;
                }
                dup2(fd, 1);
                close(fd);
                exit(invoke_node(node->lhs));
            } else{
                return_status = get_exit_status(&pid);
            }
            
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
        if ((pid = fork()) == 0) {
            int return_status;
            return_status = invoke_node(node->lhs);
            exit(return_status);
        } else return get_exit_status(&pid);

    default:
        return 0;
    }
}

