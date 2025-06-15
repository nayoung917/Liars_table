#include "game.h"

//덱 초기화 
void init_deck() {
    int index = 0;
    // KING 8장
    for (int i = 0; i < 8; i++) {
        deck[index++] = (Card){KING, index};
    }
    // QUEEN 8장
    for (int i = 0; i < 8; i++) {
        deck[index++] = (Card){QUEEN, index};
    }
    // ACE 8장
    for (int i = 0; i < 8; i++) {
        deck[index++] = (Card){ACE, index};
    }
    // JOCKER 4장
    for (int i = 0; i < 4; i++) {
        deck[index++] = (Card){JOCKER, index};
    }
    printf("덱이 초기화되었습니다. 총 카드 수: %d\n", index);
}

//카드 섞기기
void shuffle_deck() {
    for (int i = 0; i < MAX_CARDS; i++) {
        int rand_idx = rand() % MAX_CARDS;
        Card temp = deck[i];
        deck[i] = deck[rand_idx];
        deck[rand_idx] = temp;
    }
    printf("덱이 섞였습니다.\n");
}
