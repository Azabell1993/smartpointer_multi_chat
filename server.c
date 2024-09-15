/**
 * @file server.c
 * @brief 서버 코드. 클라이언트와의 채팅 통신을 관리하고, 스마트 포인터를 이용해 클라이언트 정보를 처리합니다.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include "lib/include/uniqueptr.h"
#include <fcntl.h>
#include <pthread.h>

#define DEFAULT_TCP_PORT 5100
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

/**
 * @brief 스마트 포인터 구조체
 * 
 * 동적 할당된 메모리를 안전하게 관리하며, 참조 카운트를 통해 메모리를 자동 해제합니다.
 */
typedef struct SmartPtr {
    void *ptr;                   /**< 포인터가 가리키는 실제 데이터 */
    int *ref_count;              /**< 참조 카운트 */
    pthread_mutex_t *mutex;      /**< 멀티스레드 환경에서 참조 카운트를 보호하는 뮤텍스 */
} SmartPtr;

/**
 * @brief 스마트 포인터를 생성하는 함수
 * 
 * 동적 메모리를 할당받고 참조 카운트를 초기화하며 뮤텍스를 생성합니다.
 * 
 * @param ptr 스마트 포인터가 관리할 동적 메모리의 포인터
 * @return SmartPtr 새롭게 생성된 스마트 포인터 구조체
 */
SmartPtr create_smart_ptr(void *ptr);

/**
 * @brief 스마트 포인터의 참조를 증가시키는 함수
 * 
 * @param sp 참조를 증가시킬 스마트 포인터의 포인터
 */
void retain(SmartPtr *sp);

/**
 * @brief 스마트 포인터의 참조를 해제하고, 마지막 참조 해제 시 메모리를 반환하는 함수
 * 
 * @param sp 참조를 해제할 스마트 포인터의 포인터
 */
void release(SmartPtr *sp);

/**
 * @brief 고유 포인터 구조체
 * 
 * 참조 카운트가 1인 스마트 포인터로, 다른 스마트 포인터와 공유되지 않습니다.
 */
void list_users();

/**
 * @brief 클라이언트를 강제로 퇴장시키는 함수
 * 
 * @param username 퇴장시킬 클라이언트의 사용자명
 */
void kill_user(const char *username);

/**
 * @brief 클라이언트를 강제로 퇴장시키는 함수
 * 
 * @param username 퇴장시킬 클라이언트의 사용자명
 */
void kill_room(int room_id);

/**
 * @brief 클라이언트 정보를 스마트 포인터로 관리하는 배열
 * 
 * @note 최대 MAX_CLIENTS 개의 클라이언트를 관리합니다.
 */
void kick_user(const char *username);

/**
 * @brief 클라이언트 정보를 담는 구조체
 * 
 * 각 클라이언트의 소켓 FD, ID, 채팅방 ID, 사용자명을 포함하고 있으며, 뮤텍스를 관리합니다.
 */
typedef struct {
    int client_fd;               /**< 클라이언트의 소켓 파일 디스크립터 */
    int client_id;               /**< 클라이언트 ID */
    int room_id;                 /**< 클라이언트가 참여한 채팅방 ID */
    char username[BUFFER_SIZE];  /**< 클라이언트 사용자명 */
    pthread_mutex_t *client_mutex; /**< 클라이언트 별 뮤텍스 */
} ClientInfo;

/**
 * @brief 클라이언트 정보를 스마트 포인터로 관리하는 배열
 * 
 * @note 최대 MAX_CLIENTS 개의 클라이언트를 관리합니다.
 */
SmartPtr client_infos[MAX_CLIENTS];

/**
 * @brief 클라이언트 정보를 스마트 포인터로 관리하는 배열
 * @param client_infos 클라이언트 정보를 담는 스마트 포인터 배열
 * @return void
 */
UniquePtr create_unique_ptr(size_t size, void (*deleter)(void*));

