
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef uint64_t board_t;

#define CACHE_SIZE (1 << 22) // ~4M entrées, ajustable
#define CACHE_MASK (CACHE_SIZE - 1)

typedef struct {
    uint64_t key;
    double value;
    uint8_t valid;
} cache_entry_t;

typedef struct {
    double empty_weight;
    double smooth_weight;
    double mono_weight;
    double corner_weight;
    double gradient_weight;
    double merge_weight;
} weights_t;

static cache_entry_t cache[CACHE_SIZE];
static uint16_t move_table[65536]; // ligne après mouvement gauche
static uint16_t move_table_right[65536];

static int initialized = 0;



void cache_clear() {
    memset(cache, 0, sizeof(cache));
}

int cache_get(uint64_t key, double *value) {
    uint32_t idx = (uint32_t)(key & CACHE_MASK);
    if(cache[idx].valid && cache[idx].key == key) {
        *value = cache[idx].value;
        return 1;
    }
    return 0;
}

int safe_cache_get(uint64_t key, double *value) {
    int found;

    pthread_mutex_lock(&cache_mutex);
    found = cache_get(key, value);
    pthread_mutex_unlock(&cache_mutex);

    return found;
}

void cache_set(uint64_t key, double value) {
    uint32_t idx = (uint32_t)(key & CACHE_MASK);
    cache[idx].key = key;
    cache[idx].value = value;
    cache[idx].valid = 1;
}

void safe_cache_set(uint64_t key, double value) {
    pthread_mutex_lock(&cache_mutex);
    cache_set(key, value);
    pthread_mutex_unlock(&cache_mutex);
}


uint16_t process_line(uint16_t packed) {
    int line[4] = {
        (packed >> 0)  & 0xF,
        (packed >> 4)  & 0xF,
        (packed >> 8)  & 0xF,
        (packed >> 12) & 0xF
    };
    // compression
    int tmp[4] = {0,0,0,0};
    int pos = 0;
    for(int i = 0; i < 4; i++)
        if(line[i] != 0)
            tmp[pos++] = line[i];
    // merge
    for(int i = 0; i < 3; i++) {
        if(tmp[i] != 0 && tmp[i] == tmp[i+1]) {
            tmp[i] += 1;
            tmp[i+1] = 0;
        }
    }
    // compression 2
    int res[4] = {0,0,0,0};
    pos = 0;
    for(int i = 0; i < 4; i++)
        if(tmp[i] != 0)
            res[pos++] = tmp[i];
    return res[0] | (res[1] << 4) | (res[2] << 8) | (res[3] << 12);
}


void init_move_tables() {
    for(int i = 0; i < 65536; i++) {
        move_table[i] = process_line((uint16_t)i);

        uint16_t rev = ((i & 0xF000) >> 12)
                     | ((i & 0x0F00) >> 4)
                     | ((i & 0x00F0) << 4)
                     | ((i & 0x000F) << 12);
        uint16_t res = process_line(rev);
        move_table_right[i] = ((res & 0xF000) >> 12)
                             | ((res & 0x0F00) >> 4)
                             | ((res & 0x00F0) << 4)
                             | ((res & 0x000F) << 12);
    }
}

void solver_init() {
    if(initialized) return;
    init_move_tables();
    initialized = 1;
}

static inline int get_tile(board_t board, int index) {
    return (board >> (index * 4)) & 0xF;
}

static inline board_t set_tile(board_t board, int index, int value) {
    board &= ~((board_t)0xFULL << (index * 4));
    board |= ((board_t)value << (index * 4));
    return board;
}

/*
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
*/
board_t move_left(board_t board) {
    board_t result = 0;
    for(int row = 0; row < 4; row++) {
        uint16_t line = (board >> (row * 16)) & 0xFFFF;
        uint16_t moved = move_table[line];
        result |= (board_t)moved << (row * 16);
    }
    return result;
}

board_t move_right(board_t board) {
    board_t result = 0;
    for(int row = 0; row < 4; row++) {
        uint16_t line = (board >> (row * 16)) & 0xFFFF;
        uint16_t moved = move_table_right[line];
        result |= (board_t)moved << (row * 16);
    }
    return result;
}


