/**
 * @file client.c
 * @brief 클라이언트 코드. 서버에 연결하여 로그인, 채팅, 로그아웃 기능을 제공합니다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "lib/include/user.h"  // 사용자 데이터베이스 처리

// 로그 파일 경로
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

#define PORT 5100
#define BUFFER_SIZE 1024

/**
 * @brief 서버에 연결하는 함수
 * 
 * @param host 서버의 IP 주소
 * @param port 서버의 포트 번호
 * @return int 연결된 소켓 FD, 실패 시 -1 반환
 */
int connect_to_server(const char *host, int port);

/**
 * @brief 클라이언트 로그인 함수
 * 
 * 사용자 데이터베이스에서 사용자 정보를 확인하고, 성공 시 로그인 처리합니다.
 * 
 * @param user_db 사용자 데이터베이스
 * @param username 로그인할 사용자명
 * @param password 로그인할 비밀번호
 * @return int 성공 시 1, 실패 시 0 반환
 */
int client_login_user(UserDB *user_db, char *username, char *password);

/**
 * @brief 채팅방을 선택하는 함수
 * 
 * @param sock 서버와 연결된 소켓 FD
 */
void select_chat_room(int sock);

/**
 * @brief 채팅을 시작하는 함수
 * 
 * @param sock 서버와 연결된 소켓 FD
 * @param username 사용자명
 */
void chat(int sock, char *username);

/**
 * @brief 채팅 메시지를 로그 파일에 저장하는 함수
 * 
 * @param message 저장할 메시지
 */
void log_chat_message(const char *message);

/**
 * @brief 로그아웃 및 사용자 삭제 함수
 * 
 * 로그아웃 처리 후 사용자 정보를 삭제합니다.
 * 
 * @param user_db 사용자 데이터베이스
 * @param username 로그아웃할 사용자명
 */
void logout_and_delete_user(UserDB *user_db, const char *username);

/**
 * @brief 메시지 전송 스레드 함수
 * 
 * @param args 소켓 FD 및 사용자명 배열
 * @return void* 스레드 종료 시 반환값 (NULL)
 */
void *send_messages(void *args);

/**
 * @brief 메시지 수신 스레드 함수
 * 
 * @param sock_fd 서버와 연결된 소켓 FD
 * @return void* 스레드 종료 시 반환값 (NULL)
 */
void *receive_messages(void *sock_fd);

/**
 * @brief 고정 메뉴 출력 함수
 * 
 * @param username 사용자명
 */
void print_fixed_menu(const char *username) {
    // system("clear");
    printf("\n[ 로그인 된 아이디 : %s, 채팅 메세지 키워드 : grep -r \"검색할 메세지\", 프로그램 종료 : \"exit\"]\n", username);
}


