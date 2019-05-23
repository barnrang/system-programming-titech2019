#include <signal.h> /* signal */
#include <stdio.h>
#include <unistd.h> /* write */

void handler(int sig){
  write(STDOUT_FILENO, "muri", 4);
}

void alarm_handler(int sig){
  alarm(5);
  write(STDOUT_FILENO, "*", 1);
}

int main() {
    /* 課題2: 端末で ^C (Ctrl-C) をタイプしても殺せない(無視する)ようにせよ．*/

    /* 課題3: 5秒ごとに標準エラー出力に「*」を表示せよ．*/
    alarm(5);
    signal(SIGALRM, alarm_handler);

    /* 標準エラー出力にドットを表示し続ける */
    signal(SIGINT, handler);
    for (;;) {
        for (int j = 0; j < 500000000; j++)
            ; /* busy loop */
        write(STDERR_FILENO, ".", 1);
    }
    return 0;
}
