# pragma once

#include "constants.h"

namespace zobrist {

    void initialize();

    void updateKeyPiece(Key& key, int piece, int square);
    void updateKeySide(Key& key);
    void updateKeyCastle(Key& key, int castlingRights);
    void updateKeyEp(Key& key, int square);

}