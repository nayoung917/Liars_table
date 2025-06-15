#include "game.h"

const char* card_type_to_str(CardType type) {
    switch (type) {
        case KING: return "KING";
        case QUEEN: return "QUEEN";
        case ACE: return "ACE";
        case JOCKER: return "JOCKER";
        default: return "?";
    }
}