board_t transpose(board_t x) {
    board_t a1 = x & 0xF0F00F0FF0F00F0FULL;
    board_t a2 = x & 0x0000F0F00000F0F0ULL;
    board_t a3 = x & 0x0F0F00000F0F0000ULL;
    board_t a  = a1 | (a2 << 12) | (a3 >> 12);
    board_t b1 = a & 0xFF00FF0000FF00FFULL;
    board_t b2 = a & 0x00FF00FF00000000ULL;
    board_t b3 = a & 0x00000000FF00FF00ULL;
    return b1 | (b2 >> 24) | (b3 << 24);
}

board_t move_up(board_t board) {
    board_t t = transpose(board);
    board_t moved = move_left(t);
    return transpose(moved);
}

board_t move_down(board_t board) {
    board_t t = transpose(board);
    board_t moved = move_right(t);
    return transpose(moved);
}


// bit tricks pour compter les cases vides plus rapidement qu'une boucle sur les 16 cases
int count_empty(board_t x) {
    x |= (x >> 2) & 0x3333333333333333ULL;
    x |= (x >> 1);
    x = ~x & 0x1111111111111111ULL;
    return __builtin_popcountll(x);
}
double count_empty_normalized(board_t board) {
    int empty = count_empty(board); // Ta fonction bit-trick actuelle
    if (empty == 0) return 0.0;
    // log(1) = 0, log(16) = 2.77
    // On normalise pour que le résultat soit entre 0 et 1
    return log(empty) / log(16); 
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
double smoothness_normalized(board_t board) {
    double penalty = 0;
    
    for (int i = 0; i < 16; i++) {
        int val = get_tile(board, i);
        if (val == 0) continue;

        int x = i % 4;
        int y = i / 4;

        // Voisin de droite
        if (x < 3) {
            int right = get_tile(board, i + 1);
            if (right != 0) {
                // On soustrait la différence absolue des rangs
                penalty += abs(val - right);
            }
        }
        
        // Voisin du bas
        if (y < 3) {
            int down = get_tile(board, i + 4);
            if (down != 0) {
                penalty += abs(val - down);
            }
        }
    }

    /* Normalisation : 
       Il y a 24 paires de voisins possibles au total. 
       Diviser par 24 ramène le score à une "rugosité moyenne par voisin".
       Le signe négatif transforme la pénalité en un score à maximiser.
    */
    return -penalty / 24.0;
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
    return fmin(totals[0], totals[1]) + fmin(totals[2], totals[3]);
}
double monotonicity_normalized(board_t board) {
    double total_score = 0;

    // Analyse des lignes (Horizontale)
    for (int y = 0; y < 4; y++) {
        double current_inc = 0;
        double current_dec = 0;
        for (int x = 0; x < 3; x++) {
            int val1 = get_tile(board, y * 4 + x);
            int val2 = get_tile(board, y * 4 + x + 1);
            
            if (val1 > val2) current_inc += (val1 - val2);
            else current_dec += (val2 - val1);
        }
        // On ne garde que la direction la plus "respectée"
        total_score += fmax(current_inc, current_dec);
    }

    // Analyse des colonnes (Verticale)
    for (int x = 0; x < 4; x++) {
        double current_inc = 0;
        double current_dec = 0;
        for (int y = 0; y < 3; y++) {
            int val1 = get_tile(board, y * 4 + x);
            int val2 = get_tile(board, (y + 1) * 4 + x);
            
            if (val1 > val2) current_inc += (val1 - val2);
            else current_dec += (val2 - val1);
        }
        total_score += fmax(current_inc, current_dec);
    }

    /* Normalisation :
       Sur un plateau très avancé, la somme des différences de rangs 
       peut monter assez haut. 150.0 est un diviseur empirique pour 
       maintenir le score dans une plage proche de [0, 1].
    */
    return total_score / 150.0;
}

double corner_bonus(board_t board, int max) {
    int corners[4] = {0,3,12,15};
    for(int i=0;i<4;i++) {
        if(get_tile(board,corners[i]) == max)
            return max;
    }
    return 0;
}
double snake_normalized(board_t board) {
    // Matrice de poids (valeurs décroissantes en zigzag)
    const double weights[16] = {
        10.0, 8.0, 7.0, 6.5,
        1.0,  2.0, 3.0, 4.0,
        0.5,  0.4, 0.3, 0.2,
        0.0,  0.1, 0.1, 0.1
    };
    
    double score = 0;
    for (int i = 0; i < 16; i++) {
        int rank = get_tile(board, i);
        // On multiplie le rang de la tuile par le poids de sa position
        score += (double)rank * weights[i];
    }
    
    // Normalisation : Diviser par le score d'un board parfait
    return score / 100.0; 
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
            score += (1<<tile) * weights[i];
    }
    return score;
}
    
