#include "../../include/handler.h"
#include "../../include/client_manager.h"
#include "../../include/protocol.h"
#include "../../include/market.h"
#include "../../include/orderbook.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#define MAX_TOKENS 8

/*
    클라이언트 명령어 처리 모듈

    - 클라이언트로부터 명령어를 받아서 처리하는 함수: handle_command 
    - 각 명령어별로 별도의 핸들러 함수 구현 
    (market_handler, my_handler, dom_handler, buy_handler, sell_handler, rank_handler)
    - 클라이언트에게 메시지 전송: send_to_client 함수
*/


// 클라이언트에게 메시지 전송
static void send_to_client(int session_id, const char *msg) {
    // session_id로 소켓 번호 찾기
    CliSession *session = get_session(session_id);

    if (session == NULL || session->socket_fd < 0) return;

    send(session->socket_fd, msg, strlen(msg), 0);
}



// 문자열 끝 개행 제거
// LOGIN A 1234\n -> 여기서 \n, \r 등 제거
static void trimLine(char *str) {
    if (str == NULL) return;

    int len = strlen(str);

    while(len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}


// 명령어 문자열 쪼개기 ("LOGIN", "A", "1234")
static int parse_tok(char *input, char *argv[]) {
    int argc = 0;
    char *tok = strtok(input, " \t");    // 띄어쓰기 기준으로 나눔
    
    while(tok != NULL && argc < MAX_TOKENS) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t");
        
        // argv[0] = "LOGIN"
        // argv[1] = "A"
        // argv[2] = "1234"
    }

    return argc;
}


// 회원가입: REGISTER <NAME> <PW>
static void register_handler(int session_id, int argc, char *argv[]) {
    if (argc != 3) {   
        send_to_client(session_id, "USAGE: REGISTER <NAME> <PASSWORD>\n");
        return;
    }

    const char *name = argv[1];
    const char *pw = argv[2];
    
    // 이름 입력 안함
    if (strlen(name) == 0) {
        send_to_client(session_id, ERR_INVALID_NAME);
        return;
    }
    // 비밀번호 입력 안함
    if (strlen(pw) == 0) {
        send_to_client(session_id, ERR_INVALID_PASSWORD);
        return;
    }

    // 이미 존재하는 사용자
    if (find_user(name) != -1) {
        send_to_client(session_id, ERR_USER_EXISTS);
        return;
    }

    int user_id = register_user(name, pw);
    if (user_id < 0) {
        send_to_client(session_id, ERR_INTERNAL);
        return;
    }

    send_to_client(session_id, MSG_REGISTER_SUCCESS);
} 



// 로그인: LOGIN <NAME> <PW>
static void login_handler(int session_id, int argc, char *argv[]) {
    if (argc != 3) {
        send_to_client(session_id, "USAGE: LOGIN <NAME> <PASSWORD>\n");
        return;
    }

    // 이미 로그인 되어 있음
    if (islogin(session_id)) {
        send_to_client(session_id, ERR_ALREADY_LOGGED_IN);
        return;
    }

    const char *name = argv[1];
    const char *pw = argv[2];

    int user_id = find_user(name);
    // 사용자 존재 여부 체크
    if (user_id == -1) {
        send_to_client(session_id, ERR_USER_NOT_FOUND);
        return;
    }

    User *user = get_user(user_id);
    if (user == NULL) {
        send_to_client(session_id, ERR_INTERNAL);
        return;
    }

    // 비밀번호 확인
    if (strcmp(user->password, pw) != 0) {
        send_to_client(session_id, ERR_WRONG_PASSWORD);
        return;
    }


    int result = login(session_id, name, pw);
    if (result < 0) {
        send_to_client(session_id, ERR_INTERNAL);
        return;
    }

    send_to_client(session_id, MSG_LOGIN_SUCCESS);
}



// LOGOUT
static void logout_handler(int session_id, int argc, char *argv[]) {
    (void)argv;

    if (argc != 1) {
        send_to_client(session_id, "ERR USAGE LOGOUT\n");
        return;
    }

    // 로그인 되어있지 않음
    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    // 세션 해제 (로그아웃)
    logout(session_id);

    send_to_client(session_id, MSG_LOGOUT_SUCCESS);
}



