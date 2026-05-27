#include "../../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

void *multicast_listener(void *arg);

int main(int argc, char *argv[]) {
    int sock, nbyte;
    struct sockaddr_in servaddr;
    char input[BUFSIZE];
    char buf[BUFSIZE];
    pthread_t tid;
        
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

    
    if (pthread_create(&tid, NULL, multicast_listener, NULL) != 0) {
        perror("pthread_create");
    }
    else pthread_detach(tid);    


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

void *multicast_listener(void *arg) {
    struct sockaddr_in mcast_group, from;
    struct ip_mreq mreq;
    char buf[BUFSIZE];
    int len, sock, nbyte;
    unsigned int yes = 1;
    
    memset(&mcast_group, 0, sizeof(mcast_group));
    mcast_group.sin_family = AF_INET;
    mcast_group.sin_addr.s_addr = htonl(INADDR_ANY);
    mcast_group.sin_port = htons(MULTICAST_PORT);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return NULL;
    }


    // 소켓 재사용 옵션
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("SO_REUSEADDR");
        return NULL;
    }
    
    // bind
    if (bind(sock, (struct sockaddr*)&mcast_group, sizeof(mcast_group)) < 0) {
        perror("bind");
        return NULL;
    }

    inet_pton(AF_INET, MULTICAST_IP, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    // 멀티캐스트 그룹에 가입
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("ADD_MEMBERSHIP");
        return NULL;
    }

    // multicast 수신
    for(;;) {
        len = sizeof(from);
        if ((nbyte = recvfrom(sock, buf, BUFSIZE - 1, 0, (struct sockaddr*)&from, &len)) < 0) {
            perror("recvfrom");
            return;
        }
        buf[nbyte] = '\0';

        printf("\n[MCAST] %s", buf);
        printf("> ");
        fflush(stdout);
    }

    return NULL;
}
