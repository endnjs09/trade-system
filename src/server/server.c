#include "../../include/client_manager.h"
#include "../../include/protocol.h"
#include "../../include/handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

void *client_thread(void *arg);

// 클라이언트 접속 -> 세션 생성 -> client_thread로 보냄 -> 각 스레드가 처리

// int argc, char *argv[]
int main () {
    int listen_sock;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t addrlen = sizeof(cliaddr);
    pthread_t tid;

    init_client();

    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket");
        return 1;
    }

    // 소켓 옵션 (서버 껐다 키면 에러 나는 거 방지)
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERVER_PORT);

    if (bind(listen_sock, (struct sockaddr*)&servaddr, addrlen) < 0) {
        perror("bind");
        close(listen_sock);
        return 1;
    }

    if (listen(listen_sock, MAX_CLIENTS) < 0) {
        perror("listen");
        close(listen_sock);
        return 1;
    }

    printf("[SERVER] Listening on port %d...\n", SERVER_PORT);
    while (1) {
        int accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &addrlen);
        if (accp_sock < 0) {
            perror("accept");
            continue;
        }

        printf("[SERVER] New connection from %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
        
        int session_id = create_session(accp_sock);
        if (session_id < 0) {
            send(accp_sock, "ERR SERVER_FULL\n", strlen("ERR SERVER_FULL\n"), 0);
            close(accp_sock);
            continue;
        }

        // session_id 담을 메모리 할당
        int *arg = malloc(sizeof(int));
        if (arg == NULL) {
            perror("malloc");
            remove_session(session_id);
            close(accp_sock);
            continue;
        }

        *arg = session_id;  // session_id는 계속 바뀌가 때문에 
        // thread 하나 새로 만들고 client_thread 실행
        if (pthread_create(&tid, NULL, client_thread, arg) != 0) {
            perror("pthread_create");
            free(arg);
            remove_session(session_id);
            close(accp_sock);
            continue;
        }

        pthread_detach(tid);
    }

    close(listen_sock);

    return 0;
    
}


// 클라이언트 하나 처리하는 스레드
void *client_thread(void *arg) {
    // main에서 넘긴 session_id 꺼냄
    int session_id = *(int *)arg;
    free(arg);

    CliSession *session = get_session(session_id);
    if (session == NULL) return NULL;   // 세션 정보가 없으면 NULL

    int accp_sock = session->socket_fd; 
    char buf[BUFSIZE];
    printf("[SERVER] Client connected. session_id=%d\n", session_id);

    // 연결 됐다고 알림 보내기
    send(accp_sock, "[INFO] CONNECTED\n", strlen("[INFO] CONNECTED\n"), 0);

    // echo 테스트
    while(1) {
        memset(buf, 0, sizeof(buf));

        int nbyte = recv(accp_sock, buf, sizeof(buf), 0);
        if (nbyte <= 0) break;
        buf[nbyte] = '\0';

        printf("[CLIENT %d] %s", session_id, buf);

        int result = handle_command(session_id, buf);
        if (result == CMD_CLOSE) {
            break;
        }
    }

    remove_session(session_id);
    close(accp_sock);
    printf("[SERVER] Client disconnected. session_id=%d\n", session_id);

    return NULL;
}