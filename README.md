## 개요

 이 스마트 포인터 기반 프로젝트는 주로 스마트 포인터의 구현과 활용을 중심으로 한 채팅 서버를 개발하는 내용입니다. 이 채팅 서버는 리눅스 시스템에서 다중 클라이언트와의 통신을 관리하며, 메모리 관리와 자원 해제를 효율적으로 하기 위해 스마트 포인터를 사용합니다.

### 스마트포인터 라이브러리

**[c_smartpointer]**  
[![Readme Card](https://github-readme-stats.vercel.app/api/pin/?username=Azabell1993&repo=c_smartpointer)](https://github.com/Azabell1993/c_smartpointer)

<img width="558" alt="스크린샷 2024-09-13 오후 4 23 41" src="https://github.com/user-attachments/assets/dc5e9bd2-22d7-4e85-956b-b58b39b5bf74">

-------------

## 필수 사항(경로 수정)
1. server.c - daemonize() 함수내 경로
```
    // 로그 파일을 엽니다. 기존 코드에서 파일을 올바르게 여는지 확인하세요.
    int fd = open("/home/ubuntu/Desktop/workspace/exam_chat_mutex_server/save/chat_server.log", O_RDWR | O_CREAT | O_APPEND, 0600);
    if (fd == -1) {
        perror("open log");
        exit(EXIT_FAILURE);
    }
```

2. log 기반 grep -r 명령어
```
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
```  
서버에서  
> sudo tail -f /var/log/chatlog_20240915.log
를 하면 로그에 실시간으로 메세지가 쌓이는 것을 확인하실 수 있습니다.
그리고, 서버에서 수동으로 시작하는 메뉴를 선택하여야합니다. (현재 grep은 서버에서만 가능)

 
3. start_daemon.sh 경로
**예시**
```
SERVER_PATH="/home/ubuntu/Desktop/workspace/exam_chat_mutex_server/save"
LOG_FILE="/home/ubuntu/Desktop/workspace/exam_chat_mutex_server/save/chatlog_$(date +'%Y%m%d').log"
```  

```
SERVER_PATH="/home/pi/smartpointer_multi_chat"
LOG_FILE="/home/pi/smartpointer_multi_chat/chatlog_$(date +'%Y%m%d').log"
```

4. 서버 동작 방법은 두가지입니다.
(1) 자동 : 수동으로 서버관리를 직접적으로 관여하기 힘듬
(2) 수동 : 수동으로 서버관리를 직접적으로 관여가 가능

## 주의사항
1. chat_server 로 실행시 백그라운드 실행이 가능하나, daemon_start.sh를 하여샤 완전한 백그라운드가 됩니다.
2. 서버 연결시 올바른 아이피를 입력하셔야합니다.

## 시연 모습
<img width="690" alt="스크린샷 2024-09-13 오후 4 28 09" src="https://github.com/user-attachments/assets/0246620f-72f4-4330-8f52-e28fe79dec63">
<img width="938" alt="스크린샷 2024-09-13 오후 4 29 09" src="https://github.com/user-attachments/assets/ea0cefab-6b93-486f-aa97-67d74e3ffecf">
<img width="1487" alt="스크린샷 2024-09-13 오후 4 29 52" src="https://github.com/user-attachments/assets/63ba7f96-4d8d-4d14-a7ff-93235c2804d6">
<img width="1643" alt="스크린샷 2024-09-13 오후 4 30 06" src="https://github.com/user-attachments/assets/4f1f2a6c-3002-4e37-803b-091a644ee6a5">

## 전체 아키텍트
이 시스템은 서버와 클라이언트가 소켓 통신을 통해 메시지를 주고받으며, 서버가 다수의 클라이언트를 관리하는 구조입니다. 스마트 포인터를 사용하여 메모리 관리를 쉽게 하고, 다수의 클라이언트를 관리할 때 참조 카운팅과 쓰레드 안전성을 확보하고 있습니다.

### 1. 스마트 포인터의 역할
스마트 포인터는 smartptr.h 및 uniqueptr.h 파일에 정의되어 있으며, 동적 메모리 관리와 참조 카운트 관리를 통해 메모리 해제를 자동화합니다. 또한, 뮤텍스를 사용하여 다중 쓰레드 환경에서 메모리 접근을 안전하게 보장합니다.

SharedPtr: 공유할 수 있는 포인터로, 참조 카운트를 증가시켜 여러 곳에서 안전하게 사용합니다. 서버에서 클라이언트 정보를 관리할 때 사용됩니다.
UniquePtr: 하나의 쓰레드에서만 사용할 수 있는 포인터로, 소유권 이전이 가능하며 참조 카운트 없이 고유한 메모리 관리를 합니다.

### 2. 서버-클라이언트 통신 구조
서버와 클라이언트는 소켓을 통해 연결되고, 서버는 여러 클라이언트의 메시지를 받아 처리하며, 각 클라이언트의 요청을 다중 스레드로 처리합니다.

서버는 클라이언트의 연결을 받아들이고, 각각의 클라이언트에 대해 스레드를 생성하여 처리합니다.
클라이언트는 서버에 접속 후, 로그인, 회원가입, 채팅 등 다양한 기능을 사용할 수 있습니다.

### 3. 서버 흐름
서버 시작 (server.c)

서버는 create_network_tcp_process 함수를 통해 소켓을 생성하고, 클라이언트의 연결을 기다립니다.
클라이언트가 연결되면 client_handler 스레드가 생성됩니다.
스마트 포인터로 클라이언트 관리

각 클라이언트의 정보는 스마트 포인터로 관리됩니다. 클라이언트 연결 시 create_smart_ptr 함수를 통해 클라이언트 정보를 동적으로 할당하고, 스마트 포인터로 참조 카운트를 관리합니다.
채팅 및 메시지 브로드캐스트

클라이언트가 채팅방에 들어가면 서버는 각 클라이언트의 메시지를 받고, 이를 broadcast_message 함수를 통해 같은 채팅방의 다른 클라이언트에게 전송합니다.
메시지는 클라이언트 ID와 함께 전송되며, 로그 파일에도 기록됩니다.
클라이언트 종료

클라이언트가 연결을 끊으면 스마트 포인터의 참조 카운트가 감소하고, 참조 카운트가 0이 되면 메모리가 자동으로 해제됩니다.

### 4. 클라이언트 흐름
서버 연결 (client.c)

클라이언트는 connect_to_server 함수를 통해 서버와 소켓을 통해 연결됩니다.
로그인 및 회원가입

사용자는 로그인 또는 회원가입을 선택할 수 있습니다. 회원가입 시 사용자 정보는 스마트 포인터로 관리되며, save_user_to_file 함수로 파일에 저장됩니다.
채팅방 선택 및 메시지 전송

사용자가 로그인에 성공하면 채팅방을 선택하고, 메시지를 입력할 수 있습니다. 클라이언트는 서버로 메시지를 전송하고, 서버에서 다시 받아서 출력합니다.
스레드 관리

클라이언트는 메시지를 보내는 스레드와 받는 스레드를 각각 생성하여 서버와 통신합니다.
**send_messages**와 receive_messages 스레드가 각각 메시지 전송 및 수신을 담당합니다.

### 5. 스마트 포인터의 방향성과 용도
- 방향성: 스마트 포인터는 주로 클라이언트나 서버에서 생성한 데이터를 관리하는데 사용됩니다. 예를 들어, 클라이언트가 서버에 연결되면 클라이언트 정보를 스마트 포인터로 감싸서 관리하며, 다수의 쓰레드에서 안전하게 참조할 수 있습니다.
- 용도: 스마트 포인터는 참조 카운트와 뮤텍스를 통해 자동 메모리 관리 및 쓰레드 안전성을 제공합니다. 예를 들어, 클라이언트가 종료되면 참조 카운트가 0이 되어 메모리가 자동 해제됩니다.

### 6. 메모리 구조 분석 (서버-클라이언트 통신)
**서버 스택**
client_infos[] 배열: 스마트 포인터로 관리되는 클라이언트 정보 배열입니다.
SmartPtr 객체: 클라이언트 정보를 관리하며, 각 클라이언트의 참조 카운트를 관리합니다.
쓰레드 스택: 각 클라이언트는 별도의 쓰레드를 생성하여 소켓 통신을 처리합니다.

**클라이언트 스택**
사용자 정보 (UserInfo): 스마트 포인터로 사용자 정보를 관리합니다.
채팅방 선택 및 메시지 전송: 사용자 입력은 서버로 전송되고, 서버는 다시 클라이언트에게 브로드캐스트합니다.
스레드 스택: 메시지 전송 및 수신을 각각의 스레드에서 처리하여 비동기 통신을 구현합니다.

### 7. 순서도
**서버 시작**
  소켓 생성 -> 클라이언트 대기 -> 연결 수락 -> 클라이언트 스레드 생성
**클라이언트 로그인**
  서버 연결 -> 로그인 요청 -> 서버에서 사용자 정보 확인 -> 로그인 성공 시 채팅방 선택
**채팅 흐름**
  클라이언트: 메시지 입력 -> 서버로 전송
  서버: 메시지 수신 -> 같은 채팅방 클라이언트에게 브로드캐스트
**클라이언트 종료**
  클라이언트 연결 종료 -> 스마트 포인터 참조 카운트 0 -> 메모리 해제

이 구조를 통해, 스마트 포인터는 동적 메모리 할당과 해제를 자동으로 처리하며, 다중 스레드 환경에서 안전하게 참조 카운트를 관리합니다. 이를 통해 메모리 누수와 동시성 문제를 방지합니다.


-------------

#### 코드 설명
### 1. ename.c.inc
ename.c.inc 파일은 리눅스에서 발생할 수 있는 다양한 에러코드를 문자열로 변환하는 데 사용되는 코드입니다. 시스템 콜이나 함수 호출이 실패할 경우 errno 변수를 통해 오류를 반환하는데, 이 코드는 그 오류 번호에 맞는 이름을 가져오는 역할을 합니다.

#### 주요 기능 및 설명
ename 배열은 오류 번호에 대응하는 오류 메시지를 담고 있습니다. 예를 들어, errno가 2일 경우 "ENOENT"라는 메시지를 출력할 수 있습니다.
이 배열은 주로 로그 메시지 출력이나 오류 처리 시 사용됩니다.
MAX_ENAME은 최대 에러 코드 개수를 정의합니다.
이 파일은 채팅 서버의 동작 과정에서 발생할 수 있는 시스템 콜의 오류를 처리하는 데 사용됩니다. 예를 들어, 클라이언트가 소켓 연결에 실패했을 때 그 오류를 사람이 읽을 수 있는 형식으로 변환하여 로그로 기록하거나, 디버깅 시 오류를 쉽게 이해할 수 있게 해줍니다.

### 2. user.h
user.h는 사용자 정보와 사용자 데이터베이스를 관리하는 구조와 함수를 정의합니다. 이 파일에서는 스마트 포인터를 활용하여 메모리 관리를 최적화하고, 다중 사용자 등록, 로그인, 삭제 등을 구현합니다.

##### 스마트 포인터 구현과 활용
SmartPtr: 사용자 정보(UserInfo)를 스마트 포인터로 관리함으로써, 수동적인 메모리 할당과 해제를 방지하고 참조 카운트를 통해 자동으로 메모리를 해제합니다. 이는 메모리 누수(memory leak)를 방지하는 데 큰 도움이 됩니다.
UserInfo: 각 사용자의 호스트명(host), 사용자명(user), 비밀번호(pass), 이름(name)을 스마트 포인터로 관리하여, 메모리 안전성을 보장합니다.

##### 핵심 함수 설명
register_user: 새로운 사용자를 데이터베이스에 추가합니다. 스마트 포인터를 사용하여 host, user, pass, name 정보를 안전하게 관리합니다.
query_user: 주어진 사용자명과 비밀번호가 데이터베이스에 있는지 확인하는 함수로, 로그인 시 사용자 검증에 사용됩니다.
log_user_in, logout_user: 사용자가 로그인 상태인지 확인하고, 로그인 시 숨김 파일을 생성하며, 로그아웃 시 그 파일을 삭제합니다.
delete_user: 사용자를 데이터베이스에서 삭제하고, 삭제 후 파일에 반영합니다.
리눅스 개념과 연관:
이 파일은 주로 리눅스 파일 시스템과 접근 권한에 의존합니다. 예를 들어, .username 파일을 생성하여 사용자가 로그인 중임을 기록하며, unlink()를 사용해 로그아웃 시 파일을 삭제합니다. 또한, pthread_mutex를 사용하여 다중 스레드 환경에서 데이터베이스 접근을 안전하게 관리합니다.

### 3. client.c
client.c 파일은 클라이언트 측 프로그램으로, 서버와의 통신을 처리하고 사용자 인터페이스를 제공합니다. 사용자는 서버에 연결하여 로그인하거나 채팅방을 선택하고 메시지를 주고받을 수 있습니다.

##### 주요 흐름
서버 연결: connect_to_server 함수는 서버의 IP 주소와 포트 번호를 기반으로 TCP 소켓을 생성하고 서버에 연결합니다.
로그인 및 사용자 관리: client_login_user 함수는 사용자 데이터베이스를 통해 로그인 과정을 처리합니다. 중복 로그인 방지를 위해 숨김 파일을 사용합니다.
채팅방 선택 및 메시지 송수신: 사용자는 채팅방을 선택하고 send_messages, receive_messages 함수를 통해 메시지를 주고받습니다. 이 과정에서 스마트 포인터로 관리되는 사용자 정보가 메시지에 포함됩니다.
스레드 관리 및 리눅스 시스템 호출:
클라이언트 측에서 메시지를 주고받는 과정은 pthread_create로 생성된 두 개의 스레드를 통해 처리됩니다. 하나는 메시지를 전송하고, 다른 하나는 서버로부터 메시지를 수신합니다.
스마트 포인터 활용: 클라이언트의 사용자명 등 동적 데이터를 안전하게 관리하기 위해 스마트 포인터를 사용합니다.
리눅스 시스템 호출: 클라이언트는 socket(), connect(), send(), recv() 등 네트워크 통신에 필요한 시스템 콜을 사용합니다. 또한, 채팅방 선택과 메시지 전송 과정에서 문자열 처리와 버퍼 관리를 위해 snprintf 등의 표준 C 라이브러리를 사용합니다.

### 4. server.c
server.c는 서버 측 프로그램으로, 다수의 클라이언트와의 연결을 처리하며 채팅 메시지를 중계합니다. 서버는 클라이언트 정보를 스마트 포인터로 관리하여, 메모리 할당과 해제를 자동으로 처리합니다.

#### 스마트 포인터 구현
SmartPtr: 스마트 포인터는 서버에서 관리하는 클라이언트 정보(ClientInfo)를 동적 메모리로 관리하며, 참조 카운트와 뮤텍스를 통해 멀티스레드 환경에서도 안전하게 메모리를 관리할 수 있게 합니다.
retain 함수: 스마트 포인터의 참조 카운트를 증가시킵니다.
release 함수: 참조 카운트를 감소시키고, 마지막 참조 해제 시 메모리를 자동으로 해제합니다.

#### 클라이언트 관리와 스레드 처리
서버는 각 클라이언트를 스마트 포인터로 관리되는 배열에 저장하며, 클라이언트 연결 시 client_handler 스레드를 생성합니다. 이 스레드는 클라이언트의 메시지를 처리하고, 해당 채팅방에 있는 다른 클라이언트에게 메시지를 브로드캐스트합니다.
클라이언트 종료 처리: 클라이언트 연결이 종료될 때, 스마트 포인터가 관리하는 메모리를 자동으로 해제하며, pthread_mutex_destroy로 클라이언트의 뮤텍스도 안전하게 제거합니다.
리눅스 네트워크 통신 및 멀티스레딩:
서버는 socket(), bind(), listen(), accept() 등의 시스템 콜을 사용하여 TCP 서버를 설정하고, 클라이언트 연결을 수락합니다.
각 클라이언트는 별도의 스레드로 관리되며, pthread_create를 통해 클라이언트 스레드를 생성합니다.
뮤텍스와 동기화: pthread_mutex_t를 사용하여 클라이언트 정보에 대한 동시 접근을 안전하게 제어합니다. 이로 인해 다중 스레드 환경에서도 데이터의 무결성이 보장됩니다.

#### 기능 설명
broadcast_message: 특정 채팅방에 있는 모든 클라이언트에게 메시지를 브로드캐스트합니다.
kill_user, kill_room: 특정 유저나 채팅방을 강제로 종료할 수 있는 관리자 기능을 제공합니다.
로그 기록: 모든 메시지는 log_chat_message를 통해 파일에 저장되어, 나중에 검색하거나 참조할 수 있습니다.

### 프로젝트의 아키텍처 및 리눅스 개념 분석
#### 1. 아키텍처 구조도
클라이언트-서버 모델: 프로젝트는 전형적인 클라이언트-서버 아키텍처를 따릅니다. 다수의 클라이언트가 하나의 중앙 서버에 연결하여 메시지를 주고받으며, 서버는 클라이언트 간의 통신을 중계합니다.

멀티스레드 서버: 서버는 각 클라이언트 연결을 처리하기 위해 스레드 풀을 사용하지 않고, 클라이언트 연결 시마다 새로운 스레드를 생성합니다. 이를 통해 서버는 동시 다발적으로 다수의 클라이언트와 통신할 수 있습니다.

스마트 포인터 메모리 관리: 프로젝트 전반에 걸쳐 스마트 포인터를 사용하여 동적 메모리를 안전하게 관리합니다. 이는 수동적인 메모리 해제에서 발생할 수 있는 오류(예: 메모리 누수)를 방지하며, 멀티스레드 환경에서도 안전하게 동작하도록 설계되었습니다.

#### 2. 리눅스 개념 사용
네트워크 통신: socket(), bind(), connect(), accept(), send(), recv() 등의 시스템 콜을 사용하여 서버와 클라이언트 간의 TCP 통신을 구현합니다. 이는 리눅스의 소켓 프로그래밍 기초입니다.

멀티스레딩: pthread_create(), pthread_mutex_lock(), pthread_mutex_unlock() 등의 pthread 라이브러리를 활용하여 다중 스레드 환경에서의 동시성을 처리합니다. 서버는 각 클라이언트의 연결을 별도의 스레드에서 처리하므로, 여러 클라이언트가 동시에 채팅할 수 있습니다.

파일 시스템 접근: 사용자의 로그인 상태를 .username 파일로 관리하며, access(), unlink() 등을 통해 파일 시스템에 접근하고, 사용자의 상태를 관리합니다.

로그 기록 및 검색: 채팅 로그는 파일로 저장되며, 서버 관리자 또는 클라이언트는 grep 명령어를 사용하여 특정 메시지를 검색할 수 있습니다.


------------------


#### 프로그램을 설계하고 코드를 작성하는 과정에서, 각 기능의 흐름을 예상하고 구조화하는 것이 매우 중요합니다. 이 프로젝트는 다중 클라이언트와 서버 간의 통신을 처리하는 채팅 시스템이며, 스마트 포인터를 활용하여 메모리 관리를 최적화한 것이 핵심입니다. 여기서는 이 프로그램을 설계할 때 코드를 어떻게 흐름에 맞게 예측하고 구조화했는지 단계별로 설명하겠습니다.

### 1. 요구 사항 분석 및 기능 정의
먼저, 프로그램에서 필요한 주요 기능을 정의해야 합니다. 이 채팅 시스템의 핵심 요구 사항은 다음과 같습니다:

서버는 다수의 클라이언트를 지원하며, 각 클라이언트는 채팅방에 입장해 메시지를 주고받을 수 있다.
사용자 등록, 로그인, 삭제 기능이 제공되어야 한다.
사용자 정보를 안전하게 관리하기 위한 메모리 관리 메커니즘이 필요하다.
멀티스레드 환경에서 클라이언트의 데이터를 안전하게 관리해야 한다.
서버는 각 클라이언트를 스레드로 처리하고, 메시지를 브로드캐스트해야 한다.
이를 바탕으로 기능을 구체화하고, 흐름을 어떻게 설계할지 결정했습니다.

### 2. 전체 아키텍처 설계
프로그램의 아키텍처는 크게 두 부분으로 나뉩니다.

#### 2.1 서버 아키텍처
서버는 클라이언트의 요청을 처리하고, 클라이언트 간의 메시지 전달을 중계하는 역할을 합니다. 이를 위해 다음과 같은 컴포넌트가 필요합니다.

클라이언트 관리: 클라이언트는 각각 고유한 사용자 정보를 가지고 있으며, 이 정보를 기반으로 통신을 해야 합니다. 서버는 클라이언트가 연결되면 ClientInfo라는 구조체에 클라이언트 정보를 저장하고 관리합니다. 이 구조체는 소켓 파일 디스크립터, 사용자명, 채팅방 ID 등을 포함합니다.

스마트 포인터 메모리 관리: 각 클라이언트의 정보는 스마트 포인터를 통해 안전하게 관리됩니다. 서버는 클라이언트가 접속하거나 나갈 때마다 이 스마트 포인터의 참조 카운트를 증가 또는 감소시키며, 참조 카운트가 0이 되면 해당 메모리를 자동으로 해제합니다.

채팅방 관리: 서버는 여러 채팅방을 지원해야 하며, 클라이언트는 채팅방을 선택하고 해당 방에서만 메시지를 주고받을 수 있어야 합니다. 이를 위해 room_id를 각 클라이언트의 정보에 추가하여, 메시지가 해당 방의 모든 클라이언트에게만 전송되도록 설계했습니다.

스레드 관리: 서버는 각 클라이언트 연결을 별도의 스레드에서 처리합니다. 스레드는 클라이언트로부터 받은 메시지를 읽고, 해당 클라이언트가 속한 채팅방의 다른 사용자들에게 브로드캐스트하는 역할을 합니다. 서버 자체도 입력을 받아서 전체 메시지를 전송하거나, 특정 명령을 수행할 수 있습니다.

#### 2.2 클라이언트 아키텍처
클라이언트는 서버에 연결하여 메시지를 주고받고, 채팅방을 선택하며, 로그인을 처리하는 역할을 합니다.

서버 연결 및 로그인: 클라이언트는 서버의 IP 주소와 포트를 통해 연결되며, 사용자는 서버에 로그인합니다. 로그인 후에는 서버로부터 메시지를 수신하고, 사용자가 입력한 메시지를 서버로 전송하는 기능을 수행합니다.
스레드 분리: 메시지 송수신을 독립적으로 처리하기 위해, 두 개의 스레드를 생성하여 하나는 메시지 전송, 다른 하나는 메시지 수신을 담당합니다. 이는 채팅 중에도 사용자가 메시지를 입력할 수 있도록 비동기 처리를 지원합니다.

### 3. 서버와 클라이언트 간의 통신 흐름
#### 3.1 서버 흐름
서버 초기화: create_network_tcp_process 함수에서 서버는 TCP 소켓을 생성하고, 특정 포트에 바인딩한 후 클라이언트 연결을 기다립니다.

socket(), bind(), listen() 함수를 통해 리눅스의 소켓 API를 사용하여 서버를 준비합니다.
클라이언트 연결 처리: 클라이언트가 연결을 시도하면, 서버는 accept() 호출로 이를 받아들입니다.

클라이언트가 연결되면 서버는 해당 클라이언트 정보를 담고 있는 ClientInfo 구조체를 동적으로 생성합니다.
이 클라이언트 정보는 스마트 포인터를 사용하여 안전하게 관리됩니다.
스레드 생성 및 클라이언트 처리: 각 클라이언트는 별도의 스레드에서 처리됩니다. 이 스레드는 클라이언트의 메시지를 수신하고, 해당 클라이언트의 채팅방에 있는 다른 클라이언트들에게 메시지를 브로드캐스트합니다.

참조 카운트 기반 메모리 관리: 각 클라이언트 정보는 스마트 포인터로 관리되며, 스레드가 종료되면 스마트 포인터가 메모리를 안전하게 해제합니다.
메시지 브로드캐스트: 클라이언트로부터 메시지를 받으면 broadcast_message 함수가 호출됩니다. 이 함수는 해당 채팅방에 속한 모든 클라이언트에게 메시지를 전달합니다.

서버는 멀티스레드 환경에서 안전하게 메시지를 전달하기 위해 뮤텍스를 사용하여 메시지 전송과 로그 기록을 보호합니다.
클라이언트 종료 처리: 클라이언트가 연결을 끊으면 서버는 해당 클라이언트의 정보를 해제합니다. 이때 스마트 포인터의 참조 카운트가 0이 되면 메모리를 자동으로 해제하며, 클라이언트의 뮤텍스도 안전하게 파괴됩니다.

#### 3.2 클라이언트 흐름
서버 연결: 클라이언트는 connect_to_server 함수를 통해 서버와 연결됩니다. IP 주소와 포트를 인자로 받아 서버와의 TCP 연결을 수립합니다.

로그인 및 채팅방 선택: 사용자는 먼저 로그인 과정을 거치며, 로그인 후 채팅방을 선택합니다. 로그인 상태는 .username 파일을 통해 관리되며, 로그인 중인 사용자는 중복 로그인할 수 없도록 설계되었습니다.

메시지 송수신: 클라이언트는 두 개의 스레드를 사용하여 메시지를 송신하고 수신합니다.

송신 스레드는 사용자가 입력한 메시지를 서버로 전송합니다. 이때 입력한 메시지에 사용자명을 포함하여, 서버가 브로드캐스트할 수 있도록 포맷팅합니다.
수신 스레드는 서버로부터 메시지를 받아 이를 출력합니다. 수신한 메시지가 본인이 보낸 것이 아니라면 콘솔에 출력합니다.
로그아웃 및 종료: 사용자가 로그아웃하거나 프로그램을 종료하면, 클라이언트는 서버 연결을 종료하고, .username 파일을 삭제하여 로그아웃 상태를 기록합니다.

### 4. 스마트 포인터 설계 및 활용
이 프로젝트에서 스마트 포인터는 주로 메모리 관리 최적화와 멀티스레드 안전성을 보장하기 위해 사용되었습니다.

#### 4.1 스마트 포인터 설계 의도
스마트 포인터는 동적 메모리를 자동으로 관리하여, 메모리 누수를 방지하고 참조 카운트를 통해 메모리의 정확한 해제를 보장합니다. 멀티스레드 환경에서는 참조 카운트를 보호하기 위해 뮤텍스가 사용됩니다.

참조 카운트 관리: 스마트 포인터는 참조 카운트를 가지고 있으며, 참조가 증가할 때는 retain, 참조가 감소할 때는 release 함수를 통해 관리됩니다. 참조 카운트가 0이 되면 동적 메모리가 자동으로 해제됩니다.

뮤텍스 보호: 스마트 포인터는 멀티스레드 환경에서 안전하게 동작하도록 참조 카운트를 뮤텍스로 보호합니다. 이를 통해 동시 접근 시에도 메모리 관리의 안전성을 확보합니다.

#### 4.2 스마트 포인터의 활용
스마트 포인터는 주로 다음과 같은 상황에서 사용됩니다:

클라이언트 정보 관리: 클라이언트의 연결 정보(ClientInfo)는 동적으로 생성되며, 스마트 포인터를 통해 관리됩니다. 이를 통해 클라이언트가 연결을 끊었을 때 자동으로 메모리가 해제되며, 메모리 누수가 발생하지 않도록 설계되었습니다.
다중 클라이언트 처리: 서버는 동시에 여러 클라이언트와 통신하므로, 각 클라이언트의 정보를 독립적으로 안전하게 관리하기 위해 스마트 포인터를 사용합니다. 각 클라이언트의 스레드가 종료되면 자동으로 해당 클라이언트 정보를 메모리에서 해제합니다.

### 5. 리눅스 시스템과의 연관성
이 프로젝트는 리눅스의 여러 핵심 개념을 활용합니다:

소켓 프로그래밍: 서버와 클라이언트는 socket(), bind(), listen(), accept(), connect(), send(), recv() 등 리눅스의 소켓 API를 사용하여 TCP 통신을 처리합니다.

멀티스레딩: 서버와 클라이언트는 모두 pthread_create, pthread_join, pthread_mutex_lock, pthread_mutex_unlock 등을 사용하여 멀티스레드를 관리하고, 동기화 문제를 해결합니다. 특히, 클라이언트가 채팅방에서 메시지를 주고받을 때 스레드가 독립적으로 동작할 수 있도록 설계되었습니다.

파일 시스템 사용: 클라이언트는 로그인 상태를 기록하기 위해 .username 파일을 사용하고, 서버는 채팅 메시지를 로그 파일로 기록합니다. 이러한 파일 관리는 리눅스의 파일 시스템 API를 통해 구현되었습니다.

### 6. 요약
이 프로그램은 스마트 포인터를 활용한 메모리 관리, 멀티스레드 서버 및 클라이언트 구현, 그리고 리눅스 시스템 콜을 사용한 네트워크 통신의 개념을 바탕으로 설계되었습니다. 각각의 컴포넌트는 독립적으로 동작하며, 전체 시스템의 흐름은 클라이언트와 서버 간의 메시지 교환을 중심으로 이루어집니다. 스마트 포인터는 메모리 안전성을 보장하고, 멀티스레드 환경에서 발생할 수 있는 문제를 해결하기 위한 핵심 요소로 설계되었습니다.

이 설계를 통해 다중 클라이언트 환경에서 안전하고 확장 가능한 채팅 서버를 구축할 수 있었습니다.



---------------

## 상세 기술 스택 설명

### 메모리 스택 관리 상세 설명
### 1. 서버 메모리 스택 관리
서버는 다수의 클라이언트를 관리하며, 각 클라이언트는 독립된 스레드에서 처리됩니다. 각 스레드마다 고유의 스택 메모리가 있으며, 클라이언트와의 통신에서 발생하는 데이터는 스택과 힙을 혼합하여 관리됩니다.

##### 서버 실행 흐름
- 메인 스레드
  1. 소켓 생성 및 바인딩, 클라이언트의 연결 대기.
  2. 클라이언트가 연결되면, client_handler 함수에서 새로운 스레드를 생성하여 클라이언트와의 통신을 처리.
- 스레드 스택 메모리
  1. 각 스레드가 클라이언트 요청을 처리할 때, ClientInfo 구조체는 스마트 포인터로 관리됩니다.
  2. 클라이언트의 정보는 힙 영역에 동적으로 할당되며, **스마트 포인터(SmartPtr)**는 ptr 멤버로 실제 데이터를 가리킵니다. 스마트 포인터는 스택에 위치하고,
    참조 카운트는 힙 메모리에서 관리됩니다.
  ```
  SmartPtr client_info_ptr = create_smart_ptr(client_info);
  ```
  여기서 **client_info_ptr**은 스택에서 관리되고, **client_info**는 힙에 동적으로 할당됩니다. 참조 카운트는 힙에 저장되어 여러 스레드에서 안전하게 관리되며, 스레드 간에 해당 메모리의 참조 횟수를 동기화합니다.

  - 스마트 포인터의 역할  
  스마트 포인터는 각 스레드에서 클라이언트 정보를 안전하게 참조하도록 합니다.
  스레드가 종료되거나 클라이언트와의 연결이 종료될 때, 참조 카운트가 감소하고, 0이 되면 메모리가 자동으로 해제됩니다.
### 2. 클라이언트 메모리 스택 관리
클라이언트는 서버와 통신하기 위해 주어진 흐름에서 데이터를 전송하거나, 메시지를 수신받아 처리하는 과정을 반복합니다. 클라이언트 쪽에서는 상대적으로 더 적은 메모리 사용이 발생하며, 스레드가 서버로 데이터를 전송하고 수신합니다.

#### 클라이언트 스택 흐름
  클라이언트는 메모리 스택에서 UserDB, UserInfo 등을 관리하며, 스마트 포인터는 각 사용자의 정보를 안전하게 힙 메모리로 관리합니다.
  ```
  UserInfo new_user;
  new_user.host = CREATE_SMART_PTR(char[MAX_STRING_SIZE], host);
  ```
  이때, new_user.host는 힙 메모리에 저장된 데이터를 가리키고, 스마트 포인터가 이를 관리합니다. 스마트 포인터는 스택 메모리에 존재하면서 힙 메모리를 참조하며, 참조 카운트를 통해 동적 메모리를 관리합니다.

  스레드 스택: 클라이언트에서 메시지를 보내는 스레드와 수신하는 스레드가 각각 생성되며, 해당 스레드들은 자신만의 스택 메모리를 관리합니다. 이를 통해 메시지 처리 시 비동기 통신을 수행합니다.

### 3. 스택-힙의 연관성
서버와 클라이언트 모두에서 메모리 할당은 스택과 힙 간에 구분됩니다. 동적 할당된 메모리는 힙에 존재하며, 그 참조는 스마트 포인터를 통해 관리됩니다. 스마트 포인터 자체는 스택에 존재하지만, 힙에 할당된 메모리를 참조하며, 스마트 포인터가 더 이상 사용되지 않으면 힙 메모리는 자동으로 해제됩니다.
##### 스마트 포인터의 해제 과정
  스마트 포인터는 각 스레드에서 참조되며, 참조 카운트가 0이 되면 해당 메모리 영역을 힙에서 해제합니다. 이 과정은 서버, 클라이언트 양쪽에서 동일하게 관리됩니다.


### 가변인자의 활용
  가변인자는 스마트 포인터의 초기화 과정에서 활용됩니다. 가변인자를 통해 동적으로 초기값을 설정할 수 있습니다. 본 프로젝트에서 가변인자는 주로 CREATE_SMART_PTR 매크로와 create_smart_ptr() 함수에서 사용됩니다.

#### 1. 가변인자를 사용하는 함수: create_smart_ptr()
create_smart_ptr() 함수는 가변 인자를 받아, 스마트 포인터에 초기 데이터를 할당합니다. 이 함수는 스마트 포인터가 관리하는 데이터 타입에 따라 가변적으로 초기화 작업을 수행합니다.
  ```
  SmartPtr create_smart_ptr(size_t size, ...) {
    SmartPtr sp;
    sp.ptr = malloc(size);  // 메모리 할당
    sp.ref_count = (int *)malloc(sizeof(int));
    *(sp.ref_count) = 1;  // 참조 카운트 1로 초기화
  
    va_list args;
    va_start(args, size);
  
    if (size == sizeof(int)) {  // 만약 int형일 경우
        int value = va_arg(args, int);
        *(int *)sp.ptr = value;
    } else if (size == sizeof(char) * MAX_STRING_SIZE) {  // 문자열일 경우
        const char *str = va_arg(args, const char *);
        strncpy((char *)sp.ptr, str, MAX_STRING_SIZE);
    }
  
    va_end(args);
    return sp;
  }
  ```
#### 2. 가변인자의 사용 목적
이 코드에서 가변인자는 스마트 포인터가 어떤 데이터 타입을 가리키는지에 따라 초기값을 동적으로 설정하는 데 사용됩니다.

정수형 데이터와 문자열을 가변 인자로 받아 스마트 포인터의 초기값을 설정할 수 있습니다.

```
SmartPtr sp_int = CREATE_SMART_PTR(int, 42);  // int형 스마트 포인터 생성
SmartPtr sp_str = CREATE_SMART_PTR(char[MAX_STRING_SIZE], "example");  // 문자열 스마트 포인터 생성
```  

#### 3. 가변인자의 장점
  - 유연성: create_smart_ptr() 함수는 다양한 데이터 타입을 처리할 수 있습니다. 가변인자를 통해 초기화할 데이터 타입에 구애받지 않고, 스마트 포인터를 생성할 수 있습니다.
  - 초기화 자동화: 함수 호출 시 가변 인자를 넘겨 스마트 포인터의 초기값을 설정할 수 있기 때문에, 코드의 재사용성과 유지보수성이 높아집니다.
  가변인자 사용 흐름 요약:
  스마트 포인터 생성 시 가변 인자 전달: 특정 데이터 타입을 가변 인자로 넘깁니다.
  `create_smart_ptr` 함수 내에서 데이터 타입 처리: 가변 인자를 해석하여 그에 맞는 초기값을 스마트 포인터에 할당합니다.
  - 동적 데이터 초기화: 각기 다른 타입의 데이터를 스마트 포인터로 관리하며, 참조 카운트를 설정하여 메모리 관리를 용이하게 합니다.

### 마무리
메모리 스택 관리: 스마트 포인터를 통해 서버와 클라이언트는 힙 메모리에서 할당된 데이터를 안전하게 참조하고 해제합니다. 스레드는 각각의 스택 메모리를 관리하며, 다중 클라이언트 통신에서 메모리 누수를 방지합니다.
가변 인자: 가변 인자는 스마트 포인터 생성 시 다양한 타입의 데이터를 동적으로 초기화하는 데 사용되며, 코드의 유연성을 높여줍니다.
이 구조를 통해 서버-클라이언트 시스템에서 효율적인 메모리 관리와 동적 데이터 처리가 가능해집니다.


------------------

- 실행 방법 :`Makefile`을 활용하여 make을 하시오.

# 채팅 서버 및 클라이언트 실행 방법

이 프로젝트는 간단한 채팅 서버와 클라이언트 시스템을 구현합니다. 서버는 여러 클라이언트와의 연결을 관리하며, 클라이언트는 서버와 통신하여 채팅을 할 수 있습니다. 이 README 파일에서는 프로젝트를 설정하고 실행하는 방법을 설명합니다.

## 1. 프로젝트 구조
```
$ tree
.
├── client.c
├── lib
│   └── include
│       ├── ename.c.inc
│       ├── smartptr.h
│       ├── uniqueptr.h
│       └── user.h
├── Makefile
├── README.md
├── server.c
├── start_daemon.sh
└── user_data.txt
```
## 2. 사전 요구사항

- Raspberry Pi 또는 Linux 기반 시스템
- GCC 컴파일러
- 네트워크 연결

## 3. 서버 실행 방법

### 3.1 `start_daemon.sh` 스크립트 사용

서버를 백그라운드에서 실행하고 로그를 기록할 수 있습니다.

```bash
$ ./start_daemon.sh start
```

### 서버 중지
```
$ ./start_daemon.sh stop
```

### 서버 상태 확인
```
$ ./start_daemon.sh status
```

## 3.2 직접 서버 실행 (서버에서 메세지를 보내는 것이 가능합니다.)
서버를 수동으로 실행할 수도 있습니다.
```
$ ./chat_server
```

## 4. 클라이언트 실행 방법
클라이언트를 실행하여 서버에 연결하려면 다음과 같은 명령어를 사용합니다.
```
$ ./chat_client <서버 IP 주소>
예를 들어, 서버 IP 주소가 192.168.0.105이면 다음과 같이 접속 진행
$ ./chat_client 192.168.0.105
```

### 5. IP 주소 할당 방법
서버가 실행될 Raspberry Pi 또는 Linux 시스템의 IP 주소를 확인한 후 클라이언트에서 해당 IP 주소로 연결합니다.

IP 주소를 확인하려면 다음 명령어를 사용할 수 있습니다:

> $ hostname -I
위 명령어로 나온 IP 주소를 클라이언트 실행 시 사용하십시오.

서버 코드 내에서는 정적으로 IP 주소를 할당하는 부분이 있습니다. 만약 서버 IP 주소를 변경하고 싶다면, server.c 파일의 connect_to_server 함수 내에서 server_ip 값을 변경해 주세요.

- 서버 IP 주소 설정
> const char *server_ip = "192.168.0.105";  // 이 부분을 원하는 IP로 변경하세요

### 6. 로그 확인
서버와 클라이언트의 로그는 각각의 로그 파일로 기록됩니다. 서버 로그는 chat_server.log 파일에 기록되며, 다음 명령어로 로그를 실시간으로 확인할 수 있습니다.

> $ tail -f /home/pi/Desktop/veda/workspace/save/chat_server.log

## 7. 컴파일 방법
서버와 클라이언트를 컴파일하려면 Makefile을 사용합니다.
> $ make

모든 파일이 정상적으로 컴파일된 후, chat_server와 chat_client 실행 파일이 생성됩니다.
