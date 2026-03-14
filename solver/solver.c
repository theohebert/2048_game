
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

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

int count_empty(board_t board) {
    int count = 0;
    for(int i = 0; i < 16; i++) {
        if(get_tile(board, i) == 0)
            count++;
    }
    return count;
}

double smoothness(board_t board) {
    double score = 0;

    for(int i = 0; i < 16; i++) {
        int tile = get_tile(board, i);
        if(tile == 0) continue;

        int x = i % 4;
        int y = i / 4;

        if(x < 3) {
            int right = get_tile(board, i+1);
            if(right != 0)
                score -= (tile - right) * (tile - right);
                
        }
        if(y < 3) {
            int down = get_tile(board, i+4);
            if(down != 0)
                score -= (tile - down) * (tile - down);
        }
    }
    return score;
}

double monotonicity(board_t board) {
    double totals[4] = {0,0,0,0};
    for(int y=0;y<4;y++) {
        for(int x=0;x<3;x++) {
            int a = get_tile(board, y*4 + x);
            int b = get_tile(board, y*4 + x + 1);
            if(a != 0 && b != 0){
                if(a > b)
                    totals[0] += b - a;
                else
                    totals[1] += a - b;
            }
        }
    }
    for(int x=0;x<4;x++) {
        for(int y=0;y<3;y++) {
            int a = get_tile(board, y*4 + x);
            int b = get_tile(board, (y+1)*4 + x);
            if(a != 0 && b != 0){                
                if(a > b)
                    totals[2] += b - a;
                else
                    totals[3] += a - b;
            }
        }
    }
    return fmax(totals[0], totals[1]) + fmax(totals[2], totals[3]);
}

double corner_bonus(board_t board, int max) {
    int corners[4] = {0,3,12,15};
    for(int i=0;i<4;i++) {
        if(get_tile(board,corners[i]) == max)
            return max;
    }
    return 0;
}

int max_tile(board_t board) {
    int max = 0;
    for(int i=0;i<16;i++) {
        int tile = get_tile(board,i);
        if(tile > max)
            max = tile;
    }
    return max;
}

double snake(board_t board) {
    int weights[16] = {
        65536, 32768, 16384, 8192,
        512,   1024,  2048,  4096,
        256,   128,   64,    32,
        2,     4,     8,     16
    };
    double score = 0;
    for(int i=0;i<16;i++) {
        int tile = get_tile(board,i);
        if(tile != 0)
            score += tile * weights[i];;
    }
    return score;
}

double values(board_t board) {
    double score = 0;
    for(int i=0;i<16;i++) {
        int tile = get_tile(board,i);
        if(tile != 0)
            score += tile * tile; // valorise les tuiles élevées
    }
    return score;
}

double evaluate(board_t board) {
    int empty = count_empty(board);
    double smooth = smoothness(board);
    double mono = monotonicity(board);
    int max = max_tile(board);
    double corner = corner_bonus(board, max);
    double snake_score = snake(board);

    double score = 0.0;

    score += empty * 270.0;          // tuiles vides
    score += smooth * 7.0;           // smoothness
    score += mono * 50.0;            // monotonicité
    score += max * 2000;      // max tile fortement valorisée (on utilise la log2 du tile pour éviter les grands écarts)
    score += corner * 500;        // coin bonus renforcé
    score += snake_score * 0.1;     // snake pattern
    score += values(board) * 0.1;   // valeur des tuiles

    return score;
}

double expectimax(board_t board, int depth, int is_player) {
    if(depth == 0) {
        return evaluate(board);
    }

    if(is_player) {
        double max_score = -1e18;
        int valid_move = 0;

        for(int move = 0; move < 4; move++) {
            board_t new_board;
            if(move == 0) new_board = move_up(board);
            if(move == 1) new_board = move_down(board);
            if(move == 2) new_board = move_left(board);
            if(move == 3) new_board = move_right(board);

            if(new_board == board) continue;
            valid_move = 1;
            double score = expectimax(new_board, depth - 1, 0);
            if(score > max_score) {
                max_score = score;
            }
        }
        if(!valid_move)
            return evaluate(board)-10000; // pénalité pour les positions sans coup possible
        return max_score;
    }
    else {
        double total = 0.0;
        int empty = count_empty(board);

        if(empty == 0)
            return evaluate(board)-10000; // pénalité pour les positions sans coup possible
        for(int i = 0; i < 16; i++) {
            if(get_tile(board, i) == 0) {
                
                board_t board2 = board | ((board_t)1 << (i*4));
                board_t board4 = board | ((board_t)2 << (i*4));
                total += 0.9 * expectimax(board2, depth-1, 1);
                total += 0.1 * expectimax(board4, depth-1, 1);
            }
        }
        
        return total / empty;
    }
    
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
        
        int empty = count_empty(new_board);
        int depth;
        if(empty >= 8) depth = 4;
        else if(empty >= 6) depth = 5;
        else if(empty >= 4) depth = 6;
        else depth = 7;

        double score = expectimax(new_board, depth, 0); // Profondeur de 4 pour l'exploration
        if(score > best_score) {
            best_score = score;
            best = move;
        }
    }

    return best;
}