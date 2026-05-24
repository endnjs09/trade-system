#include "../../include/orderbook.h"
#include "../../include/market.h"
#include "../../include/client_manager.h"
#include "../../include/types.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

/*
    호가창 관리 모듈

    - OrderBook 구조체 배열로 코인별 호가창 관리
    - 매수/매도 주문 추가, 제거, 정렬 등의 함수
    - 호가창 메시지 생성 함수
    - 호가창 데이터 보호를 위한 mutex
*/


// 코인별 호가창
static OrderBook orderbooks[MAX_COINS];  
static pthread_mutex_t orderbook_mutex = PTHREAD_MUTEX_INITIALIZER;


void init_orderbooks() {
    pthread_mutex_lock(&orderbook_mutex);

    for(int i = 0; i < MAX_COINS; i++) {
        orderbooks[i].buy_count = 0;
        orderbooks[i].sell_count = 0;

        for (int j = 0; j < MAX_ORDERS; j++) {
            // 매도 주문 초기화
            orderbooks[i].buy_orders[j].user_id = -1;
            orderbooks[i].buy_orders[j].user_id = -1;
            orderbooks[i].buy_orders[j].coin_id = -1;
            orderbooks[i].buy_orders[j].side = ORDER_BUY;
            orderbooks[i].buy_orders[j].price = 0;
            orderbooks[i].buy_orders[j].quantity = 0;
            orderbooks[i].buy_orders[j].created_at = 0;

            // 매수 주문 초기화
            orderbooks[i].sell_orders[j].order_id = -1;
            orderbooks[i].sell_orders[j].user_id = -1;
            orderbooks[i].sell_orders[j].coin_id = -1;
            orderbooks[i].sell_orders[j].side = ORDER_SELL;
            orderbooks[i].sell_orders[j].price = 0;
            orderbooks[i].sell_orders[j].quantity = 0;
            orderbooks[i].sell_orders[j].created_at = 0;
        }
    }

    pthread_mutex_unlock(&orderbook_mutex);
}




// BUY(매수) 주문 정렬 -> 가격이 높은 순, 같은 가격이면 먼저 들어온 게 앞
static void sort_buy_orders(int coin_id) {
    OrderBook *book = &orderbooks[coin_id];
    for (int i = 0; i < book->buy_count - 1; i++) {
        for (int j = i + 1; j < book->buy_count; j++) {
            Order *a = &book->buy_orders[i];
            Order *b = &book->buy_orders[j];

            if (a->price < b->price || (a->price == b->price && a->created_at > b->created_at)) {
                Order temp = *a;
                *a = *b;
                *b = temp;
            }
        }
    }
}

// SELL(매도) 주문 정렬 -> 가격이 낮은 순, 같은 가격이면 먼저 들어온 게 앞
static void sort_sell_orders(int coin_id) {
    OrderBook *book = &orderbooks[coin_id];
    for (int i = 0; i < book->sell_count - 1; i++) {
        for (int j = i + 1; j < book->sell_count; j++) {
            Order *a = &book->sell_orders[i];
            Order *b = &book->sell_orders[j];

            if (a->price > b->price || (a->price == b->price && a->created_at > b->created_at)) {
                Order temp = *a;
                *a = *b;
                *b = temp;
            }
        }
    }
}

// 매수 맨 앞 주문 제거
static void remove_buy_order(int coin_id, int idx) {
    OrderBook *book = &orderbooks[coin_id];

    // 덮어쓰기
    for (int i = idx; i < book->buy_count - 1; i++) {
        book->buy_orders[i] = book->buy_orders[i + 1];
    }
    book->buy_count--;
}
        
// 매도 맨 앞 주문 제거
static void remove_sell_order(int coin_id, int idx) {
    OrderBook *book = &orderbooks[coin_id];

    // 덮어쓰기
    for (int i = idx; i < book->sell_count - 1; i++) {
        book->sell_orders[i] = book->sell_orders[i + 1];
    }
    book->sell_count--;
}


static int next_order_id = 1;  // 다음 주문 ID 

