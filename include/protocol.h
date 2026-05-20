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
#define CMD_BUY "BUY"
#define CMD_SELL "SELL"
#define CMD_RANK "RANK"

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
"  REGISTER <name> <password> : create new user\n" \
"  LOGIN <name>               : start login\n" \
"  PASS <password>            : enter password after LOGIN\n" \
"  LOGOUT                     : logout current user\n" \
"  HELP                       : show command list\n" \
"  QUIT                       : disconnect from server\n"

#endif