/**
 * @brief TCP 서버를 생성하고 클라이언트 연결을 처리하는 함수
 * 
 * @param num_tcp_proc 생성할 TCP 프로세스 수
 * @param ... 서버의 IP 주소와 포트를 인자로 받습니다.
 * @return int 성공 시 0, 실패 시 -1 반환
 */
int create_network_tcp_process(int num_tcp_proc, ...);

/**
 * @brief 특정 채팅방에 있는 모든 클라이언트에게 메시지를 브로드캐스트하는 함수
 * 
 * @param sender_fd 메시지를 보낸 클라이언트의 파일 디스크립터
 * @param message 브로드캐스트할 메시지
 * @param room_id 메시지를 보낼 채팅방의 ID
 */
void broadcast_message(int sender_fd, char *message, int room_id);

/**
 * @brief 서버 측에서 발생한 채팅 메시지를 로그로 저장하는 함수
 * 
 * @param message 저장할 메시지
 */
void log_chat_message(const char *message);

/**
 * @brief 클라이언트와의 통신을 처리하는 스레드 함수
 * 
 * @param arg 클라이언트 정보를 담고 있는 스마트 포인터 구조체의 포인터
 * @return void* 스레드 종료 시 반환값 (NULL)
 */
void *client_handler(void *arg);

/**
 * @brief 클라이언트 정보를 스마트 포인터로 관리하는 배열
 * 
 * @param client_infos 클라이언트 정보를 담는 스마트 포인터 배열
 * @return void
 */
void release_client(int sock);

/**
 * @brief 서버 측에서 사용자 입력을 처리하는 스레드 함수
 * 
 * 표준 입력으로부터 메시지를 받아 서버 메시지로 브로드캐스트합니다.
 * 
 * @param arg 미사용
 * @return void* 스레드 종료 시 반환값 (NULL) */
void *server_input_handler(void *arg);

/**
 * @brief 서버 메시지를 브로드캐스트하는 함수
 * 
 * @param message 서버에서 브로드캐스트할 메시지
 * @return void
 */
void send_server_message(char *message);

/**
 * @brief 클라이언트를 강제로 퇴장시키는 함수
 * 
 * @param username 퇴장시킬 클라이언트의 사용자명
 * @return void
 */
void kill_user(const char *username);

/**
 * @brief 서버 관리자용 고정 메뉴 출력 함수
 */
void print_fixed_menu() {
    // ANSI 색상 코드
    const char *color_reset = "\033[0m";
    const char *color_red = "\033[1;31m";
    const char *color_green = "\033[1;32m";
    const char *color_blue = "\033[1;34m";
    const char *color_cyan = "\033[1;36m";

    // 메뉴 출력
    printf("\n%s================= 서버 관리자 메뉴 =================%s\n", color_blue, color_reset);
    printf("%s|%s 1. %s'list'%s : 현재 접속한 유저와 채팅방 상태 출력     %s|%s\n", color_cyan, color_reset, color_green, color_reset, color_cyan, color_reset);
    printf("%s|%s 2. %s'kill <user> (구현 예정)'%s : 특정 유저 강제 퇴장 (구현 예정)              %s|%s\n", color_cyan, color_reset, color_green, color_reset, color_cyan, color_reset);
    printf("%s|%s 3. %s'kill room <num> (구현 예정)'%s : 특정 채팅방 강제 종료 (구현 예정)          %s|%s\n", color_cyan, color_reset, color_green, color_reset, color_cyan, color_reset);
    printf("%s|%s 4. %s'grep -r \"<message>\"'%s : 채팅 로그에서 메시지 검색  %s|%s\n", color_cyan, color_reset, color_green, color_reset, color_cyan, color_reset);
    printf("%s|%s 5. %s'exit'%s : 서버 종료                                 %s|%s\n", color_cyan, color_reset, color_green, color_reset, color_cyan, color_reset);
    printf("%s=====================================================%s\n\n", color_blue, color_reset);
}

/**
 * @brief 서버 메시지를 브로드캐스트하는 함수
 * @param message 서버에서 브로드캐스트할 메시지
 * @param argument message 서버에서 브로드캐스트할 메시지
 * @return void
 */