// 매수 주문 추가
static int add_buy_order(int user_id, int coin_id, int price, int quantity) {
    OrderBook *book = &orderbooks[coin_id];
    if (book->buy_count >= MAX_ORDERS) {
        return -1;  // 주문 수 초과
    }

    Order order;
    order.order_id = next_order_id++;
    order.user_id = user_id;        // 올린 유저
    order.coin_id = coin_id;        // 코인 id
    order.side = ORDER_BUY;         // 주문 방향(매수)
    order.price = price;            // 사용자가 사고 싶은 최대 가격 
    order.quantity = quantity;      // 주문 수량
    order.created_at = time(NULL);  // 주문 생성 시간

    book->buy_orders[book->buy_count++] = order;

    sort_buy_orders(coin_id);

    return order.order_id;
}

// 매도 주문 추가
static int add_sell_order(int user_id, int coin_id, int price, int quantity) {
    OrderBook *book = &orderbooks[coin_id];

    if (book->sell_count >= MAX_ORDERS) {
        return -1;
    }

    Order order;
    order.order_id = next_order_id++;
    order.user_id = user_id;        // 올린 유저
    order.coin_id = coin_id;        // 코인 id
    order.side = ORDER_SELL;         // 주문 방향(매도)
    order.price = price;            // 사용자가 팔고 싶은 최소 가격 
    order.quantity = quantity;      // 주문 수량
    order.created_at = time(NULL);  // 주문 생성 시간

    book->sell_orders[book->sell_count++] = order;

    sort_sell_orders(coin_id);

    return order.order_id;
}

// 서버에서 구매했을 때 가격 상승 계산
static int calc_server_price(Coin *coin, int quantity) {
    if (coin == NULL || quantity <= 0) {
        return coin->current_price;
    }

    // 최대 8% 상승, 서버 초기 보유량의 10% 거래마다 8% 상승
    int price_change = (coin->current_price * quantity * 8) / (coin->initial_server_quantity * 10);  
    if (price_change <= 0) {
        price_change = 1;  // 최소 가격 변동
    }

    return coin->current_price + price_change;
}

