#ifndef COMMON_H
#define COMMON_H

#define PORT 9000
#define BUF_SIZE 512
#define MAX_PLAYERS 4 //최대 플레이어
#define MAX_CARDS 28 // 전체 카드

typedef enum {KING, QUEEN, ACE, JOCKER} CardType;

typedef enum {KING_TABLE, QUEEN_TABLE, ACE_TABLE} TableType;

#endif // COMMON_H