/**
 * @brief 클라이언트 메인 함수
 * 
 * @param argc 인수 개수
 * @param argv 인수 배열
 * @return int 프로그램 종료 코드
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("사용법: %s <서버 IP 주소>\n", argv[0]);
        return -1;
    }

    // IP 주소가 유효한지 확인
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, argv[1], &(sa.sin_addr));
    if (result <= 0) {
        if (result == 0) {
            printf("잘못된 IP 주소 형식입니다. 올바른 형식의 IP 주소를 입력하세요.\n");
        } else {
            perror("inet_pton 오류");
        }
        return -1;
    }
    
    // 서버에 연결
    int sock = connect_to_server(argv[1], PORT);
    if (sock < 0) {
        printf("서버에 연결할 수 없습니다.\n");
        return 1;
    }

    // 사용자 데이터베이스 초기화 및 로드
    UserDB user_db;
    init_user_db(&user_db);
    load_users_from_file(&user_db);

    char choice_str[MAX_STRING_SIZE];
    int choice;
    char username[MAX_STRING_SIZE], password[MAX_STRING_SIZE];
    bool login_success = false;

    while (1) {
        // 화면을 clear하고 메뉴를 출력
        system("clear");

        printf("\033[0;34m\n1. \033[0;32m회원가입\033[0;34m\n");  // 파랑 배경, 초록 글자
        printf("2. \033[0;33m로그인\033[0;34m\n");            // 노랑 글자
        printf("3. \033[0;31m사용자 삭제\033[0;34m\n");        // 빨강 글자
        printf("4. \033[0;36m모든 사용자 출력\033[0;34m\n");    // 청록 글자
        printf("5. \033[0;35m종료\033[0;34m\n");               // 자홍 글자
        printf("\033[0m");  // 색상 리셋
        printf("Enter your choice: ");
        scanf("%s", choice_str);  // 문자열로 입력을 받음

        // 문자열이 숫자로만 이루어졌는지 확인
        bool is_numeric = true;
        for(int i=0; i<strlen(choice_str); i++) {
            if (!isdigit(choice_str[i])) {
                is_numeric = false;
                break;
            }
        }

        // 숫자가 아닌 입력에 대한 예외 처리
        if (!is_numeric) {
            printf("잘못된 입력입니다. 숫자를 입력하세요.\n");
            sleep(2);  // 메시지를 잠시 보여주고 다시 입력 요청
            continue;
        }

        // 문자열을 정수로 변환하여 choice 처리
        choice = atoi(choice_str);

        switch (choice) {
            case 1:  // 사용자 등록
                system("clear");
                printf("Enter username: ");
                scanf("%s", username);
                printf("Enter password: ");
                scanf("%s", password);
                if (register_user(&user_db, "localhost", username, password, "user")) {
                    printf("회원가입이 완료되었습니다.\n");
                    save_user_to_file("localhost", username, password, "user");
                } else {
                    printf("회원가입에 실패했습니다.\n");
                }
                break;
            case 2:  // 사용자 로그인
                system("clear");
                if (client_login_user(&user_db, username, password)) {
                    login_success = true;
                } else {
                    printf("로그인 실패\n");
                }
                break;
            case 3:  // 사용자 삭제
                system("clear");
                printf("Enter username to delete: ");
                scanf("%s", username);
                if (delete_user(&user_db, username)) {
                    printf("사용자가 성공적으로 삭제되었습니다.\n");
                    write_txt_file(&user_db);  // 사용자 삭제 후 파일 업데이트
                } else {
                    printf("사용자를 삭제할 수 없습니다.\n");
                }
                break;
            case 4:  // 모든 사용자 출력
                display_all_users(&user_db);
                getchar();
                getchar();
                break;
            case 5:  // 종료
                system("clear");
                printf("Exiting...\n");
                close(sock);
                return 0;
            default:
                // 1번과 5번이 아닌 입력에 대한 처리
                printf("잘못된 입력입니다. 1번과 5번 중에서 선택하세요.\n");
                sleep(2);  // 메시지를 잠시 보여주고 다시 입력 요청
                break;
        }

        // 로그인에 성공한 경우에만 채팅 진행
        if (login_success) {
            select_chat_room(sock);
            chat(sock, username);  // username을 넘겨줌
            break;
        }
    }

    // 로그아웃 및 사용자 삭제
    logout_and_delete_user(&user_db, username);

    // 서버 연결 종료
    close(sock);
    return 0;
}


/**
 * @brief 서버에 연결하는 함수
 * @param host 서버의 IP 주소
 * @pram port 서버의 포트 번호
 * @return int 연결된 소켓 FD, 실패 시 -1 반환
 */
int connect_to_server(const char *host, int port) {
    int sock;
    struct sockaddr_in server_addr;
    
    // 서버 아이피 정적 할당
    const char *server_ip = "192.168.0.105";

    if(strcmp(host,"localhost") == 0 || strcmp(host, "server_ip") == 0) {
        printf("오류: 서버의 고정 IP(%s)로만 접속할 수 있습니다.\n", server_ip);
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("소켓 생성 실패");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("서버 연결 실패");
        close(sock);
        return -1;
    }

    printf("서버와 연결되었습니다.\n");
    return sock;
}

/**
 * @brief 클라이언트 로그인 함수
 * @param user_db 사용자 데이터베이스
 * @param username 로그인할 사용자명
 * @param password 로그인할 비밀번호
 * @return int 성공 시 1, 실패 시 0 반환
 */
int client_login_user(UserDB *user_db, char *username, char *password) {
    printf("로그인을 진행합니다.\n");
    printf("사용자명: ");
    scanf("%s", username);
    printf("비밀번호: ");
    scanf("%s", password);

    // 중복 로그인 방지
    if (is_user_logined_in(username)) {
        printf("이미 로그인 중인 사용자입니다.\n");
        printf("계속하려면 Enter 키를 누르세요...\n");
        getchar(); // Enter 대기
        getchar(); // 입력 방지 처리
        return 0;
    }

    // 사용자 정보 확인
    int user_idx = query_user(user_db, username, password);
    if (user_idx >= 0) {
        printf("로그인 성공!\n");
        log_user_in(username);  // 로그인 시 숨김 파일 생성
        system("clear");
        return 1;  // 로그인 성공
    } else {
        printf("존재하지 않는 사용자입니다. 회원가입을 진행하세요.\n");
        printf("계속하려면 Enter 키를 누르세요...\n");
        getchar(); // Enter 대기
        getchar(); // 입력 방지 처리
        return 0;  // 로그인 실패
    }
}

