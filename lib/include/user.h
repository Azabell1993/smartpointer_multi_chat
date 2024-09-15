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

/**
 * @brief 유저 정보 구조체
 * 
 * 각각의 유저 정보를 포함하는 구조체입니다. 
 * 스마트 포인터를 통해 메모리 관리가 이루어집니다.
 */
typedef struct {
    SmartPtr host;
    SmartPtr user;
    SmartPtr pass;
    SmartPtr name;
} UserInfo;

/**
 * @brief 유저 데이터베이스 구조체
 * 
 * 최대 유저 수에 대한 정보를 저장하며, 데이터베이스 동기화를 위한 뮤텍스를 포함합니다.
 */
typedef struct {
    UserInfo users[MAX_USERS];
    size_t user_count;
    pthread_mutex_t db_mutex;
} UserDB;

/**
 * @brief 모든 유저 정보를 출력합니다.
 * 
 * @param db 유저 데이터베이스 포인터
 */
void display_all_users(UserDB *db);

/**
 * @brief 유저 조회 함수
 * 
 * 주어진 유저 이름과 비밀번호가 데이터베이스에 있는지 확인합니다.
 * 
 * @param db 유저 데이터베이스 포인터
 * @param username 유저 이름
 * @param password 비밀번호
 * @return 성공 시 유저 인덱스, 실패 시 -1 반환
 */
int query_user(UserDB *db, const char *username, const char *password);

/**
 * @brief 유저 등록 함수
 * 
 * 새로운 유저를 데이터베이스에 등록합니다.
 * 
 * @param db 유저 데이터베이스 포인터
 * @param host 호스트 이름
 * @param user 유저 이름
 * @param pass 비밀번호
 * @param name 유저의 실제 이름
 * @return 성공 시 true, 실패 시 false 반환
 */
bool register_user(UserDB *db, const char *host, const char *user, const char *pass, const char *name);

/**
 * @brief 유저 데이터베이스 초기화
 * 
 * 유저 데이터베이스를 초기화하고 뮤텍스를 설정합니다.
 * 
 * @param db 유저 데이터베이스 포인터
 */
void init_user_db(UserDB *db); 

/**
 * @brief 유저 로그인 상태 확인
 * 
 * 주어진 유저 이름이 로그인된 상태인지 확인합니다.
 * 
 * @param username 유저 이름
 * @return 로그인 시 true, 아니면 false 반환
 */
bool is_user_logined_in(const char *username);

/**
 * @brief 유저 로그인 처리
 * 
 * 주어진 유저 이름을 기반으로 로그인 상태를 기록하는 파일을 생성합니다.
 * 
 * @param username 유저 이름
 */
void log_user_in(const char *username);

/**
 * @brief 유저 로그아웃 처리 및 파일 삭제
 * 
 * 유저 로그아웃 시 생성된 로그인 파일을 삭제합니다.
 * 
 * @param username 유저 이름
 */
void logout_user(const char *username);

/**
 * @brief 유저 삭제 함수
 * 
 * 데이터베이스에서 유저 정보를 삭제합니다.
 * 
 * @param db 유저 데이터베이스 포인터
 * @param username 유저 이름
 * @return 성공 시 true, 실패 시 false 반환
 */
bool delete_user(UserDB *db, const char *username);

/**
 * @brief 유저 정보를 파일에 저장
 * 
 * 주어진 유저 정보를 텍스트 파일에 저장합니다.
 * 
 * @param host 호스트 이름
 * @param user 유저 이름
 * @param pass 비밀번호
 * @param name 유저 이름
 */
void save_user_to_file(const char *host, const char *user, const char *pass, const char *name);

/**
 * @brief 유저 정보를 파일에서 불러옴
 * 
 * 텍스트 파일에서 유저 정보를 읽어 데이터베이스에 저장합니다.
 * 
 * @param db 유저 데이터베이스 포인터
 */
void load_users_from_file(UserDB *db);

/**
 * @brief 유저 데이터를 텍스트 파일에 기록
 * 
 * 현재 데이터베이스에 있는 모든 유저 정보를 파일에 기록합니다.
 * 
 * @param db 유저 데이터베이스 포인터
 * @return 성공 시 0, 실패 시 1 반환
 */
int write_txt_file(UserDB *db);

/**
 * @brief 모든 유저 정보를 출력
 * 
 * 유저 데이터베이스에 있는 모든 유저 정보를 출력합니다.
 * 
 * @param db 유저 데이터베이스 포인터
 */
void display_all_users(UserDB *db);


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