double gradient(board_t board) {
    // Dégradé depuis le coin haut-gauche
    double weights[16] = {
        4,3,2,1,
        3,2,1,0,
        2,1,0,-1,
        1,0,-1,-2
    };
    double score = 0;
    for(int i = 0; i < 16; i++) {
        int tile = get_tile(board, i);
        if(tile) score += (1 << tile) * weights[i]; // correction log2 au passage
    }
    return score;
}

double merge_potential(board_t board) {
    double score = 0;
    for(int y = 0; y < 4; y++)
        for(int x = 0; x < 3; x++)
            if(get_tile(board, y*4+x) == get_tile(board, y*4+x+1))
                score += (1 << get_tile(board, y*4+x));
    for(int x = 0; x < 4; x++)
        for(int y = 0; y < 3; y++)
            if(get_tile(board, y*4+x) == get_tile(board, (y+1)*4+x))
                score += (1 << get_tile(board, y*4+x));
    return score;
}

double values(board_t board) {
    double score = 0;
    for(int i=0;i<16;i++) {
        int tile = get_tile(board,i);
        if(tile != 0)
            score += (1<<tile) * (1<<tile); // valorise les tuiles élevées
    }
    return score;
}

double isolation_penalty(board_t board) {
    double penalty = 0;
    for(int i = 0; i < 16; i++) {
        int tile = get_tile(board, i);
        if(tile < 4) continue; // ignorer les petites tuiles
        int x = i % 4, y = i / 4;
        int neighbors[4][2] = {{x-1,y},{x+1,y},{x,y-1},{x,y+1}};
        int isolated = 1;
        for(int n = 0; n < 4; n++) {
            int nx = neighbors[n][0], ny = neighbors[n][1];
            if(nx < 0 || nx > 3 || ny < 0 || ny > 3) continue;
            int neighbor = get_tile(board, ny*4+nx);
            if(neighbor == 0 || neighbor == tile || neighbor == tile-1 || neighbor == tile+1)
                isolated = 0;
        }
        if(isolated) penalty -= (1 << tile);
    }
    return penalty;
}



double evaluate(board_t board, weights_t *w) {
    int max = max_tile(board);

    double score = 0.0;

    score += count_empty_normalized(board) * w->empty_weight;
    score += smoothness_normalized(board) * w->smooth_weight;
    score += monotonicity_normalized(board) * w->mono_weight;
    score += corner_bonus(board, max) * w->corner_weight;
    //score += (1ULL << max) * 20.0;                   
    score += merge_potential(board) * w -> merge_weight; // ajouter un poids pour le potentiel de fusion
    score += gradient(board) * w -> gradient_weight; // réutiliser corner_weight pour le gradient    
    //score += isolation_penalty(board) * 10.0; 
    //score += snake_normalized(board) * w->corner_weight; // réutiliser corner_weight pour le snake

    return score;
}