// HELP
static void help_handler(int session_id, int argc, char *argv[]) {
    (void)argv;

    if (argc != 1) {
        send_to_client(session_id, "ERR USAGE HELP\n");
        return;
    }

    send_to_client(session_id, HELP_MESSAGE);
}


// 코인의 현재가, 등락률 조회
static void market_handler(int session_id, int argc, char *argv[]) {
    (void)argv; // argv는 사용하지 않지만 함수 시그니처 맞추기 위해서 남겨두는 것

    if (argc != 1) {
        send_to_client(session_id, "USAGE: MARKET\n");
        return;
    }

    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    char msg[BUFSIZE];
    memset(msg, 0,sizeof(msg));
    market_msg(msg, sizeof(msg));
    send_to_client(session_id, msg);
}


// MY 명령어 처리
static void my_handler(int session_id, int argc, char *argv[]) {
    (void)argv;

    if (argc != 1) {
        send_to_client(session_id, "USAGE: MY\n");
        return;
    }

    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    int user_id = get_session_userid(session_id);
    User *user = get_user(user_id);
    if (user == NULL) {
        send_to_client(session_id, ERR_INTERNAL);
        return;
    }

    char msg[BUFSIZE];
    memset(msg, 0, sizeof(msg));
    my_msg(user, msg, sizeof(msg));
    send_to_client(session_id, msg);
}


// DOM <COIN> 명령어 처리
static void dom_handler(int session_id, int argc, char *argv[]) {
    if (argc != 2) {
        send_to_client(session_id, "USAGE: DOM <COIN>\n");
        return;
    }

    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    const char *symbol = argv[1];
    int coin_id = find_coin(symbol);
    if (coin_id < 0) {
        send_to_client(session_id, "ERR UNKNOWN_COIN\n");
        return;
    }

    char msg[BUFSIZE];
    memset(msg, 0, sizeof(msg));
    orderbook_msg(coin_id, msg, sizeof(msg));
    send_to_client(session_id, msg);
}


// BUY <COIN> <PRICE> <QTY> 명령어 처리
static void buy_handler(int session_id, int argc,char *argv[]) {
    if (argc != 4) {
        send_to_client(session_id, "USAGE: BUY <COIN> <PRICE> <QTY>\n");
        return;
    }

    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    const char *symbol = argv[1];   // 코인 심볼
    int price = atoi(argv[2]);      // 가격
    int qty = atoi(argv[3]);        // 수량

    if (price <= 0 || qty <= 0) {
        send_to_client(session_id, "ERR INVALID_ORDER\n");
        return;
    }

    int coin_id = find_coin(symbol);

    if (coin_id < 0) {
        send_to_client(session_id, "ERR UNKNOWN_COIN\n");
        return;
    }

    int user_id = get_session_userid(session_id);

    char msg[BUFSIZE];
    memset(msg, 0, sizeof(msg));
    process_buy_order(user_id, coin_id, price, qty, msg, sizeof(msg));
    send_to_client(session_id, msg);
}


// SELL <COIN> <PRICE> <QTY> 명령어 처리
static void sell_handler(int session_id, int argc, char *argv[]) {
    if (argc != 4) {
        send_to_client(session_id, "USAGE: SELL <COIN> <PRICE> <QTY>\n");
        return;
    }

    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    const char *symbol = argv[1];   // 코인 심볼
    int price = atoi(argv[2]);      // 가격
    int qty = atoi(argv[3]);        // 수량

    if (price <= 0 || qty <= 0) {
        send_to_client(session_id, "ERR INVALID_ORDER\n");
        return;
    }

    int coin_id = find_coin(symbol);

    if (coin_id < 0) {
        send_to_client(session_id, "ERR UNKNOWN_COIN\n");
        return;
    }

    int user_id = get_session_userid(session_id);

    char msg[BUFSIZE];
    memset(msg, 0, sizeof(msg));
    process_sell_order(user_id, coin_id, price, qty, msg, sizeof(msg));
    send_to_client(session_id, msg);
}


// RANK 명령어 처리
static void rank_handler(int session_id, int argc, char *argv[]) {
    (void)argv;

    if (argc != 1) {
        send_to_client(session_id, "USAGE: RANK\n");
        return;
    }

    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    char msg[BUFSIZE];
    memset(msg, 0, sizeof(msg));
    rank_msg(msg, sizeof(msg));
    send_to_client(session_id, msg);
}