SmartPtr create_smart_ptr(void *ptr) {
    SmartPtr sp;
    sp.ptr = ptr;
    sp.ref_count = (int *)malloc(sizeof(int));
    if (sp.ref_count == NULL) {
        perror("Failed to allocate memory for ref_count");
        exit(EXIT_FAILURE);
    }
    *(sp.ref_count) = 1;
    sp.mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (sp.mutex == NULL) {
        perror("Failed to allocate memory for mutex");
        free(sp.ref_count);
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(sp.mutex, NULL);
    return sp;
}

/**
 * @brief 스마트 포인터 retain 함수 구현
 * @param sp 참조를 증가시킬 스마트 포인터의 포인터
 * @return void
 */
void retain(SmartPtr *sp) {
    pthread_mutex_lock(sp->mutex);
    (*(sp->ref_count))++;
    pthread_mutex_unlock(sp->mutex);
}

/**
 * @brief 스마트 포인터 release 함수 구현
 * @param sp 해제할 스마트 포인터의 포인터
 * @return void
 */
void release(SmartPtr *sp) {
    int should_free = 0;
    pthread_mutex_lock(sp->mutex);
    (*(sp->ref_count))--;
    if (*(sp->ref_count) == 0) {
        should_free = 1;
    }
    pthread_mutex_unlock(sp->mutex);

    if (should_free) {
        free(sp->ptr);
        free(sp->ref_count);
        pthread_mutex_destroy(sp->mutex);
        free(sp->mutex);
    }
}

/**
 * @brief 클라이언트를 강제로 퇴장시키는 함수
 * @param username 퇴장시킬 클라이언트의 사용자명
 * @return void
 */
void list_users() {
    printf("현재 접속 중인 유저 목록:\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_infos[i].ptr != NULL) {
            ClientInfo *client_info = (ClientInfo *)client_infos[i].ptr;
            printf("User: %s, Room: %d\n", client_info->username, client_info->room_id);
        }
    }

    for (int room_id = 1; room_id <= 5; room_id++) {
        int user_count = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_infos[i].ptr != NULL) {
                ClientInfo *client_info = (ClientInfo *)client_infos[i].ptr;
                if (client_info->room_id == room_id) {
                    user_count++;
                }
            }
        }
        printf("Room %d: %d명\n", room_id, user_count);
    }
}

/**
 * @brief 클라이언트를 강제로 퇴장시키는 함수
 * @param username 퇴장시킬 클라이언트의 사용자명
 * @return void
 */
void kill_room(int room_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_infos[i].ptr != NULL) {
            ClientInfo *client_info = (ClientInfo *)client_infos[i].ptr;
            if (client_info->room_id == room_id) {
                send(client_info->client_fd, "The room has been closed. You have been kicked out.\n", strlen("The room has been closed. You have been kicked out.\n"), 0);
                release_client(i);  // Properly release client
            }
        }
    }
    printf("Room %d has been closed, and all users have been kicked.\n", room_id);
}

/**
 * @brief 서버 메시지를 브로드캐스트하는 함수
 * @param message 서버에서 브로드캐스트할 메시지
 * @param argument message 서버에서 브로드캐스트할 메시지
 * @return void
 */
SmartPtr client_infos[MAX_CLIENTS];

/**
 * @brief 클라이언트 정보를 스마트 포인터로 관리하는 배열
 * @param client_infos 클라이언트 정보를 담는 스마트 포인터 배열
 * @return void
 */
void add_new_client(int sock, int client_id, const char *username) {
    ClientInfo *client_info = (ClientInfo *)malloc(sizeof(ClientInfo));
    client_info->client_fd = sock;
    client_info->client_id = client_id;
    strcpy(client_info->username, username); 
    pthread_mutex_t *client_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(client_mutex, NULL);
    client_info->client_mutex = client_mutex;
    
    client_infos[sock] = create_smart_ptr(client_info);
}

