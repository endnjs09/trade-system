#ifndef TYPES_H
#define TYPES_H
#include <time.h>

#define MAX_USERS 100
#define MAX_CLIENTS 100
#define MAX_COINS 10
#define MAX_ORDERS 1000

#define NAME_LEN 32
#define PASSWORD_LEN 64
#define SYMBOL_LEN 16

#define INITIAL_CASH 1000000


// 클라이언트 인증 상태
typedef enum {
    AUTH_NONE = 0,        // 아직 로그인하지 않은 상태
    AUTH_LOGGED_IN        // 로그인 완료 상태
} AuthState;



// 사용자 계정 정보
typedef struct {
    int id;                         // 식별 아이디(번호)
    char name[NAME_LEN];            // 이름
    char password[PASSWORD_LEN];    // 비밀번호
    int cash;                       // 보유 자산
    int initial_asset;              // 초기 자산
    int holdings[MAX_COINS];        // 보유 코인
    int socket_fd;                  // 소켓 번호
    int connected;                  // 연결 유무
    int logged_in;                  // 로그인 유무
} User;



// 세션 정보
typedef struct {
    int socket_fd;          // 소켓 번호
    int user_id;            // 사용자 아이디(번호)
    AuthState auth_state;   // 인증 상태
} CliSession;



// 코인 정보
typedef struct {
    int id;                         // 식별 아이디(번호)
    char symbol[SYMBOL_LEN];        
    int base_price;                 //
    int current_price;              // 현재가
    int server_quantity;            // 서버 잔여
    int initial_server_quantity;    // 서버 초기 보유량
} Coin;




// 주문 방향
typedef enum {
    ORDER_BUY = 0,
    ORDER_SELL
} OrderSide;





// 주문 정보
typedef struct {
    int order_id;

    int user_id;
    int coin_id;

    OrderSide side;

    int price;
    int quantity;

    time_t created_at;
} Order;





// 거래 체결 기록
typedef struct {
    int trade_id;

    int coin_id;
    int buyer_id;
    int seller_id;

    int price;
    int quantity;

    time_t traded_at;
} Trade;

#endif