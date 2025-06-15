#include "game.h"

void distribute_cards();
void send_table_info(int player_index);
void send_hand(int player_index);

// 카드 분배 로직
void distribute_cards()
{
    int index = 0;
    int cards_per_player = MAX_CARDS / player_count;

    for (int i = 0; i < player_count; i++)
    {
        players[i].card_count = cards_per_player;
        players[i].life = 3;

        for (int j = 0; j < cards_per_player; j++)
        {
            players[i].hand[j] = deck[index++];
        }
    }

    // 분배된 카드 목록 출력
    for (int i = 0; i < player_count; i++)
    {
        printf("Player %d 카드: ", i);
        for (int j = 0; j < players[i].card_count; j++)
        {
            switch (players[i].hand[j].type)
            {
            case KING:
                printf("KING ");
                break;
            case QUEEN:
                printf("QUEEN ");
                break;
            case ACE:
                printf("ACE ");
                break;
            case JOCKER:
                printf("JOCKER ");
                break;
            default:
                printf("? ");
            }
        }
        printf("\n");
    }
}
// 테이블 정보 출력
void send_table_info(int player_index)
{
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    switch (currenttable)
    {
    case KING_TABLE:
        strcpy(buffer, "\n 이번 턴은 KING Table입니다!\n");
        break;
    case QUEEN_TABLE:
        strcpy(buffer, "\n이번 턴은 QUEEN Table입니다!\n");
        break;
    case ACE_TABLE:
        strcpy(buffer, "\n이번 턴은 ACE Table입니다!\n");
        break;
    }

    send(players[player_index].socket, buffer, strlen(buffer), 0);
}

void send_hand(int player_index)
{
    if (player_index < 0 || player_index >= player_count)
    {
        printf("Invalid player index %d\n", player_index);
        return;
    }

    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);

    for (int i = 0; i < players[player_index].card_count; i++)
    {
        int card_type = players[player_index].hand[i].type;

        switch (card_type)
        {
        case KING:
            strcat(buffer, "KING ");
            break;
        case QUEEN:
            strcat(buffer, "QUEEN ");
            break;
        case ACE:
            strcat(buffer, "ACE ");
            break;
        case JOCKER:
            strcat(buffer, "JOCKER ");
            break;
        }
    }

    char msg[BUF_SIZE] = "HAND "; // 접두어 추가
    for (int i = 0; i < players[player_index].card_count; i++)
    {
        strcat(msg, card_type_to_str(players[player_index].hand[i].type));
        strcat(msg, " ");
    }
    strcat(msg, "\n"); // 줄바꿈 추가
    send(players[player_index].socket, msg, strlen(msg), 0);

    strcat(buffer, "\n");
    send(players[player_index].socket, buffer, strlen(buffer), 0);
}