/**
 * @brief 클라이언트 정보를 스마트 포인터로 관리하는 배열
 * @param client_infos 클라이언트 정보를 담는 스마트 포인터 배열
 * @return void
 */
void release_client(int sock) {
    if (client_infos[sock].ptr != NULL) {
        ClientInfo *client_info = (ClientInfo *)client_infos[sock].ptr;
        close(client_info->client_fd);
        release_shared_ptr((SharedPtr*)&client_infos[sock]);
        printf("클라이언트 %d 연결 종료 및 메모리 해제 완료\n", client_info->client_id);
    }
}


/**
 * @brief 클라이언트 정보를 스마트 포인터로 관리하는 배열
 * @param client_infos 클라이언트 정보를 담는 스마트 포인터 배열
 * @return void
 */
void kill_user(const char *username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_infos[i].ptr != NULL) {
            ClientInfo *client_info = (ClientInfo *)client_infos[i].ptr;
            if (strcmp(client_info->username, username) == 0) {
                send(client_info->client_fd, "You have been kicked from the chat.\n", strlen("You have been kicked from the chat.\n"), 0);
                release_client(i);  // Properly release client
                printf("User %s has been kicked.\n", username);
                break;
            }
        }
    }
}

/**
 * @brief 특정 채팅방에 있는 모든 클라이언트에게 메시지를 브로드캐스트하는 함수
 * @param sender_fd 메시지를 보낸 클라이언트의 파일 디스크립터
 * @param message 브로드캐스트할 메시지
 * @param room_id 메시지를 보낼 채팅방의 ID
 * @return void
 */
void broadcast_message(int sender_fd, char *message, int room_id) {
    char broadcast_message[BUFFER_SIZE + 50];
    ClientInfo *sender_info = (ClientInfo *)client_infos[sender_fd].ptr;

    snprintf(broadcast_message, sizeof(broadcast_message), "[%s]: %s", sender_info->username, message);
    log_chat_message(broadcast_message);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_infos[i].ptr != NULL) {
            ClientInfo *client_info = (ClientInfo *)client_infos[i].ptr;
            if (client_info->room_id == room_id && client_info->client_fd != sender_fd) {
                write(client_info->client_fd, broadcast_message, strlen(broadcast_message));
            }
        }
    }
}


#include <time.h>

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief 채팅 메시지를 로그 파일에 저장하는 함수
 * @param message 저장할 메시지
 * @return void
 */
void log_chat_message(const char *message) {
    // 절대 경로로 로그 파일 지정
    char log_path[BUFFER_SIZE];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // 현재 날짜를 기반으로 로그 파일명을 만듦
    // 반드시 touch 명령어로 해당 파일을 미리 생성해두어야 함
    // 권한에 대해서도 고려해야 함
    // sudo touch /var/log/chatlog_20240915.log
    // sudo chmod 777 /var/log/chatlog_20240915.log
    strftime(log_path, sizeof(log_path), "/var/log/chatlog_%Y%m%d.log", t);

    // 뮤텍스 잠금으로 동시 접근 제어
    pthread_mutex_lock(&log_mutex);

    FILE *log_file = fopen(log_path, "a");
    if (log_file == NULL) {
        perror("로그 파일을 열 수 없습니다.");
        pthread_mutex_unlock(&log_mutex);  // 잠금 해제
        return;
    }

    // 로그 파일에 메시지 기록
    fprintf(log_file, "%s\n", message);
    fclose(log_file);

    // 뮤텍스 잠금 해제
    pthread_mutex_unlock(&log_mutex);
}

/**
 * @brief 클라이언트와의 통신을 처리하는 스레드 함수
 * @param arg 클라이언트 정보를 담고 있는 스마트 포인터 구조체의 포인터
 * @return void* 스레드 종료 시 반환값 (NULL)
 */