/**
 * @brief 고정 메뉴 출력 함수
 * @param username 사용자명
 * @return void
 */
void select_chat_room(int sock) {
    int chat_room_id;
    char input_str[BUFFER_SIZE];

    while (1) {
        printf("채팅룸을 선택하세요 (1 ~ 5): ");
        scanf("%s", input_str);  // 문자열로 입력 받기

        // 입력이 숫자인지 확인
        bool is_numeric = true;
        for (int i = 0; i < strlen(input_str); i++) {
            if (!isdigit(input_str[i])) {
                is_numeric = false;
                break;
            }
        }

        // 숫자가 아닐 경우 예외 처리
        if (!is_numeric) {
            printf("잘못된 입력입니다. 숫자를 입력하세요.\n");
            continue;  // 잘못된 입력이므로 다시 입력 받음
        }

        // 숫자로 변환
        chat_room_id = atoi(input_str);

        // 1 ~ 5 범위의 채팅방 번호 확인
        if (chat_room_id < 1 || chat_room_id > 5) {
            printf("유효한 채팅룸을 선택하세요 (1 ~ 5): ");
            continue;
        }

        // 올바른 입력이므로 반복문 탈출
        break;
    }

    char room_message[BUFFER_SIZE];
    snprintf(room_message, sizeof(room_message), "%d", chat_room_id);
    send(sock, room_message, strlen(room_message), 0);

    printf("채팅룸 %d에 입장합니다.\n", chat_room_id);
}

/**
 * @brief 채팅을 시작하는 함수
 * @param sock 서버와 연결된 소켓 FD
 * @param username 사용자명
 * @return void
 */
void chat(int sock, char *username) {
    pthread_t send_thread, receive_thread;

    // sock과 username을 함께 전달할 수 있도록 배열 생성
    int sock_and_username[BUFFER_SIZE / sizeof(int)];
    sock_and_username[0] = sock;
    strncpy((char *)&sock_and_username[1], username, BUFFER_SIZE - sizeof(int));

    // 스레드 생성
    pthread_create(&send_thread, NULL, send_messages, (void *)sock_and_username);
    pthread_create(&receive_thread, NULL, receive_messages, &sock);

    // 스레드 종료될 때까지 대기
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
}

/**
 * @brief 고정 메뉴 출력 함수
 * @param username 사용자명
 * @return void
 */
void *send_messages(void *args) {
    int *sock_and_username = (int *)args;
    int sock = sock_and_username[0];
    char *username = (char *)&sock_and_username[1];

    char buffer[BUFFER_SIZE];
    char message_with_username[BUFFER_SIZE + 50];  
    char grep_command[BUFFER_SIZE + 50];  

    print_fixed_menu(username);

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        // grep 명령을 처리하는 경우
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
        } else {
            int ret = snprintf(message_with_username, sizeof(message_with_username), "[%s]: %s", username, buffer);
            if (ret >= sizeof(message_with_username)) {
                printf("경고: 메시지가 너무 깁니다. 일부가 잘렸을 수 있습니다.\n");
            }

            send(sock, message_with_username, strlen(message_with_username), 0);

            if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "...") == 0) {
                printf("채팅을 종료합니다.\n");

                // 로그아웃 시 숨김 파일 삭제
                logout_user(username);

                close(sock);
                exit(0);
            }

            if (strlen(buffer) > 0) {
                printf("본인 [%s]의 메세지 : %s\n", username, buffer);
            }
        }
    }
}

/**
 * @brief 메시지 수신 스레드 함수
 * @param sock_fd 서버와 연결된 소켓 FD
 * @return void* 스레드 종료 시 반환값 (NULL)
 */
void *receive_messages(void *sock_fd) {
    int sock = *(int *)sock_fd;
    char buffer[BUFFER_SIZE];
    int n;

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (n > 0) {
            // 본인이 보낸 메시지가 아닐 때만 출력
            if (!strstr(buffer, "본인 [")) {
                printf("%s\n", buffer);  // 수신한 메시지 출력
            }
        } else if (n == 0) {
            printf("서버 연결 종료.\n");
            exit(0);
        }
    }
}

/**
 * @brief 로그아웃 및 사용자 삭제 함수
 * @param user_db 사용자 데이터베이스
 * @param username 로그아웃할 사용자명
 * @return void
 */
void logout_and_delete_user(UserDB *user_db, const char *username) {
    printf("로그아웃 처리 중...\n");
    log_chat_message("User logged out.");

    // 로그아웃 시 숨김 파일 삭제
    logout_user(username);

    delete_user(user_db, username);
    write_txt_file(user_db);  // 파일에 저장
}

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