double expectimax(board_t board, int depth, int is_player, weights_t *w) {
    if(depth == 0) {
        return evaluate(board, w);
    }
    uint64_t key = board ^ ((uint64_t)depth * 0x9e3779b97f4a7c15ULL) ^ (is_player<<1);
    double cached;
    // Utiliser le cache uniquement pour les profondeurs plus élevées
    if(depth >= 4) {
        if(safe_cache_get(key, &cached)) return cached;
    }

    double result;

    if(is_player) {
        double max_score = -1e18;
        int valid_move = 0;

            #pragma omp single nowait
            {
                for(int move = 0; move < 4; move++) {

                    board_t new_board;

                    if(move == 0) new_board = move_left(board);
                    if(move == 1) new_board = move_up(board);
                    if(move == 2) new_board = move_right(board);
                    if(move == 3) new_board = move_down(board);

                    if(new_board == board) continue;

                    valid_move = 1;

                    #pragma omp task shared(max_score)
                    {
                        double score = expectimax(new_board, depth-1, 0, w);

                        #pragma omp critical
                        {
                            if(score > max_score)
                                max_score = score;
                        }
                    }
                }
        }
        if(!valid_move)
            result = evaluate(board, w)*-2; // pénalité pour les positions sans coup possible
        else
            result = max_score;
    }
    else {
        double total = 0.0;
        int empty = count_empty(board);

        if(empty == 0)
            result = evaluate(board, w)*-2; // pénalité pour les positions sans coup possible
        else {
                #pragma omp single nowait
                {
                    for(int i=0;i<16;i++) {

                        if(get_tile(board,i) != 0) continue;

                        board_t board2 = board | ((board_t)1 << (i*4));
                        board_t board4 = board | ((board_t)2 << (i*4));

                        #pragma omp task shared(total)
                        {
                            double value =
                                0.9 * expectimax(board2, depth-1, 1, w) +
                                0.1 * expectimax(board4, depth-1, 1, w);

                            #pragma omp critical
                            total += value;
                        }
                    }
                
            }  
        } 
        result = total / empty;
    }
    // Stocker dans le cache uniquement pour les profondeurs plus élevées
    // Cela permet de réduire les appels au cache pour les appels plus fréquents à faible profondeur
    if(depth >= 4) {
        safe_cache_set(key, result);
    }
    return result;   
}

int best_move(board_t board) {
     weights_t w = {
        .empty_weight = 30.0,
        .smooth_weight = 15.0,
        .mono_weight = 40.0,
        .corner_weight = 15.0,
        .gradient_weight = 40.0,
        .merge_weight = 20.0
    };


    cache_clear();

    double score[4] = {-1e18, -1e18, -1e18, -1e18};

    #pragma omp parallel for
    for(int move = 0; move < 4; move++) {

        board_t new_board;

        if(move == 0) new_board = move_left(board);
        else if(move == 1) new_board = move_up(board);
        else if(move == 2) new_board = move_right(board);
        else new_board = move_down(board);

        if(new_board == board) continue;

        int empty = count_empty(new_board);
        int depth;

        if(empty >= 6) depth = 6;
        else if(empty >= 4) depth = 7;
        else depth = 8;

        #pragma omp task firstprivate(move, new_board, depth)
        {
            score[move] = expectimax(new_board, depth, 0, &w);
        }
    }

    double best_score = -1e18;
    int best = 0;

    for(int move = 0; move < 4; move++) {
        if(score[move] > best_score) {
            best_score = score[move];
            best = move;
        }
    }

    return best;
}

int best_move_with_weights(board_t board, weights_t *w) {

    cache_clear();

    double score[4] = {-1e18, -1e18, -1e18, -1e18};

    #pragma omp parallel for
    for(int move = 0; move < 4; move++) {

        board_t new_board;

        if(move == 0) new_board = move_left(board);
        else if(move == 1) new_board = move_up(board);
        else if(move == 2) new_board = move_right(board);
        else new_board = move_down(board);

        if(new_board == board) continue;

        int empty = count_empty(new_board);
        int depth;

        if(empty >= 6) depth = 6;
        else if(empty >= 4) depth = 7;
        else depth = 8;

        #pragma omp task firstprivate(move, new_board, depth)
        {
            score[move] = expectimax(new_board, depth, 0, w);
        }
    }

    double best_score = -1e18;
    int best = 0;

    for(int move = 0; move < 4; move++) {
        if(score[move] > best_score) {
            best_score = score[move];
            best = move;
        }
    }

    return best;
}