void *client_handler(void *arg) {
    SmartPtr *sp = (SmartPtr *)arg;
    ClientInfo *client_info = (ClientInfo *)sp->ptr;
    char buffer[BUFFER_SIZE];
    int nbytes;

    // 사용자명 수신
    memset(buffer, 0, sizeof(buffer));
    nbytes = read(client_info->client_fd, buffer, BUFFER_SIZE);
    if (nbytes <= 0) {
        printf("사용자명 수신 실패 또는 클라이언트 연결 종료\n");
        close(client_info->client_fd);
        release(sp);
        return NULL;
    }
    strncpy(client_info->username, buffer, BUFFER_SIZE);
    printf("사용자명: %s\n", client_info->username);

    // 채팅방 선택 수신
    memset(buffer, 0, sizeof(buffer));
    nbytes = read(client_info->client_fd, buffer, BUFFER_SIZE);
    if (nbytes <= 0) {
        printf("채팅방 수신 실패 또는 클라이언트 연결 종료\n");
        close(client_info->client_fd);

        // 클라이언트 종료 시 뮤텍스 제거
        printf("클라이언트 %d 연결 종료에 따른 뮤텍스 파괴", client_info->client_id);
        pthread_mutex_destroy(client_info -> client_mutex);
        free(client_info -> client_mutex);
        printf("뮤텍스 파괴 완료. 클라이언트 아이디 : [ %d ] -> destroyed\n", client_info->client_id);

        if(client_info->room_id != 0) {
            printf("클라이언트 %d가 채팅방 %d에서 퇴장했습니다.\n", client_info->client_id, client_info->room_id);
        } else {
            printf("클라이언트 %d 연결 종료\n", client_info->client_id);
        }

        release(sp);
        return NULL;
    }
    
    client_info->room_id = atoi(buffer);
    printf("클라이언트 %d가 채팅방 %d에 입장했습니다.\n", client_info->client_id, client_info->room_id);

    // 메시지 처리
    while ((nbytes = read(client_info->client_fd, buffer, BUFFER_SIZE)) > 0) {
        buffer[nbytes] = '\0';
        printf("클라이언트 %d (%s) 메시지: %s\n", client_info->client_id, client_info->username, buffer);
        broadcast_message(client_info->client_fd, buffer, client_info->room_id);
    }

    printf("클라이언트 %d 연결 종료\n", client_info->client_id);

    // 뮤텍스 파괴 및 참조 감소 확인
    printf("클라이언트 %d 연결 종료. 뮤텍스 파괴 중...\n", client_info->client_id);
    pthread_mutex_destroy(client_info -> client_mutex);
    free(client_info -> client_mutex);
    printf("뮤텍스 파괴 완료. 클라이언트 아이디 : [ %d ] -> destroyed\n", client_info->client_id);

    close(client_info->client_fd);
    release(sp);  // 스마트 포인터 해제
    return NULL;
}
/**
 * @brief TCP 서버를 생성하고 클라이언트 연결을 처리하는 함수
 * @param num_tcp_proc 생성할 TCP 프로세스 수
 * @param ... 서버의 IP 주소와 포트를 인자로 받습니다.
 * @return int 성공 시 0, 실패 시 -1 반환
 */
