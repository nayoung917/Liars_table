#include "game.h"

void set_table_type();
void *handle_client(void *arg);
void process_play_command(int player_index, const char *command);
void process_liar_command(int player_index);
void advanced_turn();
void notify_turn();

// 게임 상태 관련 전역변수들
int liar_pending = 0;     // 1이면 LIAR 선언 가능한 상태
int liar_target = -1;     // 거짓말 의심 대상
int liar_escaped_win = 0; // 카드 0장으로 이긴 사람 ID, LIAR 기회가 끝난 후에만 승리 처리
int game_started = 0;     // 게임 시작 여부
int liar_win_ready = 0;

// 랜덤 테이블 타입 설정
void set_table_type()
{
    int random = rand() % 3; // 0 ~ 2
    currenttable = (TableType)random;

    const char *type_str;
    switch (currenttable)
    {
    case KING_TABLE:
        type_str = "KING";
        break;
    case QUEEN_TABLE:
        type_str = "QUEEN";
        break;
    case ACE_TABLE:
        type_str = "ACE";
        break;
    }

    // 서버 콘솔 출력
    printf("\n 이번 턴은 %s's Table입니다!\n", type_str);

    // 클라이언트에게도 전송
    char table_msg[BUF_SIZE];
    snprintf(table_msg, sizeof(table_msg),
             " 이번 턴의 테이블 타입: %s Table입니다!\n",
             type_str);

    for (int i = 0; i < player_count; i++)
    {
        send(players[i].socket, table_msg, strlen(table_msg), 0);
    }
}
// 스레드 함수/ 명령수신 처리(PLAY,LIAR)
void *handle_client(void *arg)
{
    int player_index = *(int *)arg;
    free(arg);

    char buffer[BUF_SIZE];
    int received;
    char roulette_intro[BUF_SIZE];

    send(players[player_index].socket, roulette_intro, strlen(roulette_intro), 0);
    // 클라이언트로부터 명령 수신 루프
    while ((received = recv(players[player_index].socket, buffer, BUF_SIZE, 0)) > 0)
    {
        buffer[received] = '\0';

        // 현재 차례 아닌 경우
        if (player_index != current_turn)
        {
            send(players[player_index].socket, "아직 당신의 차례가 아닙니다!\n", 42, 0);
            continue;
        }

        if (strncmp(buffer, "PLAY", 4) == 0)
        {
            process_play_command(player_index, buffer);
        }
        else if (strncmp(buffer, "LIAR", 4) == 0)
        {
            process_liar_command(player_index); // 새로 추가된 LIAR 처리 함수 호출
        }
        else
        {
            // 잘못된 명령어 처리
            printf("Player %d: %s\n", player_index, buffer);
            send(players[player_index].socket, "지원하지 않는 명령어입니다. PLAY 또는 LIAR를 입력하세요.\n", BUF_SIZE, 0); // PLAY나 LIAR가 아닌 명령어 입력 예외 처리
        }
    }

    // 연결 종료 처리
    close(players[player_index].socket);
    pthread_mutex_lock(&lock);
    // 해당 플레이어를 탈락 처리
    players[player_index].life = 0;
    active_players[player_index] = 0;

    char dc_msg[BUF_SIZE];
    snprintf(dc_msg, sizeof(dc_msg), "Player %d의 연결이 끊겼습니다. 해당 플레이어는 탈락 처리됩니다. \n", player_index);
    for (int i = 0; i < player_count; i++)
    {
        if (i != player_index && active_players[i])
            send(players[i].socket, dc_msg, strlen(dc_msg), 0);
    }
    pthread_mutex_unlock(&lock);

    // 턴 중에 나간 경우 다음 턴 넘기기
    if (player_index == current_turn)
    {
        advanced_turn();
        notify_turn();
    }

    check_player_status(player_index);

    return NULL;
}
// 카드 제출 로직
void process_play_command(int player_index, const char *command)
{
    if (liar_pending && game_started)
    {
        // liar_target 유효성 먼저 확인

        if (players[liar_target].card_count == 0 &&
            players[liar_target].life > 0 &&
            active_players[liar_target])
        {
            liar_escaped_win = liar_target; // 우승 후보 저장
            liar_pending = 0;
            liar_target = -1;
            last_player_id = -1;
            last_played_count = 0;
        }
    }
    // 카드 파싱
    char card_strs[3][16];
    char extra[16]; // 4번째 카드가 있는지 체크
    int count = sscanf(command, "PLAY %s %s %s %s", card_strs[0], card_strs[1], card_strs[2], extra);
    if (count < 1 || count > 3)
    {
        send(players[player_index].socket, "카드는 1장 이상 3장 이하로 제출해야 합니다.\n", 60, 0);
        return;
    }
    // 순서 검증
    if (player_index != current_turn)
    {
        send(players[player_index].socket, "아직 당신의 차례가 아닙니다!\n", 50, 0);
        return;
    }
    // 카드 모두 제출한 사람의 마지막 제출 카드 검증
    if (liar_pending)
    {
        if (players[liar_target].card_count == 0 && players[liar_target].life > 0 && active_players[liar_target])
        {
            liar_escaped_win = liar_target; // 일단 승리 후보로 보관
        }
        liar_pending = 0;
        liar_target = -1;
        last_player_id = -1;
        last_played_count = 0;
    }

    CardType submitted[3];
    for (int i = 0; i < count; i++)
    {
        if (strcmp(card_strs[i], "KING") == 0)
            submitted[i] = KING;
        else if (strcmp(card_strs[i], "QUEEN") == 0)
            submitted[i] = QUEEN;
        else if (strcmp(card_strs[i], "ACE") == 0)
            submitted[i] = ACE;
        else if (strcmp(card_strs[i], "JOCKER") == 0)
            submitted[i] = JOCKER;
        else
        {
            send(players[player_index].socket, "유효하지 않은 카드 타입입니다.\n", 60, 0);
            return;
        }
    }
    // 유효성 검사 및 hand에서 제거
    for (int i = 0; i < count; i++)
    {
        int found = -1;
        for (int j = 0; j < players[player_index].card_count; j++)
        {
            if (players[player_index].hand[j].type == submitted[i])
            {
                found = j;
                break;
            }
        }
        if (found == -1)
        {
            send(players[player_index].socket, "제출한 카드가 손에 없습니다.\n", 60, 0);
            return;
        }
        // hand에서 제거 (순서 유지)
        for (int j = found; j < players[player_index].card_count - 1; j++)
        {
            players[player_index].hand[j] = players[player_index].hand[j + 1];
        }
        players[player_index].card_count--;
    }
    // 서버 로그
    printf("[서버] Player %d가 제출한 카드: ", player_index);
    for (int i = 0; i < count; i++)
    {
        printf("%s ", card_type_to_str(submitted[i]));
    }
    printf("\n");
    // 클라이언트에는 카드 내용 없이 전송
    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "Player %d이 카드를 %d장 제출했습니다.\n", player_index, count);
    for (int i = 0; i < player_count; i++)
    {
        send(players[i].socket, msg, strlen(msg), 0);
    }
    // 남은 카드 출력
    char hand_msg[BUF_SIZE];
    snprintf(hand_msg, sizeof(hand_msg), "남은 카드 (%d장): ", players[player_index].card_count);
    for (int i = 0; i < players[player_index].card_count; i++)
    {
        strcat(hand_msg, card_type_to_str(players[player_index].hand[i].type));
        strcat(hand_msg, " ");
    }
    strcat(hand_msg, "\n");
    send(players[player_index].socket, hand_msg, strlen(hand_msg), 0);

    // 마지막 제출 정보 저장
    last_submission.player_id = player_index;
    last_submission.card_count = count;
    for (int i = 0; i < count; i++)
    {
        last_submission.cards[i] = submitted[i];
        last_played_cards[i].claimed_card = submitted[i];
        last_played_cards[i].actual_card = submitted[i];
    }

    last_player_id = player_index;
    last_played_count = count;
    check_player_status(player_index);
    advanced_turn();
    notify_turn();
}
// 라이어 로직
void process_liar_command(int player_index)
{
    if (player_index != current_turn)
    {
        send(players[player_index].socket, "당신의 턴이 아닙니다. LIAR 명령은 자신의 턴에만 사용할 수 있습니다.\n", BUF_SIZE, 0);
        return;
    }

    if (last_player_id == -1 || last_played_count == 0)
    {
        send(players[player_index].socket, "라이어 선언 대상이 없습니다. 직전 플레이어가 카드를 낸 적이 없습니다.\n", BUF_SIZE, 0);
        return;
    }
    // 직전 플레이어의 제출 카드 정보 확인
    int mismatch_found = 0;
    for (int i = 0; i < last_played_count; i++)
    {
        CardType actual = last_played_cards[i].actual_card;

        // JOCKER는 어떤 카드로도 주장 가능
        if (actual == JOCKER)
            continue;

        if (actual != (CardType)currenttable)
        {
            mismatch_found = 1;
            break;
        }
    }

    int roulette = rand() % 4; // 0이면 죽음 (25%)
    int victim = mismatch_found ? last_player_id : player_index;
    Player *target = &players[victim];
    // 결과 메시지 준비
    char result_msg[BUF_SIZE * 2];
    char info_msg[BUF_SIZE];
    char roulette_msg[BUF_SIZE];
    char shot_msg[BUF_SIZE];
    char card_list_msg[BUF_SIZE] = "실제 제출 카드: ";
    memset(result_msg, 0, sizeof(result_msg));

    strcpy(card_list_msg, " Player ");
    char temp[16];
    sprintf(temp, "%d", last_player_id);
    strcat(card_list_msg, temp);
    strcat(card_list_msg, "의 실제 제출 카드: ");
    for (int i = 0; i < last_played_count; i++)
    {
        switch (last_played_cards[i].actual_card)
        {
        case KING:
            strcat(card_list_msg, "KING ");
            break;
        case QUEEN:
            strcat(card_list_msg, "QUEEN ");
            break;
        case ACE:
            strcat(card_list_msg, "ACE ");
            break;
        case JOCKER:
            strcat(card_list_msg, "JOCKER ");
            break;
        }
    }
    strcat(card_list_msg, "\n");

    // 라이어 여부 안내
    if (mismatch_found)
    {
        snprintf(info_msg, sizeof(info_msg), " Player %d의 거짓말을 간파했습니다!\n", last_player_id);
    }
    else
    {
        snprintf(info_msg, sizeof(info_msg), " Player %d은 거짓말을 하지 않았습니다!\n", last_player_id);
    }
    // 러시안 룰렛 안내 메시지
    snprintf(roulette_msg, sizeof(roulette_msg),
             " Player %d에게 러시안 룰렛이 발동됩니다...\n", victim);
    // 메시지 전송
    for (int i = 0; i < player_count; i++)
    {
        send(players[i].socket, card_list_msg, strlen(card_list_msg), 0);
        send(players[i].socket, info_msg, strlen(info_msg), 0);
        send(players[i].socket, roulette_msg, strlen(roulette_msg), 0);
    }
    // 3초 대기
    sleep(3);
    // 러시안 룰렛 발사 결과
    if (roulette == 0 || target->roulette_slots == 1)
    {
        target->life = 0;
        snprintf(shot_msg, sizeof(shot_msg),
                 " 탕! 러시안 룰렛이 발사되었습니다!\n");
        if (victim == liar_escaped_win)
        {
            liar_escaped_win = -1;
        }
        send(players[victim].socket, shot_msg, strlen(shot_msg), 0);
    }
    else
    {
        target->roulette_slots--; // 여기서 먼저 줄인다!

        snprintf(shot_msg, sizeof(shot_msg),
                 "...아무 일도 일어나지 않았습니다.\n Player %d이 살아남았습니다!\n", victim);

        // 남은 실린더 출력
        char cylinder_state[5] = "OOOO"; // 4칸 기준
        cylinder_state[target->roulette_slots] = '\0';

        char cylinder_msg[BUF_SIZE];
        snprintf(cylinder_msg, sizeof(cylinder_msg), "남은 실린더 : %s\n", cylinder_state);
        send(players[victim].socket, cylinder_msg, strlen(cylinder_msg), 0);
    }

    for (int i = 0; i < player_count; i++)
    {
        send(players[i].socket, shot_msg, strlen(shot_msg), 0);
    }
    check_player_status(victim);
    advanced_turn();
    notify_turn();

    // 제출 기록 초기화
    last_player_id = -1;
    last_played_count = 0;
}
// 턴 넘기기
void advanced_turn()
{
    int original = current_turn;
    do
    {
        current_turn = (current_turn + 1) % player_count;
    } while (active_players[current_turn] == 0 && current_turn != original);
    printf(" 다음 턴: Player %d\n", current_turn);
}
// 턴 알리기
void notify_turn()
{
    // 우승 조건 확인
    if (liar_escaped_win != -1 && liar_win_ready)
    {
        if (players[liar_escaped_win].life > 0 &&
            active_players[liar_escaped_win] &&
            players[liar_escaped_win].card_count == 0)
        {
            char win_msg[BUF_SIZE];
            snprintf(win_msg, sizeof(win_msg), " Player %d이 모든 카드를 제출하고 살아남아 승리했습니다!\n", liar_escaped_win);
            for (int i = 0; i < player_count; i++)
            {
                send(players[i].socket, win_msg, strlen(win_msg), 0);
            }
            sleep(3);
            exit(0);
        }
        liar_escaped_win = -1;
        liar_win_ready = 0;
    }
    else if (liar_escaped_win != -1 && !liar_win_ready)
    {
        liar_win_ready = 1;
    }
    char msg[BUF_SIZE];
    snprintf(msg, sizeof(msg), "TURN %d\n", current_turn);

    for (int i = 0; i < player_count; i++)
    {
        send(players[i].socket, msg, strlen(msg), 0);
        if (i == current_turn)
        {
            // 내 턴이면 남은 카드 보여주기
            char hand_msg[BUF_SIZE];
            snprintf(hand_msg, sizeof(hand_msg), "REMAINING (%d장): ", players[i].card_count);
            for (int j = 0; j < players[i].card_count; j++)
            {
                switch (players[i].hand[j].type)
                {
                case KING:
                    strcat(hand_msg, "KING ");
                    break;
                case QUEEN:
                    strcat(hand_msg, "QUEEN ");
                    break;
                case ACE:
                    strcat(hand_msg, "ACE ");
                    break;
                case JOCKER:
                    strcat(hand_msg, "JOCKER ");
                    break;
                }
            }
            strcat(hand_msg, "\n");
            send(players[i].socket, hand_msg, strlen(hand_msg), 0);
        }
    }
    game_started = 1;
}
// 플레이어 생존 여부
void check_player_status(int player_index)
{
    // 탈락 처리
    if (players[player_index].life <= 0)
    {
        active_players[player_index] = 0;
        char msg[BUF_SIZE];
        snprintf(msg, sizeof(msg), " Player %d이 탈락했습니다!\n", player_index);
        for (int i = 0; i < player_count; i++)
        {
            send(players[i].socket, msg, strlen(msg), 0);
        }
        printf("[서버] 현재 생존 플레이어: ");
        for (int i = 0; i < player_count; i++)
        {
            if (active_players[i])
                printf("%d ", i);
        }
        printf("\n");
    }
    // 생존자 수 확인
    int alive_count = 0;
    int winner = -1;
    for (int i = 0; i < player_count; i++)
    {
        if (active_players[i])
        {
            alive_count++;
            winner = i;
        }
    }
    // 최종 1인 생존 시 게임 종료
    if (alive_count == 1)
    {
        char end_msg[BUF_SIZE];
        snprintf(end_msg, sizeof(end_msg), " 최종 승자: Player %d! 게임 종료!\n", winner);
        for (int i = 0; i < player_count; i++)
        {
            send(players[i].socket, end_msg, strlen(end_msg), 0);
        }
        sleep(3);
        exit(0);
    }
    // 카드 0장인 생존자 승리 처리
    if (game_started)
    {
        for (int i = 0; i < player_count; i++)
        {
            if (active_players[i] && players[i].card_count == 0)
            {
                liar_escaped_win = i; // 바로 등록
                return;
            }
        }
    }
}
