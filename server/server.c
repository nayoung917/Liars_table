#include "game.h"

// 게임 흐름 제어 함수 선언
void *handle_client(void *arg);

int init_server();
void accept_clients(int server_socket);
// 서버 생성
int init_server()
{
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() error");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_PLAYERS) == -1)
    {
        perror("listen() error");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server started. Waiting for players...\n");
    return server_socket;
}
// 클라이언트 접속
void accept_clients(int server_socket)
{
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);

    //최대 플레이어 수만큼만 접속(4명)
    //MAX_PLAYERS 보다 크거나 같아지면 루프 종료-> 이후 접속 자동 차단
    while (player_count < MAX_PLAYERS)
    {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket == -1)
        {
            perror("accept() error");
            continue;
        }

        pthread_mutex_lock(&lock);
        players[player_count].socket = client_socket;
        players[player_count].id = player_count;
        players[player_count].roulette_slots = 4;
        player_count++;
        printf("Player %d connected. 현재 접속 인원: %d\n", player_count - 1, player_count);
        pthread_mutex_unlock(&lock);
    }

    // 모든 플레이어 접속 후 카드 분배 및 테이블 타입 설정
    distribute_cards();
    set_table_type();

    for (int i = 0; i < player_count; i++)
    {
        send_hand(i); //카드 전송

        char info[32];
        snprintf(info, sizeof(info), "YOU_ARE %d\n", i);
        send(players[i].socket, info, strlen(info), 0);

        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, arg);
        pthread_detach(tid); // 종료 시 자동 회수
    }
    char roulette_info[BUF_SIZE];
    snprintf(roulette_info, sizeof(roulette_info), " 러시안 룰렛 안내: 실린더에는 총 4개의 칸이 있습니다.\n");
    for (int i = 0; i < player_count; i++)
    {
        send(players[i].socket, roulette_info, strlen(roulette_info), 0);
    }
    notify_turn();
    //LAIR 관련 상태 초기화
    liar_pending = 0;
    liar_target = -1;
}
