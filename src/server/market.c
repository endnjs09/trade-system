#include "../../include/market.h"
#include "../../include/types.h"
#include "../../include/client_manager.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

/*
    시장 데이터 관리 모듈

    - Coin 구조체 배열로 코인 정보 관리
    - 시장 초기화, 코인 조회, 등락률 계산 등의 함수 
    - 시장 데이터 보호를 위한 mutex
*/


static Coin coins[MAX_COINS];   // 코인 배열
static int coin_count = 0;  // 현재 등록된 코인 수

 // 시장 데이터 보호를 위한 뮤텍스
static pthread_mutex_t market_mutex = PTHREAD_MUTEX_INITIALIZER;

// rank용 구조체
typedef struct {
    int user_id;            // 사용자 ID
    int total_asset;        // 총 자산 (현금 + 코인 가치)
    double profit_rate;     // 수익률
} Rank;


// 서버용 코인 추가
static void add_coin(const char *symbol, int base_price, int server_quantity) {
    if (coin_count >= MAX_COINS) return;

    int id = coin_count++;
    
    coins[id].id = id;
    strncpy(coins[id].symbol, symbol, SYMBOL_LEN - 1);
    coins[id].symbol[SYMBOL_LEN - 1] = '\0'; 
    coins[id].base_price = base_price;
    coins[id].current_price = base_price;
    coins[id].server_quantity = server_quantity;
    coins[id].initial_server_quantity = server_quantity;
}


// 시장 초기화
void init_market() {
    pthread_mutex_lock(&market_mutex);
    coin_count = 0;  // 코인 수 초기화
    for (int i = 0; i < MAX_COINS; i++) {
        coins[i].id = -1;  
        coins[i].symbol[0] = '\0';
        coins[i].base_price = 0;
        coins[i].current_price = 0;
        coins[i].server_quantity = 0;
        coins[i].initial_server_quantity = 0;
    }

    add_coin("XXX", 10000, 1000);
    add_coin("YYY", 5000, 1500);
    add_coin("ZZZ", 20000, 2000);

    pthread_mutex_unlock(&market_mutex);
}



// 코인 심볼로 coin_id 찾기
int find_coin(const char *symbol) {
    if (symbol == NULL) return -1;

    pthread_mutex_lock(&market_mutex);

    for (int i = 0; i < coin_count; i++) {
        if (strcmp(coins[i].symbol, symbol) == 0) {
            pthread_mutex_unlock(&market_mutex);
            return coins[i].id;
        }
    }

    pthread_mutex_unlock(&market_mutex);
    return -1;
}



// coin_id로 Coin 정보 조회
Coin *get_coin(int coin_id) {
    if (coin_id < 0 || coin_id >= coin_count) {
        return NULL;
    }

    pthread_mutex_lock(&market_mutex);
    Coin *coin = &coins[coin_id];
    pthread_mutex_unlock(&market_mutex);

    return coin;
}



// 현재 등록된 코인 수
int get_coin_count() {
    return coin_count;
}



// 전체 코인 배열 반환
Coin *get_coins() {
    return coins;
}



// 등락률 계산
static double calc_change_rate(Coin *coin) {
    if (coin == NULL || coin->base_price == 0) {
        return 0.0;
    }

    return ((double)(coin->current_price - coin->base_price) / coin->base_price) * 100.0;

}



// MARKET 응답 문자열
void market_msg(char *buf, int size) {
    if (buf == NULL || size <= 0) {
        return;
    }

    pthread_mutex_lock(&market_mutex);

    int offset = 0;

    offset += snprintf(buf + offset, size - offset, "[MARKET]\n");
    offset += snprintf(buf + offset, size - offset, "%-8s %-10s %-10s\n", "SYMBOL", "PRICE", "CHANGE");

    for (int i = 0; i < coin_count; i++) {
        double change = calc_change_rate(&coins[i]);

        offset += snprintf(buf + offset, size - offset, 
            "%-8s %-10d %+0.2f%%\n", coins[i].symbol, coins[i].current_price, change);

        if (offset >= size) {
            break;
        }
    }

    pthread_mutex_unlock(&market_mutex);
}

// MY 응답 문자열
void my_msg(User *user,char *buf, int size) {
    if (buf == NULL || size <= 0) {
        return;
    }

    if (user == NULL) {
        snprintf(buf, size, "ERR INTERNAL_SERVER_ERROR\n");
        return;
    }

    pthread_mutex_lock(&market_mutex);

    int total_asset = user->cash;
    int offset = 0;

    offset += snprintf(buf + offset, size - offset, "[MY]\n");
    offset += snprintf(buf + offset, size - offset, "User: %s\n", user->name);
    offset += snprintf(buf + offset, size - offset, "Cash: %d\n", user->cash);
    offset += snprintf(buf + offset, size - offset, "\n[HOLDINGS]\n");

    for (int i = 0; i < coin_count; i++) {
        int qty = user->holdings[i];
        int value = qty * coins[i].current_price;

        total_asset += value;

        offset += snprintf(buf + offset, size - offset, "%s: %d (value=%d)\n", coins[i].symbol, qty, value);

        if (offset >= size) {
            break;
        }
    }

    offset += snprintf(buf + offset, size - offset, "\nTotal Asset: %d\n", total_asset);

    pthread_mutex_unlock(&market_mutex);
}



// 총 자산 계산 (현금 + 보유 코인 가치)
static int calc_total_asset(User *user) {
    if (user == NULL) return 0;

    int total = user->cash;

    for (int i = 0; i < coin_count; i++) {
        total += user->holdings[i] * coins[i].current_price;
    }

    return total;
}

// RANK 응답 문자열
void rank_msg(char *buf, int size) {
    if (buf == NULL || size <= 0) return;

    pthread_mutex_lock(&market_mutex);

    User *users = get_users();
    int user_count = get_userCount();

    Rank ranks[MAX_USERS];
    int rank_count = 0;

    for (int i = 0; i < user_count; i++) {
        User *user = &users[i];
        if (user->id < 0) continue;

        int total_asset = calc_total_asset(user);
        double profit_rate = 0.0;
        if (user->initial_asset > 0) {
            // 수익률 계산: (총 자산 - 초기 자산) / 초기 자산 * 100
            profit_rate = ((double)(total_asset - user->initial_asset) / user->initial_asset) * 100.0;
        }

        ranks[rank_count].user_id = user->id;
        ranks[rank_count].total_asset = total_asset;
        ranks[rank_count].profit_rate = profit_rate;
        rank_count++;
    }

    // 내림차순 정렬 (총자산 기준)
    for (int i = 0; i < rank_count - 1; i++) {
        for (int j = i + 1; j < rank_count; j++) {
            if (ranks[i].total_asset < ranks[j].total_asset) {
                Rank temp = ranks[i];
                ranks[i] = ranks[j];
                ranks[j] = temp;
            }
        }
    }
    
    int offset = 0;
    offset += snprintf(buf + offset, size - offset, "[RANK]\n");
    offset += snprintf(buf + offset, size - offset, "%-5s %-10s %-12s %-10s\n", "RANK", "USER", "ASSET", "PROFIT");

    for (int i = 0; i < rank_count; i++) {
        offset += snprintf(buf + offset, size - offset, "%-5d U%-9d %-12d %+0.2f%%\n", 
            i + 1, ranks[i].user_id, ranks[i].total_asset, ranks[i].profit_rate);

        if (offset >= size) {
            break;
        }
    }

    pthread_mutex_unlock(&market_mutex);
}