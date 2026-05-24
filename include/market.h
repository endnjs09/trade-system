#ifndef MARKET_H
#define MARKET_H

#include "types.h"

// 시장 초기화
void init_market();

// 코인 심볼로 coin_id 찾기
int find_coin(const char *symbol);

// coin_id로 Coin 정보 조회
Coin *get_coin(int coin_id);

// 현재 등록된 코인 수
int get_coin_count();

// 전체 코인 배열 반환
Coin *get_coins();

// MARKET 응답 문자열
void market_msg(char *buf, int size);

// MY 응답 문자열
void my_msg(User *user,char *buf, int size);

// rank 응답 문자열
void rank_msg(char *buf, int size);

#endif
