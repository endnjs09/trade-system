#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../../include/client_manager.h"
#include "../../include/types.h"


static User users[MAX_USERS];
static int user_count = 0;

static CliSession sessions[MAX_CLIENTS];
static int session_count = 0;

static pthread_mutex_t client_mgr_lock = PTHREAD_MUTEX_INITIALIZER;


// User 배열에 새 사용자를 추가
static int create_user_internal(const char *name, const char *pw) {
    if (user_count >= MAX_USERS) {
        return -1;
    }

    int id = user_count++;

    users[id].id = id;

    // 사용자 정보 등록
    strncpy(users[id].name, name, NAME_LEN - 1);
    users[id].name[NAME_LEN - 1] = '\0';
    strncpy(users[id].password, pw, PASSWORD_LEN - 1);
    users[id].password[PASSWORD_LEN - 1] = '\0';
    users[id].cash = INITIAL_CASH;
    users[id].initial_asset = INITIAL_CASH;

    for (int i = 0; i < MAX_COINS; i++) {
        users[id].holdings[i] = 0;
    }

    users[id].socket_fd = -1;
    users[id].connected = 0;
    users[id].logged_in = 0;

    return id;
}




// 사용자, 세션 초기화
void init_client() {
    pthread_mutex_lock(&client_mgr_lock);

    user_count = 0;
    session_count = 0;

    for (int i = 0; i < MAX_USERS; i++) {
        users[i].id = -1;
        users[i].name[0] = '\0';
        users[i].password[0] = '\0';
        users[i].cash = 0;
        users[i].initial_asset = 0;

        for (int j = 0; j < MAX_COINS; j++) {
            users[i].holdings[j] = 0;
        }

        users[i].socket_fd = -1;
        users[i].connected = 0;
        users[i].logged_in = 0;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        sessions[i].socket_fd = -1;
        sessions[i].user_id = -1;
        sessions[i].auth_state = AUTH_NONE;
    }

    pthread_mutex_unlock(&client_mgr_lock);
}




// 새로운 클라이언트 세션 생성(소켓 fd 받은걸로) 
int create_session(int sock_fd) {
    pthread_mutex_lock(&client_mgr_lock);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].socket_fd == -1) {  // 비어있는 자리 찾음
            sessions[i].socket_fd = sock_fd;
            sessions[i].user_id = -1;
            sessions[i].auth_state = AUTH_NONE;

            pthread_mutex_unlock(&client_mgr_lock);
            return i;
        }
    }

    pthread_mutex_unlock(&client_mgr_lock);
    return -1;
}



// 세션 정보 조회
CliSession *get_session(int session_id) {
    if (session_id < 0 || session_id >= MAX_CLIENTS) {
        return NULL;
    }

    return &sessions[session_id];
}



// 클라이언트 세션 제거
void remove_session(int session_id) {
    pthread_mutex_lock(&client_mgr_lock);

    if (session_id < 0 || session_id >= MAX_CLIENTS) {
        pthread_mutex_unlock(&client_mgr_lock);
        return;
    }

    int user_id = sessions[session_id].user_id;

    if (user_id >= 0 && user_id < user_count) {
        users[user_id].socket_fd = -1;
        users[user_id].connected = 0;
        users[user_id].logged_in = 0;
    }

    sessions[session_id].socket_fd = -1;
    sessions[session_id].user_id = -1;
    sessions[session_id].auth_state = AUTH_NONE;

    pthread_mutex_unlock(&client_mgr_lock);
}



// 사용자 이름으로 user_id 찾기
int find_user(const char *name) {
    if (name == NULL) {
        return -1;
    }

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}




// 회원가입
int register_user(const char *name, const char *pw) {
    pthread_mutex_lock(&client_mgr_lock);

    if (name == NULL || pw == NULL) {
        pthread_mutex_unlock(&client_mgr_lock);
        return -1;
    }

    if (strlen(name) == 0 || strlen(pw) == 0) {
        pthread_mutex_unlock(&client_mgr_lock);
        return -1;
    }

    if (find_user(name) != -1) {
        pthread_mutex_unlock(&client_mgr_lock);
        return -1;
    }

    // 
    int user_id = create_user_internal(name, pw);

    pthread_mutex_unlock(&client_mgr_lock);

    return user_id;
}




// 로그인
int login(int session_id, const char *name, const char *pw) {
    pthread_mutex_lock(&client_mgr_lock);

    if (session_id < 0 || session_id >= MAX_CLIENTS) {
        pthread_mutex_unlock(&client_mgr_lock);
        return -1;
    }

    if (sessions[session_id].auth_state == AUTH_LOGGED_IN) {
        pthread_mutex_unlock(&client_mgr_lock);
        return -1;
    }

    int user_id = find_user(name);

    // user_id 존재 x (회원가입 안함)
    if (user_id == -1) {
        pthread_mutex_unlock(&client_mgr_lock);
        return -1;
    }
    // 비밀번호 불일치
    if (strcmp(users[user_id].password, pw) != 0) {
        pthread_mutex_unlock(&client_mgr_lock);
        return -1;
    }
    // 이미 로그인 했음
    if (users[user_id].logged_in) {
        pthread_mutex_unlock(&client_mgr_lock);
        return -1;
    }

    sessions[session_id].user_id = user_id;
    sessions[session_id].auth_state = AUTH_LOGGED_IN;

    users[user_id].socket_fd = sessions[session_id].socket_fd;
    users[user_id].connected = 1;
    users[user_id].logged_in = 1;

    pthread_mutex_unlock(&client_mgr_lock);

    return user_id;
}



// 로그아웃
void logout(int session_id) {
    pthread_mutex_lock(&client_mgr_lock);

    if (session_id < 0 || session_id >= MAX_CLIENTS) {
        pthread_mutex_unlock(&client_mgr_lock);
        return;
    }

    int user_id = sessions[session_id].user_id;

    if (user_id >= 0 && user_id < user_count) {
        users[user_id].socket_fd = -1;
        users[user_id].connected = 0;
        users[user_id].logged_in = 0;
    }

    sessions[session_id].user_id = -1;
    sessions[session_id].auth_state = AUTH_NONE;

    pthread_mutex_unlock(&client_mgr_lock);
}




// 로그인 여부 확인
int islogin(int session_id) {
    if (session_id < 0 || session_id >= MAX_CLIENTS) {
        return 0;
    }

    if (sessions[session_id].socket_fd == -1) {
        return 0;
    }

    return sessions[session_id].auth_state == AUTH_LOGGED_IN;
}




// 세션에 연결된 user_id 조회
int get_session_userid(int session_id) {
    if (session_id < 0 || session_id >= MAX_CLIENTS) {
        return -1;
    }

    return sessions[session_id].user_id;
}




// user_id로 사용자 정보 조회
User *get_user(int user_id) {
    if (user_id < 0 || user_id >= user_count) {
        return NULL;
    }

    return &users[user_id];
}




// 사용자 수 조회
int get_userCount() {
    return user_count;
}




// 전체 사용자 배열 조회
User *get_users() {
    return users;
}