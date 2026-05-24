#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include "types.h"

typedef struct {
    // 매수 주문
    Order buy_orders[MAX_ORDERS];
    int buy_count;

    // 매도 주문
    Order sell_orders[MAX_ORDERS];
    int sell_count; 
} OrderBook;

// 전체 호가창 초기화
void init_orderbooks();


// 특정 코인의 호가창 메시지
void orderbook_msg(int coin_id, char *buf, int size);

// 매수 주문 처리
void process_buy_order(int user_id, int coin_id, int price, int quantity, char *buf, int size);

// 매도 주문 처리
void process_sell_order(int user_id, int coin_id, int price, int quantity, char *buf, int size);

// 내 주문 조회 메시지
void my_orders_msg(int user_id, char *buf, int size);

#endif