int create_network_tcp_process(int num_tcp_proc, ...) {
    va_list args;
    va_start(args, num_tcp_proc);

    for (int i = 0; i < num_tcp_proc; i++) {
        int ssock, client_count = 1;
        socklen_t clen;
        struct sockaddr_in servaddr, cliaddr;
        char buffer[BUFFER_SIZE];
        char client_ip[INET_ADDRSTRLEN];

        const char *ip_address = va_arg(args, const char*);
        int port = va_arg(args, int);

        if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("socket()");
            return -1;
        }

        int enable = 1;
        if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            perror("setsockopt(SO_REUSEADDR) failed");
            return -1;
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htons(INADDR_ANY);
        servaddr.sin_port = htons(port);

        if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            perror("bind()");
            return -1;
        }

        if (listen(ssock, 8) < 0) {
            perror("listen()");
            return -1;
        } else {
            printf("서버가 포트 %d에서 듣고 있습니다.\n", port);
        }

        printf("서버가 클라이언트의 연결을 기다립니다...\n");

        pthread_t tid;
        pthread_create(&tid, NULL, server_input_handler, NULL); // 서버 입력 처리 스레드 생성

        while (1) {
            clen = sizeof(cliaddr);
            int csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
            if (csock > 0) {
                inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, INET_ADDRSTRLEN);
                printf("[ 클라이언트 %d가 연결되었습니다. IP: %s ]\n", client_count, client_ip);

                pthread_mutex_t *client_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
                pthread_mutex_init(client_mutex, NULL);

                ClientInfo *client_info = (ClientInfo *)malloc(sizeof(ClientInfo));
                client_info->client_fd = csock;
                client_info->client_id = client_count++;
                client_info->client_mutex = client_mutex;

                // 클라이언트 정보를 스마트 포인터로 관리
                client_infos[csock] = create_smart_ptr(client_info);

                // 클라이언트 스레드 생성
                pthread_create(&tid, NULL, client_handler, (void *)&client_infos[csock]);

                printf("mutex %d called\n", client_info->client_id);
                
                // 추가: 클라이언트 종료 시 뮤텍스 제거
                pthread_detach(tid);  // 스레드 분리
            }
        }

        close(ssock);  // 소켓 닫기
    }

    va_end(args);
    return 0;
}

/**
 * @brief 서버 측에서 사용자 입력을 처리하는 스레드 함수
 * @param arg 미사용
 * @param return void* 스레드 종료 시 반환값 (NULL)
 * @param argument arg 미사용
 * @return void* 스레드 종료 시 반환값 (NULL)
 */
void *server_input_handler(void *arg) {
    char buffer[BUFFER_SIZE];

    // 메뉴 출력
    print_fixed_menu();  // 메뉴 출력

    while (1) {
        // 표준 입력으로부터 메시지 입력받기
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // 개행 문자 제거

        // 종료 명령어 처리
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "...") == 0) {
            printf("채팅을 종료합니다.\n");
            exit(0);
        }

        // list 명령어 처리
        if (strcmp(buffer, "list") == 0) {
            list_users();
        }
        
        // kill 명령어 처리
        if (strncmp(buffer, "kill ", 5) == 0) {
            char *username = buffer + 5;
            kill_user(username);
        }
        
        // kill room 명령어 처리
        if (strncmp(buffer, "kill room ", 10) == 0) {
            int room_id = atoi(buffer + 10);
            kill_room(room_id);
        }

        // grep -r 명령어 처리
        if (strncmp(buffer, "grep -r", 7) == 0) {
            char command[BUFFER_SIZE + 100];
            char log_filename[50];
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            // 로그 파일명에 날짜 붙이기
            strftime(log_filename, sizeof(log_filename), "/var/log/chatlog_%Y%m%d.log", t);

            // grep 명령어에 로그 파일 경로 포함
            snprintf(command, sizeof(command), "%s %s", buffer, log_filename);
            system(command);  // 로그 파일에서 grep 명령 실행
        }

        if (strlen(buffer) > 0) {
            send_server_message(buffer);  // 서버 메시지 전송
        }
    }
    return NULL;
}
/**
 * @brief 서버 메시지를 브로드캐스트하는 함수
 * @param message 서버에서 브로드캐스트할 메시지
 * @param argument message 서버에서 브로드캐스트할 메시지
 * @return void
 */
void send_server_message(char *message) {
    char server_message[BUFFER_SIZE + 50];
    snprintf(server_message, sizeof(server_message), "[서버]: %s", message);
    log_chat_message(server_message);

    // 채팅방에 있는 클라이언트들에게 메시지 전송
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_infos[i].ptr != NULL) {
            write(i, server_message, strlen(server_message));
        }
    }   
}

/**
 * @brief daemonize 함수 구현
 * @param void
 * @return void
 */
