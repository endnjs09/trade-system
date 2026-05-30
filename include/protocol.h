#ifndef PROTOCOL_H
#define PROTOCOL_H

#define SERVER_PORT 9000

#define MULTICAST_IP "239.0.0.1"
#define MULTICAST_PORT 9001
#define BUFSIZE 1024

// 클라이언트 명령어
#define CMD_REGISTER "REGISTER"
#define CMD_LOGIN "LOGIN"
#define CMD_LOGOUT "LOGOUT"
#define CMD_HELP "HELP"
#define CMD_QUIT "QUIT"
#define CMD_MARKET "MARKET"
#define CMD_MY "MY"
#define CMD_DOM "DOM"
#define CMD_ORDERS "ORDERS"
#define CMD_BUY "BUY"
#define CMD_SELL "SELL"
#define CMD_RANK "RANK"
#define CMD_CANCEL "CANCEL"

// 서버 응답
#define RES_OK "OK"
#define RES_ERR "ERR"
#define RES_INFO "INFO"
#define RES_EVENT "EVENT"

// 성공 메시지
#define MSG_REGISTER_SUCCESS "REGISTER_SUCCESS\n"
#define MSG_PASSWORD_REQUIRED "INFO PASSWORD_REQUIRED\n"
#define MSG_LOGIN_SUCCESS "LOGIN_SUCCESS\n"
#define MSG_LOGOUT_SUCCESS "LOGOUT_SUCCESS\n"
#define MSG_QUIT "GOOD BYE\n"

// 에러 메시지
#define ERR_UNKNOWN_COMMAND "ERR UNKNOWN_COMMAND\n"
#define ERR_INVALID_COMMAND "ERR INVALID_COMMAND\n"
#define ERR_LOGIN_REQUIRED "ERR LOGIN_REQUIRED\n"
#define ERR_ALREADY_LOGGED_IN "ERR ALREADY_LOGGED_IN\n"
#define ERR_WRONG_PASSWORD "ERR WRONG_PASSWORD\n"
#define ERR_USER_NOT_FOUND "ERR USER_NOT_FOUND\n"
#define ERR_USER_EXISTS "ERR USER_EXISTS\n"
#define ERR_INVALID_NAME "ERR INVALID_NAME\n"
#define ERR_INVALID_PASSWORD "ERR INVALID_PASSWORD\n"
#define ERR_INTERNAL "ERR INTERNAL_SERVER_ERROR\n"

// HELP
#define HELP_MESSAGE \
"INFO COMMANDS\n" \
"  REGISTER <name> <password> : 회원가입\n" \
"  LOGIN <name> <password>    : 로그인\n" \
"  MARKET                     : 시장 가격 보기\n" \
"  MY                         : 포르폴리오 보기\n" \
"  DOM <coin>                 : 호가창 보기\n" \
"  BUY <coin> <price> <qty>   : 매수 주문 하기\n" \
"  SELL <coin> <price> <qty>  : 매도 주문 하기\n" \
"  CANCEL <order_id>          : 미체결 주문 취소\n" \
"  ORDERS                     : 나의 미체결 주문 보기\n" \
"  RANK                       : 순위 보기\n" \
"  LOGOUT                     : 로그아웃\n" \
"  HELP                       : 명령어 목록 보기\n" \
"  QUIT                       : 서버 연결 끊기\n"

#endif