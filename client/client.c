#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../common.h"

int my_player_index = -1;
int my_turn = 0;
//메시지 출력
void receive_hand(int sock)
{
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    int received = recv(sock, buffer, BUF_SIZE, 0);

    if (received > 0)
    {
        buffer[received] = '\0';

        char *line = strtok(buffer, "\n");
        while (line != NULL)
        {
            if (strncmp(line, "HAND", 4) == 0)
            {
                printf("🃏내 카드: %s\n", line + 5);
                line = strtok(NULL, "\n");
                continue;
            }
            else if (strncmp(line, "REMAINING", 9) == 0)
            {
                printf("📦 남은 카드 %s\n", line + 10);
            }
            else if (strncmp(line, "YOU_ARE", 7) == 0)
            {
                my_player_index = atoi(line + 8);
                printf("🙋‍♂️당신은 Player %d입니다!\n", my_player_index);
            }
            else if (strncmp(line, "TURN", 4) == 0)
            {
                int turn = atoi(line + 5);
                if (turn == my_player_index)
                {
                    printf("🎮당신의 차례입니다! PLAY <카드>/LIAR 를 입력하세요.\n");
                    my_turn = 1;
                }
                else
                {
                    printf("⌛Player %d의 차례입니다. 잠시만 기다려주세요...\n", turn);
                    my_turn = 0;
                }
            }
            else if (strstr(line, "테이블 타입:") != NULL)
            {
                printf("--------------------------------------------\n %s\n--------------------------------------------\n", line);
            }
            else
            {
                //중복 카드 출력 제외
                if (
                    strstr(line, "KING") && strstr(line, "QUEEN") &&
                    strstr(line, "ACE") && strstr(line, "JOCKER") &&
                    strstr(line, "내 카드") == NULL)
                { //카드 구성 정보 중복 방지
                }
                else
                {
                    printf("%s\n", line); // 그 외는 정상 출력
                }
            }
            line = strtok(NULL, "\n");
        }
    }
}
//서버 연결
int connect_to_server()
{
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect() error");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("서버에 연결되었습니다.\n");
    return sock;
}

int main()
{
    int sock = connect_to_server();
    fd_set read_fds;
    char buffer[BUF_SIZE];

    printf("서버로부터 테이블 타입과 카드 분배를 수신 대기 중...\n");

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        if (select(sock + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("select() error");
            break;
        }

        // 서버로부터 메시지 수신
        if (FD_ISSET(sock, &read_fds))
        {
            receive_hand(sock);
        }

        // 사용자 입력 처리
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            memset(buffer, 0, BUF_SIZE);
            if (fgets(buffer, BUF_SIZE, stdin) != NULL)
            {
                buffer[strcspn(buffer, "\n")] = '\0'; // 개행 제거

                if (strncmp(buffer, "PLAY ", 5) == 0 || strcmp(buffer, "LIAR") == 0)
                {
                    send(sock, buffer, strlen(buffer), 0);
                }
                else
                {
                    printf("지원하지 않는 명령입니다. 예: PLAY ACE 또는 LIAR\n");
                }
            }
        }
    }

    close(sock);
    return 0;
}
