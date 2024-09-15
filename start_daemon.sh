#!/bin/bash
# start_chat_server.sh

SERVER_PATH="/home/ubuntu/Desktop/workspace/exam_chat_mutex_server/save"
LOG_FILE="/home/ubuntu/Desktop/workspace/exam_chat_mutex_server/save/chatlog_$(date +'%Y%m%d').log"

start() {
    # SERVER_PATH 확인
    if [ ! -d "$SERVER_PATH" ]; then
        echo "Error: SERVER_PATH ($SERVER_PATH) 경로가 존재하지 않습니다. 올바른 경로를 등록해주세요."
        exit 1
    fi

    # LOG_FILE의 디렉토리 확인
    LOG_DIR=$(dirname "$LOG_FILE")
    if [ ! -d "$LOG_DIR" ]; then
        echo "Error: 로그 파일 경로 ($LOG_DIR) 가 존재하지 않습니다. 올바른 경로를 등록해주세요."
        exit 1
    fi

    echo "Starting chat server..."
    # nohup으로 백그라운드 실행 시 stdin을 /dev/null로 연결하여 입력 대기 없앰
    nohup $SERVER_PATH/chat_server > $LOG_FILE 2>&1 < /dev/null &
    echo "Chat server started. Log is being written to $LOG_FILE"
}

stop() {
    echo "Stopping chat server..."
    pkill -f chat_server
    echo "Chat server stopped."
}

restart() {
    stop
    start
}

status() {
    if pgrep -f chat_server > /dev/null
    then
        echo "Chat server is running."
    else
        echo "Chat server is not running."
    fi
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    status)
        status
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac
exit 0
