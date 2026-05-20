#include "../../include/handler.h"
#include "../../include/client_manager.h"
#include "../../include/protocol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#define MAX_TOKENS 8

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