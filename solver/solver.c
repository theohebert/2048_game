#include <stdint.h>

typedef uint64_t board_t;


static inline int get_tile(board_t board, int index) {
    return (board >> (index * 4)) & 0xF;
}

static inline board_t set_tile(board_t board, int index, int value) {
    board &= ~((board_t)0xFULL << (index * 4));
    board |= ((board_t)value << (index * 4));
    return board;
}

void process_line(int line[4]) {
    int tmp[4] = {0,0,0,0};
    int pos = 0;
    // compression
    for(int i=0;i<4;i++) {
        if(line[i] != 0)
            tmp[pos++] = line[i];
    }
    // merge
    for(int i=0;i<3;i++) {
        if(tmp[i] != 0 && tmp[i] == tmp[i+1]) {
            tmp[i] += 1;   // log2 -> +1
            tmp[i+1] = 0;
        }
    }
    // compression 2
    int res[4] = {0,0,0,0};
    pos = 0;
    for(int i=0;i<4;i++) {
        if(tmp[i] != 0)
            res[pos++] = tmp[i];
    }
    for(int i=0;i<4;i++)
        line[i] = res[i];
}

board_t move_left(board_t board) {
    board_t new_board = board;
    for(int row = 0; row < 4; row++) {
        int line[4];
        for(int col = 0; col < 4; col++)
            line[col] = get_tile(board, row*4 + col);
        process_line(line);
        for(int col = 0; col < 4; col++)
            new_board = set_tile(new_board, row*4 + col, line[col]);
    }
    return new_board;
}

board_t move_right(board_t board) {
    board_t new_board = board;
    for(int row = 0; row < 4; row++) {
        int line[4];
        for(int col = 0; col < 4; col++)
            line[3-col] = get_tile(board, row*4 + col);
        process_line(line);
        for(int col = 0; col < 4; col++)
            new_board = set_tile(new_board, row*4 + col, line[3-col]);
    }
    return new_board;
}

board_t move_up(board_t board) {
    board_t new_board = board;
    for(int col = 0; col < 4; col++) {
        int line[4];
        for(int row = 0; row < 4; row++)
            line[row] = get_tile(board, row*4 + col);
        process_line(line);
        for(int row = 0; row < 4; row++)
            new_board = set_tile(new_board, row*4 + col, line[row]);
    }
    return new_board;
}

board_t move_down(board_t board) {
    board_t new_board = board;
    for(int col = 0; col < 4; col++) {
        int line[4];
        for(int row = 0; row < 4; row++)
            line[3-row] = get_tile(board, row*4 + col);
        process_line(line);
        for(int row = 0; row < 4; row++)
            new_board = set_tile(new_board, row*4 + col, line[3-row]);
    }
    return new_board;
}

double expectimax(board_t board, int depth){
    // Cette fonction doit être implémentée pour calculer la valeur expectimax d'un plateau de jeu donné.
    // Le paramètre 'board' représente l'état actuel du plateau de jeu sous forme de bitboard.
    // Le paramètre 'depth' indique la profondeur maximale de l'exploration de l'arbre expectimax.
    // La fonction doit retourner une valeur double représentant l'évaluation du plateau de jeu.

    if(depth == 0) {
        return evaluate(board);
    }

    double max_score = -1e18;

    for(int move = 0; move < 4; move++) {
        board_t new_board;
        if(move == 0) new_board = move_up(board);
        if(move == 1) new_board = move_down(board);
        if(move == 2) new_board = move_left(board);
        if(move == 3) new_board = move_right(board);

        if(new_board == board) continue;

        double score = expectimax(new_board, depth - 1);
        if(score > max_score) {
            max_score = score;
        }
    }

    return max_score;
}

int best_move(board_t board) {
    // Cette fonction doit être implémentée pour calculer le meilleur coup à jouer
    // en fonction de l'état actuel du plateau de jeu représenté par le bitboard.
    // Le bitboard est une représentation compacte du plateau de jeu, où chaque tile est codée sur 4 bits.
    // La fonction doit retourner un entier représentant la direction du mouvement (0: haut, 1: bas, 2: gauche, 3: droite).

    double best_score = -1e18;
    int best = 0;

    for(int move = 0; move < 4; move++) {
        board_t new_board;
        if(move == 0) new_board = move_up(board);
        if(move == 1) new_board = move_down(board);
        if(move == 2) new_board = move_left(board);
        if(move == 3) new_board = move_right(board);

        if(new_board == board) continue;

        double score = expectimax(new_board, 4);
        if(score > best_score) {
            best_score = score;
            best = move;
        }
    }

    return best;
}