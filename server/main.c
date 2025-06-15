#include "game.h"

// 전역 변수 정의
Player players[MAX_PLAYERS];
int player_count = 0;
pthread_mutex_t lock;
TableType currenttable;
Card deck[MAX_CARDS];
int current_turn = 0;

Submission last_submission;
PlayedCard last_played_cards[3];
int last_played_count = 0;
int last_player_id = -1;
int active_players[MAX_PLAYERS] = {1, 1, 1, 1};

// 필요한 함수 선언
int init_server();
void accept_clients(int server_socket);
void init_deck();
void shuffle_deck();

int main()
{
    srand(time(NULL));
    pthread_mutex_init(&lock, NULL);

    init_deck();
    shuffle_deck();
    srand(time(NULL));

    int server_socket = init_server();
    accept_clients(server_socket); // 여기서 핸들러 스레드 생성

    while (1)
    {
        sleep(1);
    }

    close(server_socket);
    pthread_mutex_destroy(&lock);
    return 0;
}
