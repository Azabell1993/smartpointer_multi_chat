#pragma ONCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "smartptr.h"

#define MAX_STRING_SIZE 100
#define MAX_USERS 10
#define USER_DATA_FILE "user_data.txt"

typedef struct {
    SmartPtr host;
    SmartPtr user;
    SmartPtr pass;
    SmartPtr name;
} UserInfo;

typedef struct {
    UserInfo users[MAX_USERS];
    size_t user_count;
    pthread_mutex_t db_mutex;
} UserDB;

void display_all_users(UserDB *db);
int query_user(UserDB *db, const char *username, const char *password);
bool register_user(UserDB *db, const char *host, const char *user, const char *pass, const char *name);

// 유저 데이터베이스 초기화
void init_user_db(UserDB *db); 
// 유저 로그인 확인
bool is_user_logined_in(const char *username);
// 유저 로그인 처리
void log_user_in(const char *username);
// 로그아웃 및 숨김 파일 삭제
void logout_user(const char *username);
// 유저 삭제
bool delete_user(UserDB *db, const char *username);
int write_txt_file(UserDB *db);

// 사용자 정보 파일 관리 함수
void save_user_to_file(const char *host, const char *user, const char *pass, const char *name);
void load_users_from_file(UserDB *db);

void save_user_to_file(const char *host, const char *user, const char *pass, const char *name) {
    printf("Saving user: %s, %s, %s, %s\n", host, user, pass, name);

    FILE *file = fopen(USER_DATA_FILE, "a");
    if (file == NULL) {
        printf("Error: Could not open user data file.\n");
        return;
    }

    fprintf(file, "%s %s %s %s\n", host, user, pass, name);
    fclose(file);
    printf("User data saved successfully.\n");
}

void load_users_from_file(UserDB *db) {
    FILE *file = fopen(USER_DATA_FILE, "r");
    if (file == NULL) {
        printf("Error: Could not open user data file.\n");
        return;
    }

    char host[MAX_STRING_SIZE], user[MAX_STRING_SIZE], pass[MAX_STRING_SIZE], name[MAX_STRING_SIZE];
    while (fscanf(file, "%s %s %s %s", host, user, pass, name) != EOF) {
        UserInfo new_user;
        new_user.host = CREATE_SMART_PTR(char[MAX_STRING_SIZE], host);
        new_user.user = CREATE_SMART_PTR(char[MAX_STRING_SIZE], user);
        new_user.pass = CREATE_SMART_PTR(char[MAX_STRING_SIZE], pass);
        new_user.name = CREATE_SMART_PTR(char[MAX_STRING_SIZE], name);
        db->users[db->user_count++] = new_user;
    }

    fclose(file);
    printf("User data loaded successfully.\n");
}

int write_txt_file(UserDB *db) {
    FILE *file = fopen(USER_DATA_FILE, "w");
    if (file == NULL) {
        printf("Error: Could not open user data file.\n");
        return 1;
    }

    for (size_t i = 0; i < db->user_count; ++i) {
        fprintf(file, "%s %s %s %s\n",
            (char *)db->users[i].host.ptr,
            (char *)db->users[i].user.ptr,
            (char *)db->users[i].pass.ptr,
            (char *)db->users[i].name.ptr);
    }

    fclose(file);
    printf("User data saved successfully.\n");
    return 0;
}

// 유저 데이터베이스 초기화
void init_user_db(UserDB *db) {
    db->user_count = 0;
    pthread_mutex_init(&db->db_mutex, NULL);
}

// 유저 로그인 확인
bool is_user_logined_in(const char *username) {
    // .username 파일이 존재하는지 확인
    char filename[MAX_STRING_SIZE + 1] = ".";
    strncat(filename, username, MAX_STRING_SIZE);

    // 파일이 존재하면 true 반환
    if (access(filename, F_OK) == 0) {
        return true;
    }
    return false;
}

// 유저 로그인 처리
void log_user_in(const char *username) {
    // .username 파일을 생성하여 로그인 상태 기록
    char filename[MAX_STRING_SIZE + 1] = ".";
    strncat(filename, username, MAX_STRING_SIZE);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error creating login file");
        return;
    }
    fclose(file);
}

// 로그아웃 및 숨김 파일 삭제
void logout_user(const char *username) {
    // .username 파일 삭제
    char filename[MAX_STRING_SIZE + 1] = ".";
    strncat(filename, username, MAX_STRING_SIZE);

    if (unlink(filename) == 0) {
        printf("%s 로그아웃 성공, 숨김 파일 삭제됨\n", username);
    } else {
        perror("Error deleting login file");
    }
}

// 유저 삭제
bool delete_user(UserDB *db, const char *username) {
    pthread_mutex_lock(&db->db_mutex);
    for (size_t i = 0; i < db->user_count; ++i) {
        if (strcmp((char *)db->users[i].user.ptr, username) == 0) {
            for (size_t j = i; j < db->user_count - 1; ++j) {
                db->users[j] = db->users[j + 1];
            }
            db->user_count--;
            pthread_mutex_unlock(&db->db_mutex);
            return true;
        }
    }
    pthread_mutex_unlock(&db->db_mutex);
    return false;
}

int query_user(UserDB *db, const char *username, const char *password) {
    pthread_mutex_lock(&db->db_mutex);
    for (size_t i = 0; i < db->user_count; ++i) {
        if (strcmp((char *)db->users[i].user.ptr, username) == 0 &&
            strcmp((char *)db->users[i].pass.ptr, password) == 0) {
            pthread_mutex_unlock(&db->db_mutex);
            return i;  // 로그인 성공
        }
    }
    pthread_mutex_unlock(&db->db_mutex);
    return -1;  // 로그인 실패
}

bool register_user(UserDB *db, const char *host, const char *user, const char *pass, const char *name) {
    pthread_mutex_lock(&db->db_mutex);

    if (db->user_count >= MAX_USERS) {
        printf("User database is full.\n");
        pthread_mutex_unlock(&db->db_mutex);
        return false;
    }

    UserInfo new_user;
    new_user.host = CREATE_SMART_PTR(char[MAX_STRING_SIZE], host);
    strcpy((char *)new_user.host.ptr, host);
    
    new_user.user = CREATE_SMART_PTR(char[MAX_STRING_SIZE], user);
    strcpy((char *)new_user.user.ptr, user);
    
    new_user.pass = CREATE_SMART_PTR(char[MAX_STRING_SIZE], pass);
    strcpy((char *)new_user.pass.ptr, pass);
    
    new_user.name = CREATE_SMART_PTR(char[MAX_STRING_SIZE], name);
    strcpy((char *)new_user.name.ptr, name);

    db->users[db->user_count++] = new_user;

    pthread_mutex_unlock(&db->db_mutex);
    return true;
}

void display_all_users(UserDB *db) {
    for (size_t i = 0; i < db->user_count; ++i) {
        printf("Host: %s, User: %s, Pass: %s, Name: %s\n",
            (char *)db->users[i].host.ptr,
            (char *)db->users[i].user.ptr,
            (char *)db->users[i].pass.ptr,
            (char *)db->users[i].name.ptr);
    }
}