// ORDERS 명령어 처리
static void orders_handler(int session_id, int argc, char *argv[]) {
    (void)argv; 

    if (argc != 1) {
        send_to_client(session_id, "USAGE: ORDERS\n");
        return;
    }

    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    int user_id = get_session_userid(session_id);
    if (user_id < 0) {
        send_to_client(session_id, ERR_INTERNAL);
        return;
    }

    char msg[BUFSIZE];
    memset(msg, 0, sizeof(msg));
    my_orders_msg(user_id, msg, sizeof(msg));
    send_to_client(session_id, msg);
}


static void cancel_handler(int session_id, int argc, char *argv[]) {

    if (argc != 2) {
        send_to_client(session_id, "USAGE: CANCEL <ORDER_ID>\n");
        return;
    }

    if (!islogin(session_id)) {
        send_to_client(session_id, ERR_LOGIN_REQUIRED);
        return;
    }

    int user_id = get_session_userid(session_id);
    if (user_id < 0) {
        send_to_client(session_id, ERR_INTERNAL);
        return;
    }

    int order_id = atoi(argv[1]);
    if (order_id <= 0) {
        send_to_client(session_id, "ERR INVALID_ORDER_ID\n");
        return;
    }

    char msg[BUFSIZE];
    memset(msg, 0, sizeof(msg));
    cancel_order(user_id, order_id, msg, sizeof(msg));
    send_to_client(session_id, msg);
}





// 서버의 client_thread에서 호출하는 명령어 처리
int handle_command(int session_id, char *input) {
    if (input == NULL) {
        send_to_client(session_id, ERR_INVALID_COMMAND);
        return CMD_CONTINUE;
    }

    trimLine(input);    // 문자열 개행 제거

    if (strlen(input) == 0) {
        send_to_client(session_id, ERR_INVALID_COMMAND);
        return CMD_CONTINUE;
    }

    // 명령어 파싱
    char *argv[MAX_TOKENS];
    int argc = parse_tok(input, argv);
    if (argc == 0) {
        send_to_client(session_id, ERR_INVALID_COMMAND);
        return CMD_CONTINUE;
    }

    // 1. 회원가입 REGISTER
    if (strcmp(argv[0], CMD_REGISTER) == 0) {
        register_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 2. 로그인 LOGIN
    if (strcmp(argv[0], CMD_LOGIN) == 0) {
        login_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 3. 로그아웃 LOGOUT
    if (strcmp(argv[0], CMD_LOGOUT) == 0) {
        logout_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }
    
    // 4. HELP
    if (strcmp(argv[0], CMD_HELP) == 0) {
        help_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 5. MARKET
    if (strcmp(argv[0], CMD_MARKET) == 0) {
        market_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 6. MY
    if (strcmp(argv[0], CMD_MY) == 0) {
        my_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 7. DOM <COIN>
    if (strcmp(argv[0], CMD_DOM) == 0) {
        dom_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 8. BUY <COIN> <PRICE> <QTY>
    if (strcmp(argv[0], CMD_BUY) == 0) {
        buy_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 9. SELL <COIN> <PRICE> <QTY>
    if (strcmp(argv[0], CMD_SELL) == 0) {
        sell_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 10. RANK
    if (strcmp(argv[0], CMD_RANK) == 0) {
        rank_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }

    // 11. ORDERS
    if (strcmp(argv[0], CMD_ORDERS) == 0) {
        orders_handler(session_id, argc, argv);
        return CMD_CONTINUE;    
    }

    // 12. CANCEL <ORDER_ID>
    if (strcmp(argv[0], CMD_CANCEL) == 0) {
        cancel_handler(session_id, argc, argv);
        return CMD_CONTINUE;
    }




    // QUIT
    if (strcmp(argv[0], CMD_QUIT) == 0) {
        if (argc != 1) {
            send_to_client(session_id, "ERR USAGE QUIT\n");
            return CMD_CONTINUE;
        }

        send_to_client(session_id, MSG_QUIT);
        return CMD_CLOSE;
    }

    // 알 수 없는 명령어
    send_to_client(session_id, ERR_UNKNOWN_COMMAND);

    return CMD_CONTINUE;
}