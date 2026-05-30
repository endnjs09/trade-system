# Trade System

C Socket 기반 실시간 가상 코인 거래 시스템입니다.


## 프로젝트 목표

본 프로젝트는 다중 클라이언트가 접속할 수 있는 가상 코인 거래 서버를 구현하는 것을 목표로 합니다.

최종적으로는 사용자가 서버에 접속하여 회원가입/로그인 후, 가상 코인을 조회하고 매수/매도 주문을 등록하며, 서버가 호가창 기반으로 주문을 체결하는 구조를 구현할 예정입니다.


## 프로젝트 구조

```text
trade_sys/
├── Makefile
├── README.md
├── include/			
│   ├── client_manager.h		
│   ├── handler.h		
│   ├── market.h			
│   ├── multicast.h				
│   ├── orderbook.h		
│   ├── protocol.h				
│   └── types.h				
├── src/
│   ├── server/	
│   │   ├── client_manager.c		
│   │   ├── handler.c		
│   │   ├── market.c			
│   │   ├── multicast.c			
│   │   ├── orderbook.c			
│   │   └── server.c				
│   └── client/
│       └── client.c
├── data/
└── logs/

```


## 개발 예정 기능
### Phase 2
- 코인 초기 데이터 설정
- MARKET 명령어
- MY 명령어
### Phase 3
- 서버 초기 유동성 기반 BUY 구현
- 사용자 현금 감소
- 사용자 코인 보유량 증가
- 현재가 갱신
### Phase 4
- SELL 주문 등록
- DOM 호가창 조회
### Phase 5
- 사용자 간 BUY/SELL 주문 체결
- 가격 우선 / 시간 우선 매칭
### Phase 6
- BUY 주문장 구현
- 미체결 주문 관리
### Phase 7
-nRANK 랭킹 기능
### Phase 8
- 가격 변동 이벤트 브로드캐스트
-vUDP Multicast 기반 시세 전파
### Phase 9
- 거래 로그 저장
