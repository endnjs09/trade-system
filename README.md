# Trade System

TCP Socket과 UDP Multicast를 이용한 가상 코인 거래 시스템입니다.

네트워크 프로그래밍 기말 프로젝트로 제작했으며, 여러 클라이언트가 서버에 접속해 가상 코인을 매수/매도하고, 호가창 기반으로 주문을 체결하는 구조를 구현했습니다.

## 주요 기능

- TCP Socket 기반 클라이언트-서버 통신
- pthread 기반 다중 클라이언트 처리
- 회원가입 / 로그인 / 로그아웃
- 시장 가격 조회
- 사용자 자산 조회
- 호가창 조회
- BUY / SELL 주문
- 사용자 간 주문 체결
- 서버 유동성 기반 초기 거래
- 미체결 주문 조회
- 주문 취소
- 총자산 기준 랭킹
- UDP Multicast 기반 가격 변동 이벤트 전파

## 사용 기술

- C
- TCP Socket
- UDP Multicast
- pthread
- mutex
- Makefile
- Linux / WSL

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
├── data/    # 사용자/시장 데이터 저장 확장용
└── logs/    # 거래 로그 저장 확장용
  
```

## 실행 방법

### 빌드

```bash
make
```

### 서버 실행

```bash
./server
```

### 클라이언트 실행

다른 터미널에서 실행합니다.

```bash
./client
```

서버 IP를 직접 지정할 수도 있습니다.

```bash
./client 127.0.0.1
```

## 지원 명령어

```text
REGISTER <NAME> <PW>        회원가입
LOGIN <NAME> <PW>           로그인
LOGOUT                      로그아웃
MARKET                      시장 가격 조회
MY                          내 자산 조회
DOM <COIN>                  호가창 조회
BUY <COIN> <PRICE> <QTY>    매수 주문
SELL <COIN> <PRICE> <QTY>   매도 주문
ORDERS                      내 미체결 주문 조회
CANCEL <ORDER_ID>           미체결 주문 취소
RANK                        총자산 랭킹 조회
HELP                        명령어 도움말
QUIT                        클라이언트 종료
```

## 통신 구조

* TCP Socket
  클라이언트의 명령어 요청과 서버 응답 처리에 사용합니다.

* UDP Multicast
  거래 체결이나 가격 변동이 발생했을 때 여러 클라이언트에게 이벤트를 전파하는 데 사용합니다.

## 거래 규칙

매수 가격이 매도 가격보다 크거나 같으면 거래가 체결됩니다.

```text
BUY 가격 >= SELL 가격
```

사용자 간 거래가 체결되면 현재가는 마지막 체결가로 갱신됩니다.
서버 유동성에서 구매하는 경우에는 구매 수량에 비례해 현재가가 상승합니다.

## 시연 예시

```text
REGISTER A 1234
LOGIN A 1234
MARKET
BUY XXX 10000 10
SELL XXX 10500 5
ORDERS
RANK
```

여러 터미널에서 클라이언트를 실행하면 다중 사용자 거래와 멀티캐스트 가격 변동 이벤트를 확인할 수 있습니다.