// BUY <COIN> <PRICE> <QTY> 명령어 처리
void process_buy_order(int user_id, int coin_id, int price, int quantity, char *buf, int size) {
    // ==================예외 처리==================
    if (buf == NULL || size <= 0) return;

    if (price <= 0 || quantity <= 0) {
        snprintf(buf, size, "ERR INVALID_ORDER\n");
        return;
    }

    User *buy_user = get_user(user_id);
    Coin *coin = get_coin(coin_id);
    if (buy_user == NULL || coin == NULL) {
        snprintf(buf, size, "ERR INTERNAL_SERVER_ERROR\n");
        return;
    }
    // ============================================

    pthread_mutex_lock(&orderbook_mutex);   // mutex 잠금 (동시성 문제 방지)

    OrderBook *book = &orderbooks[coin_id];

    int remain = quantity;  // 남은 수량
    int total_cost = 0;   // 총 거래 금액
    int total_filled = 0; // 총 체결 수량
    int old_price = coin->current_price;  // 거래 전 가격



    // 1. 매도 주문과 매칭 (기존 SELL 주문과 먼저 체결). BUY 가격 >= SELL 가격인 경우에만 체결
    int i = 0;
    while (i < book->sell_count && remain > 0) {
        Order *sell_order = &book->sell_orders[i];

        // BUY 가격이 SELL 가격보다 낮으면 더 이상 체결 불가능
        if (price < sell_order->price) break;  

        User *sell_user = get_user(sell_order->user_id);

        if (sell_user == NULL) {
            i++;    // 사용자 정보 없으면 건너뛰기
            continue;
        }

        int trade_qty =  0;
        if (remain < sell_order->quantity) trade_qty = remain;
        else trade_qty = sell_order->quantity;

        int trade_price = sell_order->price;  // 체결 가격: 매도 주문 가격
        int cost = trade_price * trade_qty;   // 거래 금액 = 체결 가격 * 체결 수량

        if (buy_user->cash < cost) break;    // 구매자에게 돈이 부족하면 건너뛰기

        

        // 매도 주문은 등록 시점에 이미 매도자 코인을 차감함
        // 그래서 매수자에게 코인을 주고 매도자에게 현금만 줌
        buy_user->cash -= cost;              // 구매자에게서 돈 차감
        buy_user->holdings[coin_id] += trade_qty;  // 구매자에게 코인 주기

        // 매도자에게 돈 주기 (매도자는 이미 코인을 차감했으므로 추가로 차감할 필요 없음)
        sell_user->cash += cost;             

        sell_order->quantity -= trade_qty;   // 매도 주문에서 체결 수량 차감
        total_cost += cost;                  // 총 거래 금액 누적
        total_filled += trade_qty;           // 총 체결 수량 누적
        remain -= trade_qty;                 // 남은 수량 차감

        coin->current_price = trade_price;  // 가격 업데이트

        // 매도 주문 완전히 체결됐으면(없어졌으면) 호가창에서 제거
        if (sell_order->quantity == 0) remove_sell_order(coin_id, i);
        else i++;    // 아직 남은 수량이 있으면 다음 매도 주문으로 넘어가기
    }


    // 2. SELL 주문이 없고 남은 수량이 있으면 서버에서 구매하기
    if (remain > 0 && book->sell_count == 0) {
        if (price >= coin->current_price && coin->server_quantity > 0) {
            int server_qty = remain;
            if (server_qty > coin->server_quantity) {
                server_qty = coin->server_quantity;
            }

            int server_cost = coin->current_price * server_qty;  // 서버에서 구매하는 가격은 현재가

            // 구매자에게 돈이 충분한지
            if (buy_user->cash >= server_cost) {
                buy_user->cash -= server_cost;              // 구매자에게서 돈 차감
                buy_user->holdings[coin_id] += server_qty;  // 구매자에게 코인 주기

                coin->server_quantity -= server_qty;         // 서버 잔여량 차감
                total_cost += server_cost;                  // 총 거래 금액 누적
                total_filled += server_qty;                 // 총 체결 수량 누적
                remain -= server_qty;                       // 남은 수량 차감

                // 서버에서 구매할 때마다 가격 상승
                coin->current_price = calc_server_price(coin, server_qty);
            }
        }
    }


    // 3. 남은 수량이 있으면 BUY 주문으로 호가창에 등록시킴 (현금 예약)
    if (remain > 0) {
        int reserve_cost = price * remain;  // 예약 금액 = 주문 가격 * 남은 수량

        if (buy_user->cash >= reserve_cost) {
            buy_user->cash -= reserve_cost;  // 예약 금액 차감

            int order_id = add_buy_order(user_id, coin_id, price, remain);
            if (order_id < 0) {
                buy_user->cash += reserve_cost;  // 주문 등록 실패하면 예약 금액 돌려줌
                pthread_mutex_unlock(&orderbook_mutex);
                snprintf(buf, size, "ERR ORDERBOOK_FULL\n");
                return;
            }

            snprintf(buf, size, "§ BUY filled=%d remain=%d order_id=%d cost=%d current=%d §\n", 
                total_filled, remain, order_id, total_cost, coin->current_price);
        }
        else {
            snprintf(buf, size, "§ BUY filled=%d remain=%d cost=%d current=%d WARN INSUFFICIENT_CASH_FOR_REMAINING\n §", 
                total_filled, remain, total_cost, coin->current_price);
        }
    }
    else {
        snprintf(buf, size, "§ BUY filled=%d cost=%d price_change=%d->%d §\n", 
            total_filled, total_cost, old_price, coin->current_price);
    }

    pthread_mutex_unlock(&orderbook_mutex);
}



