#ifndef CLIENT_MGR_H
#define CLIENT_MGR_H
#include "types.h"


void init_client();

int create_session(int sock_fd);

CliSession *get_session(int session_id);

void remove_session(int session_id);

int find_user(const char *name);

int register_user(const char *name, const char *pw);

int login(int session_id, const char *name, const char *pw);

void logout(int session_id);

int islogin(int session_id);

int get_session_userid(int session_id);

User *get_user(int user_id);

int get_userCount();

User *get_users();

#endif