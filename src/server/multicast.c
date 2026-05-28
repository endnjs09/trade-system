#include "../../include/multicast.h"
#include "../../include/protocol.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

static int sock = -1;
static struct sockaddr_in mcastaddr;

void init_multicast() {
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return;
    }

    memset(&mcastaddr, 0, sizeof(mcastaddr));
    mcastaddr.sin_family = AF_INET;
    mcastaddr.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_IP, &mcastaddr.sin_addr);

    unsigned int ttl = 1;
    // ttl(생존시간) 설정
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("IP_MULTICAST_TTL");
        close(sock);
        sock = -1;
        return;
    }
    
    // 디버깅용
    printf("Multicast Send ready, IP = %s, PORT = %d\n", MULTICAST_IP, MULTICAST_PORT);
}

void multicast_send(const char* msg) {
    if (sock < 0 || msg == NULL) return;

    if (sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&mcastaddr, sizeof(mcastaddr)) < 0) {
        perror("multicast sendto");
    }
}
