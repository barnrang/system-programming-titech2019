#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h> /* signal */

#if 1
#define FIRE "\xF0\x9F\x94\xA5\n"
#else
#define FIRE "FIRE\n"
#endif

void async_handler(int sig){
  int status;
  waitpid(-1, &status, WNOHANG);
  printf(FIRE);
}

char *not_found_body = "<html><body><center><h1>404 Not Found!</h1></center></body></html>\n";

int find_file(char *path) {
    struct stat info;
    if (stat(path, &info) == -1) {
        return -1;
    }
    return (int)info.st_size;        // file size
}

uint my_strcmp(char* str1, char* str2){
    int strlen1 = strlen(str1);
    int strlen2 = strlen(str2);
    if (strlen1 != strlen2) return 0;
    for (int i = 0; i < strlen1; i++) {
        if (str1[i] != str2[i]) return 0;
    }
    return 1;
}

char *get_content_type(char *filename) {
    if (filename[strlen(filename) - 1] == 'g') {        // jpeg or jpg
        return "image/jpeg";

    } else if (filename[strlen(filename) - 1] == 'f') {        // gif
        return "image/gif";

    } else if (filename[strlen(filename) - 1] == 'l' || filename[strlen(filename) - 1] == 'm') {        // html or htm
        return "text/html";

    /* others */
    } else {
        return "text/plain";
    }
}

void send_content_data(FILE *from, FILE *to) {
    char buf[BUFSIZ];
    int counter = 0;
    while (1) {
        int size = fread(buf, 1, BUFSIZ, from);
        if (size > 0) {
            fwrite(buf, size, 1, to);
        } else {
            break;
        }
        counter++;
    }
}

void session(int fd, char *cli_addr, int cli_port) {

    FILE *fin, *fout;
    fin = fdopen(fd, "r"); fout = fdopen(fd, "w");

    int cut_connection = 0;

    while(cut_connection==0){
        /* read request line */
        char request_buf[BUFSIZ];
        if (fgets(request_buf, sizeof(request_buf), fin) == NULL) {
            fflush(fout);
            fclose(fin);
            fclose(fout);
            close(fd);
            return;        // disconnected
        }

        /* parse request line */
        char method[BUFSIZ];
        char uri[BUFSIZ], *path;
        char version[BUFSIZ];
        sscanf(request_buf, "%s %s %s", method, uri, version);
        path = &(uri[1]);

        printf("HTTP Request: %s %s %s %s\n", method, uri, path, version);
        fflush(stdout);

        // fflush(fout);
        fprintf(fout, "HTTP/1.1 200 OK\r\n");

        /* read/parse header lines */
        while (1) {

            /* read header lines */
            char headers_buf[BUFSIZ];
            if (fgets(headers_buf, sizeof(headers_buf), fin) == NULL) {
                fflush(fout);
                fclose(fin);
                fclose(fout);
                close(fd);
                return;            // disconnected from client
            }
            /* check header end */
            if (strcmp(headers_buf, "\r\n") == 0) {
                break;
            } 

            /* parse header lines */
            char header[BUFSIZ];
            char value[BUFSIZ];
            sscanf(headers_buf, "%s %s", header, value);

            printf("Header: %s %s\n", header, value);
            if ((strcmp(header, "Connection:") == 0) && (strcmp(value, "close") == 0)){
                printf("Ready to close connection");
                cut_connection = 1;
            }
            fflush(stdout);

        } // while


        /*

        送信部分を完成させなさい．

        */
        char* use_path;
        if (strcmp(path, "")==0) {
            printf("Use default\n");
            char* tmp = "index.html";
            use_path = tmp;
        } else use_path = path;
        long long not_found_size = strlen(not_found_body);
        char* content_type = get_content_type(use_path);
        fprintf(fout, "Content-Type: %s\r\n", content_type);
        FILE* file = fopen(use_path, "r");
        if (file == NULL) {
            fprintf(fout, "Content-Length: %d\r\n", not_found_size);
        } else {
            fseek(file, 0L, SEEK_END);
            fprintf(fout, "Content-Length: %d\r\n", ftell(file));
        }
        fclose(file);
        fprintf(fout, "\r\n");



        if (!strcmp(method, "GET")) {
            printf("%s\n", content_type);
            printf("Incoming attack\n");
            if (strcmp(content_type, "text/html") == 0) {
                printf("%s\n", use_path);
                FILE* f_html = fopen(use_path, "r");
                if (f_html == NULL) {
                    printf("Not found\n");
                    fprintf(fout, "%s", not_found_body);  
                } 
                else {
                    char buf[BUFSIZ];
                    printf("Reading\n");
                    while (fgets(buf, sizeof(buf), f_html)){
                        printf("Read\n");
                        fprintf(fout, "%s", buf);
                    }
                }
                fclose(f_html);
            }
             
            else if (strcmp(content_type, "image/jpeg") == 0) {
                // fprintf(fout, "<img src=\"%s\" alt=\"%s\">\n", uri, uri);
                FILE* f_html = fopen(use_path, "rb");
                if (f_html == NULL) {
                    printf("Not found\n");
                    fprintf(fout, "%s", not_found_body);  
                } 
                else {
                    char buf[BUFSIZ];
                    printf("Reading\n");
                    while (fgets(buf, sizeof(buf), f_html)){
                        printf("Read\n");
                        fprintf(fout, "%s", buf);
                    }
                }
                fclose(f_html);
            } else {
                printf("file type not found!");
                fprintf(fout, "%s", not_found_body);  
            }
            
        } 
        else {
            printf("NOT GET\n");
            FILE* f_html = fopen("./index.html", "r");
            if (f_html == NULL) {
                printf("Not found\n");
                fprintf(fout, "%s", not_found_body);  
            } 
            else {
                char buf[BUFSIZ];
                printf("Reading\n");
                while (fgets(buf, sizeof(buf), f_html)){
                    fprintf(fout, "%s", buf);
                }
            }
        }
        fflush(fout);
    }


    /* close connection */
    fclose(fin);
    fclose(fout);
    close(fd);
    printf("Connection closed.\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {

    int listfd, connfd, optval = 1, port = 10000;
    unsigned int addrlen;
    struct sockaddr_in saddr, caddr;

    if (argc >= 2) {
        port = atoi(argv[1]);
    }

    listfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listfd, (struct sockaddr *)&saddr, sizeof(saddr));

    listen(listfd, 10);

    while (1) {
        addrlen = sizeof(caddr);
        connfd = accept(listfd, (struct sockaddr *)&caddr, (socklen_t*)&addrlen);
        pid_t pid;
        if ((pid = fork()) == 0){
            session(connfd, inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
            exit(0);
        } else {
            close(connfd);
            signal(SIGCHLD, async_handler);
        }
    }

    return 0;
}
