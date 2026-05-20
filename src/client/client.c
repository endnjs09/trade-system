#include "../../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>


int main(int argc, char *argv[]) {
    int sock, nbyte;
    struct sockaddr_in servaddr;
    char input[BUFSIZE];
    char buf[BUFSIZE];
    
    const char *server_ip = "127.0.0.1";

    if (argc >= 2) {
        server_ip = argv[1];
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip, &servaddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    printf("[CLIENT] Connected to server %s:%d\n", server_ip, SERVER_PORT);

    // [INFO] CONNECTED 출력
    memset(buf, 0, sizeof(buf));
    nbyte = recv(sock, buf, sizeof(buf), 0);
    if (nbyte < 0) {
        perror("recv");
        exit(1);
    }
    buf[nbyte] = '\0';
    printf("%s", buf);


    while(1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (strcmp(input, "\n") == 0) continue;

        // 서버로 명령어 전송
        if (send(sock, input, strlen(input), 0) < 0) {
            perror("send");
            break;
        }

        // 서버 응답 수신
        memset(buf, 0, sizeof(buf));
        nbyte = recv(sock, buf, sizeof(buf), 0);
        if (nbyte < 0) {
            perror("recv");
            exit(1);
        }
        buf[nbyte] = '\0';
        printf("%s", buf);

        // QUIT
        if (strncmp(input, CMD_QUIT, strlen(CMD_QUIT)) == 0) {
            break;
        }

    }

    close(sock);

    printf("[CLIENT] Disconnected.\n");

    return 0;
}