// SELL <COIN> <PRICE> <QTY> 명령어 처리
void process_sell_order(int user_id, int coin_id, int price, int quantity, char *buf, int size) {
    // ==================예외 처리==================
    if (buf == NULL || size <= 0) return;

    if (price <= 0 || quantity <= 0) {
        snprintf(buf, size, "ERR INVALID_ORDER\n");
        return;
    }

    User *sell_user = get_user(user_id);
    Coin *coin = get_coin(coin_id);
    if (sell_user == NULL || coin == NULL) {
        snprintf(buf, size, "ERR INTERNAL_SERVER_ERROR\n");
        return;
    }

    if (sell_user->holdings[coin_id] < quantity) {
        snprintf(buf, size, "ERR INSUFFICIENT_COIN\n");
        return;
    }
    // ============================================

    pthread_mutex_lock(&orderbook_mutex);   // mutex 잠금 (동시성 문제 방지)

    OrderBook *book = &orderbooks[coin_id];

    int remain = quantity;  // 남은 수량
    int total_income = 0;   // 총 거래 금액(수입)
    int total_filled = 0;   // 총 체결 수량
    int old_price = coin->current_price;    // 거래 전 가격



    // 1. 기존 매수 주문과 매칭 (BUY 가격 >= SELL 가격인 경우에만 체결)
    int i = 0;
    while (i < book->buy_count && remain > 0) {
        Order *buy_order = &book->buy_orders[i];

        // SELL 가격이 BUY 가격보다 높으면 더 이상 체결 불가능
        if (buy_order->price < price) break;

        User *buy_user = get_user(buy_order->user_id);

        if (buy_user == NULL) {
            i++;    // 사용자 정보 없으면 건너뛰기
            continue;
        }

        int trade_qty =  0;
        if (remain < buy_order->quantity) trade_qty = remain;
        else trade_qty = buy_order->quantity;

        int trade_price = buy_order->price;  // 체결 가격: 매수 주문 가격
        int income = trade_price * trade_qty;   // 거래 금액 = 체결 가격 * 체결 수량
        

        // 매도 주문은 등록 시점에 이미 매도자 코인을 차감함
        // 그래서 매수자에게 코인을 주고 매도자에게 현금만 줌
        buy_user->holdings[coin_id] += trade_qty;  // 구매자에게 코인 주기
        sell_user->holdings[coin_id] -= trade_qty; // 매도자에게서 코인 차감

        // 매도자에게 돈 주기 (매도자는 이미 코인을 차감했으므로 추가로 차감할 필요 없음)
        sell_user->cash += income;              

        buy_order->quantity -= trade_qty;   // 매수 주문에서 체결 수량 차감
        remain -= trade_qty;              // 남은 수량 차감
        total_income += income;           // 총 거래 금액 누적
        total_filled += trade_qty;        // 총 체결 수량 누적

        coin->current_price = trade_price;  // 가격 업데이트

        // 매수 주문 완전히 체결됐으면(없어졌으면) 호가창에서 제거
        if (buy_order->quantity == 0) remove_buy_order(coin_id, i);
        else i++;    // 아직 남은 수량이 있으면 다음 매수 주문으로 넘어가기
    }

    // 2. 남은 수량이 있으면 SELL 주문으로 호가창에 등록시킴 (코인 예약)
    if (remain > 0) {
        sell_user->holdings[coin_id] -= remain; // 예약된 코인 차감

        int order_id = add_sell_order(user_id, coin_id, price, remain);
        if (order_id < 0) {
            sell_user->holdings[coin_id] += remain; // 주문 등록 실패하면 예약된 코인 돌려줌
            pthread_mutex_unlock(&orderbook_mutex);
            snprintf(buf, size, "ERR ORDERBOOK_FULL\n");
            return;
        }

        snprintf(buf, size, "§ SELL filled=%d remain=%d order_id=%d income=%d current=%d §\n",
            total_filled, remain, order_id, total_income, coin->current_price);
        
    }
    else {
        snprintf(buf, size, "§ SELL filled=%d income=%d price_change=%d->%d\n §", 
            total_filled, total_income, old_price, coin->current_price);
    }

    pthread_mutex_unlock(&orderbook_mutex);

}




