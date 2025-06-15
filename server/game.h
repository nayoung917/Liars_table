#ifndef GAME_H
#define GAME_H

//필수 헤더
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#include "../common.h"

typedef struct {
    CardType type;
    int id;
} Card;

typedef struct {
    int socket;
    int id;
    Card hand[MAX_CARDS / MAX_PLAYERS];
    int card_count;
    int life;
    int roulette_slots;
} Player;

typedef struct {
    CardType claimed_card;
    CardType actual_card;
} PlayedCard;

typedef struct {
    int player_id;
    CardType cards[3];
    int card_count;
} Submission;

// 전역 변수들 
extern Player players[MAX_PLAYERS];
extern int player_count;
extern pthread_mutex_t lock;
extern TableType currenttable;
extern Card deck[MAX_CARDS];
extern int current_turn;
extern int active_players[MAX_PLAYERS];//생존 여부

extern Submission last_submission;
extern PlayedCard last_played_cards[3];
extern int last_played_count;
extern int last_player_id;
extern int liar_pending;
extern int liar_target;
extern int liar_escaped_win;

const char* card_type_to_str(CardType type);
void init_deck();
void shuffle_deck();
void distribute_cards();
void send_hand(int player_index);
void send_table_info(int player_index);
void notify_turn();
void set_table_type();
void process_play_command(int player_index, const char* command);
void process_liar_command(int player_index);
void accept_clients(int server_socket);
void check_player_status(int player_index);
void advanced_turn();
int init_server();
#endif
