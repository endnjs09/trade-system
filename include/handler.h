#ifndef HANDLER_H
#define HANDLER_H

#define CMD_CONTINUE 0      // 클라이언트 연결 유지
#define CMD_CLOSE 1         // 클라이언트 연결 종료

// session_id: 현재 명령어를 보낸 클라이언트 세션 id
// input: 클라이언트가 보낸 명령어 
int handle_command(int session_id, char *input);

#endif