// DOM <COIN> 명령어 처리 메시지 
void orderbook_msg(int coin_id, char *buf, int size) {
    if (buf == NULL || size <= 0) return;

    Coin *coin = get_coin(coin_id);
    if (coin == NULL) {
        snprintf(buf, size, "ERR UNKNOWN_COIN\n");
        return;
    }

    pthread_mutex_lock(&orderbook_mutex);

    OrderBook *book = &orderbooks[coin_id];

    int offset = 0;
    offset += snprintf(buf + offset, size - offset, "[%s ORDERBOOK]\n", coin->symbol);
    offset += snprintf(buf + offset, size - offset, "Current: %d\n\n", coin->current_price);


    // SELL 주문 출력
    offset += snprintf(buf + offset, size - offset, "SELL\n");

    if (book->sell_count == 0) 
        offset += snprintf(buf + offset, size - offset, "__EMPTY__\n");
    else {
        offset += snprintf(buf + offset, size - offset, "%-10s %-10s %-10s\n", "PRICE", "QTY", "UID");
        
        for (int i = 0; i < book->sell_count; i++) {
            Order *order = &book->sell_orders[i];

            User *user = get_user(order->user_id);
            int uid = -1;
            if (user != NULL) 
                uid = user->id;

            offset += snprintf(buf + offset, size - offset, "%-10d %-10d U%-9d\n", 
                order->price, order->quantity, uid);

            if (offset >= size) break;        
        }
    }


    // BUY 주문 출력
    offset += snprintf(buf + offset, size - offset, "\nBUY\n");

    if (book->buy_count == 0) 
        offset += snprintf(buf + offset, size - offset, "__EMPTY__\n");
    else {
        offset += snprintf(buf + offset, size - offset, "%-10s %-10s %-10s\n", "PRICE", "QTY", "UID");

        for (int i = 0; i < book->buy_count; i++) {
            Order *order = &book->buy_orders[i];

            User *user = get_user(order->user_id);
            int uid = -1;
            if (user != NULL) 
                uid = user->id;


            offset += snprintf(buf + offset, size - offset, "%-10d %-10d U%-9d\n", 
                order->price, order->quantity, uid);

            if (offset >= size) break;      
        }
    }

    pthread_mutex_unlock(&orderbook_mutex);
}



// ORDERS 명령어 처리 메시지
void my_orders_msg(int user_id, char *buf, int size) {
    if (buf == NULL || size <= 0) return;

    pthread_mutex_lock(&orderbook_mutex);

    int offset = 0;
    int found = 0;  // 내 주문이 하나라도 있는지 여부

    offset += snprintf(buf + offset, size - offset, "[MY ORDERS]\n");
    offset += snprintf(buf + offset, size - offset, "%-10s %-6s %-8s %-10s %-10s\n", "ORDER_ID", "SIDE", "COIN", "PRICE", "QTY");

    for (int i = 0; i < get_coin_count(); i++) {
        Coin *coin = get_coin(i);
        if (coin == NULL) continue;

        OrderBook *book = &orderbooks[i];

        // BUY 주문 조회
        for (int j = 0; j < book->buy_count; j++) {
            Order *order = &book->buy_orders[j];

            // 내 주문이면 출력
            if (order->user_id == user_id) {
                found = 1;  // 내 주문이 하나라도 있으면 found = 1

                offset += snprintf(buf + offset, size - offset, "%-10d %-6s %-8s %-10d %-10d\n",
                    order->order_id, "BUY", coin->symbol, order->price, order->quantity);
                
                // 출력 버퍼 크기 초과 방지
                if (offset >= size) {     
                    pthread_mutex_unlock(&orderbook_mutex); 
                    return;
                }
            }
        }


        // SELL 주문 조회
        for (int j = 0; j < book->sell_count; j++) {
            Order *order = &book->sell_orders[j];

            // 내 주문이면 출력
            if (order->user_id == user_id) {
                found = 1;  // 내 주문이 하나라도 있으면 found = 1

                offset += snprintf(buf + offset, size - offset, "%-10d %-6s %-8s %-10d %-10d\n",
                    order->order_id, "SELL", coin->symbol, order->price, order->quantity);
                
                // 출력 버퍼 크기 초과 방지
                if (offset >= size) {     
                    pthread_mutex_unlock(&orderbook_mutex); 
                    return;
                }
            }
        
        }


        // 못 찾음
        if (!found) {
            snprintf(buf, size, "[MY ORDERS]\n__EMPTY__\n");
        }
    }

    pthread_mutex_unlock(&orderbook_mutex);

}