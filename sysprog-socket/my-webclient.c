#include <sys/socket.h>     /* socket, connect */
#include <string.h>         /* memset */
#include <arpa/inet.h>      /* htons, inet_addr */
#include <sys/types.h>      /* read */
#include <sys/uio.h>        /* read */
#include <unistd.h>         /* read, write, close */
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

    int sockfd;

    /*

     ソケットの生成，サーバへの接続部分を完成させなさい．

    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(10000);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));


    /*

     HTTPリクエストの送信部分を完成させなさい．

    */
    FILE* fout;
    FILE* fin;
    fout = fdopen(sockfd, "w");
    fin = fdopen(sockfd, "r");

    fprintf(fout, "GET / HTTP/1.0\r\n");
    fprintf(fout, "Connection: close\r\n");
    fprintf(fout, "\r\n");
    fflush(fout);


    /*

     HTTPレスポンスを受信し，メッセージボディだけを標準出力へ出力しなさい．
  
    */

    char buf[1024];
    
    // Check for \r\n
    while(1) {
        if (fgets(buf, sizeof(buf), fin) == NULL){
            break;
        } else {
            if (!strcmp(buf, "\r\n")) break;
        }
    }

    // Begin writing response
    while(1) {
        if (fgets(buf, sizeof(buf), fin) == NULL){
            break;
        } else {
            fprintf(stdout, buf);
        }
    }

    // write(STDOUT_FILENO, "aaa\n", 3);

    fflush(stdout);
    fclose(fin);  
    fclose(fout);  
    close(sockfd);
    return 0;
}