void daemonize() {
    pid_t pid, sid;

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);  // 부모 프로세스 종료
    }

    umask(0);

    // 로그 파일을 엽니다. 기존 코드에서 파일을 올바르게 여는지 확인하세요.
    int fd = open("/home/pi/Desktop/veda/workspace/save/chat_server.log", O_RDWR | O_CREAT | O_APPEND, 0600);
        if (fd == -1) {
        perror("open log");
        printf("로그 파일 열기 실패\n");
        printf("server.c 내의 daemonize()함수에서 로그 파일을 열지 못했습니다. 새로 시작하는 장소이니 올바른 경로를 지정하세요.\n");
        exit(EXIT_FAILURE);
    } else {
        printf("로그 파일 열기 성공\n");
        printf("올바른 경로로 지정이 되었고, 시작이 되었습니다. 이 작동은 client에서 한번 Exit를 하면 server도 종료가 되므로, 반드시 daemon_start.sh를 사용하여 실행을 하세요.\n");
    }

    // 로그 파일로 stdout과 stderr 리다이렉션
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);  // 파일 디스크립터 닫기

    // 세션 ID 설정
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    // stdin, stdout, stderr 닫기
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void logo() {
    printf("\033[0;34m ▌ \033[0;33m▐·▄▄▄ .·▄▄▄▄   ▄▄▄·      ▄▄▄·  ▄▄·  ▄▄▄· ·▄▄▄▄  ▄▄▄ .• \033[0;34m▌ \033[0;37m▄ ·.  ▄· ▄▌\n");
    printf("\033[0;34m▪█·█▌▀▄.▀·██▪ ██ ▐█ ▀█     ▐█ ▀█ ▐█ ▌▪▐█ ▀█ ██▪ ██ ▀▄.▀··\033[0;37m██ ▐███▪▐█▪██▌\n");
    printf("\033[0;34m▐█▐█•▐▀▀▪▄▐█· ▐█▌▄█▀▀█     ▄█▀▀█ ██ ▄▄▄█▀▀█ ▐█· ▐█▌▐▀▀▪▄▐█ \033[0;37m▌▐▌▐█·▐█▌▐█▪\n");
    printf("\033[0;34m ███ ▐█▄▄▌██. ██ ▐█ ▪▐▌    ▐█ ▪▐▌▐███▌▐█ ▪▐▌██. ██ ▐█▄▄▌\033[0;37m██ ██▌▐█▌ ▐█▀·.\n");
    printf("\033[0;34m. \033[0;33m▀   ▀▀▀ ▀▀▀▀▀•  ▀  ▀      ▀  ▀ ·▀▀▀  ▀  ▀ ▀▀▀▀▀•  \033[0;37m▀▀▀ ▀▀  █▪▀▀▀  ▀ •\n");

    // 색상 리셋
    printf("\033[0m");
}

/**
 * @brief main 함수
 * @param void
 * @return int
 */
int main() {
    char yn = '\0';  // 문자를 저장할 변수
    
    // 로고 출력
    logo();
    printf("이 프로그램을 자동으로 데몬 화 하고 싶으시다면 스크립트를 참조하세요.\n");
    daemonize();  // 데몬화 함수 자동 호출
    // 이거로 데몬프로그램을 돌리고 싶으면 아래 주석 해제
    // Enter 키를 누르라고 안내
    // printf("\nPress Enter to continue...");
    // while (getchar() != '\n');  // Enter 키가 눌릴 때까지 대기
    // printf("이 프로그램을 자동으로 데몬 화 하시겠습니까? (y/n): ");
    
    // while (1) {
    //     yn = getchar();  // 한 문자를 입력받음
    //     if (yn == 'y' || yn == 'n') {
    //         break;  // y 또는 n을 입력받으면 루프 종료
    //     }
    //     printf("잘못된 입력입니다. (y/n): ");  // 유효하지 않은 입력에 대한 처리
    //     while (getchar() != '\n');  // 버퍼를 비워서 남은 입력을 처리
    // }

    // if (yn == 'y') {
    //     daemonize();  // 데몬화 함수 호출
    // } else {
    //     printf("데몬화를 하지 않습니다.\n");
    // }
    // 여기까지 주석

    create_network_tcp_process(1, "127.0.0.1", DEFAULT_TCP_PORT);
    return 0;
}
