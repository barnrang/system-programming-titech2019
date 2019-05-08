#include "mycat.h"
#include "main.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @param size: オプション処理後の引数の個数
 * @param args: オプション処理後の引数群
 */
int do_cat(int size, char **args) {
    if (use_stdio) {
        /* -l 有効（課題3）*/
        // implement here
        for (int i = -1; i < size; i++){
            FILE *fd;
            if (i == -1) {
                if (size != 0) continue;
                fd = stdin;
            }
            if (i >= 0) {
                if ((strlen(args[i])==1) && (args[i][0] == '-')) fd = stdin;
                else fd = fopen(args[i], "r");
            }
            if (fd == NULL) {
                perror("Cannot open file");
                exit(1);
            }
            char buffer[buffer_size];
            ssize_t write_st = 1;
            while (1) { 
                write_st = fread(buffer, 1, buffer_size, fd);
                if (write_st < buffer_size){
                    fwrite(buffer, 1, write_st, stdout);
                    break;
                } else {
                    fwrite(buffer, 1, write_st, stdout);
                }
                
            }
            LOG("%d\n", fd->_bf._size);
            fclose(fd);
        }
    } else {
        /* -l 無効（課題1）*/
        // implement here
        for (int i = -1; i < size; i++){
            int fd;
            if (i == -1) {
                if (size != 0) continue;
                fd = STDIN_FILENO;
            } else {
                if ((strlen(args[i]) == 1) && (args[i][0] == '-')) fd = STDIN_FILENO;
                else fd = open(args[i], O_RDONLY);
                
            }
            if (fd == -1){
                perror("Cannot open file");
                exit(1);
            }
            char buffer[buffer_size];
            ssize_t write_st = 1;
            while (1) { 
                write_st = read(fd, buffer, buffer_size);
                if (write_st < buffer_size){
                    write(STDOUT_FILENO, buffer, write_st);
                    if (!follow) break;
                } else {
                    write(STDOUT_FILENO, buffer, buffer_size); 
                }
                
            }
            close(fd);
        }
    }
    return 0;
}
