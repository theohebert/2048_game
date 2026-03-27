import solver
import multiprocessing as mp
import logic
import puzzle
import constants as c

import csv


def init_log_file(filename="game_log.csv"):
    with open(filename, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["gradient_weight", "merge_weight", "Score", "MaxTile"])
    return filename

def init_log_games_file(filename="grid_search_log.csv"):
    with open(filename, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["gradient_weight", "merge_weight", "mean_score", "mean_max_tile", "median_score", "median_max_tile"])
    return filename


def log_game(filename, weights, score, max_tile):
    with open(filename, mode='a', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            weights.gradient_weight,
            weights.merge_weight,
            score,
            max_tile
        ])



def log_games(filename, weights: solver.Weights, scores, max_tiles):
    with open(filename, mode='a', newline='') as f:
        writer = csv.writer(f)
        mean_score = sum(scores) / len(scores) if scores else 0
        mean_max_tile = sum(max_tiles) / len(max_tiles) if max_tiles else 0
        median_score = sorted(scores)[len(scores) // 2] if scores else 0
        median_max_tile = sorted(max_tiles)[len(max_tiles) // 2] if max_tiles else 0
        
        writer.writerow([
            weights.gradient_weight,
            weights.merge_weight,
            mean_score,
            mean_max_tile,
            median_score,
            median_max_tile
        ])

"""
def play_game(weights, solver_instance):
    # Initialiser le plateau
    board = solver.new_game()
    score = 0

    while True:
        move = solver.get_best_move(board, weights)
        if move is None:
            break  # Plus de mouvements possibles

        board, gained_score = solver.move(board, move)
        score += gained_score

    max_tile = solver.get_max_tile(board)
    log_game("game_log.csv", weights, score, max_tile)
    return score, max_tile
"""

def play_game(weights, solver_instance):
    # Initialiser le plateau
    solver_instance.puzzle.matrix = logic.new_game(c.GRID_LEN)
    while True:
        board = solver_instance.puzzle.matrix
        move = solver_instance.get_best_move_with_weights(weights)
        # Appliquer le mouvement
        if move == 0:
            new_board = logic.left(board)
        elif move == 1:
            new_board = logic.up(board)
        elif move == 2:
            new_board = logic.right(board)
        else:
            new_board = logic.down(board)
        # Pas de mouvement possible -> game over
        if new_board == board:
            break
        # Mettre à jour le plateau avec le mouvement appliqué + ajouter une tuile
        solver_instance.puzzle.matrix = logic.add_two(new_board[0])
        # Vérifier fin de partie
        if logic.game_state(solver_instance.puzzle.matrix) != 'not over':
            break
    
    log_game("game_log.csv", weights, logic.get_score(solver_instance.puzzle.matrix), logic.max_tile(solver_instance.puzzle.matrix))
    return logic.get_score(solver_instance.puzzle.matrix), logic.max_tile(solver_instance.puzzle.matrix)

"""
def grid_search(x0, y0, x1, y1, step0, step1):
    # Créer une grille de poids pour les 2 heuristiques
    for gradient_weight in range(x0, y0 + 1, step0):
        for merge_weight in range(x1, y1 + 1, step1):
            weights = solver.Weights(gradient_weight, merge_weight)
            scores = []
            max_tiles = []
            for _ in range(10):  # Jouer plusieurs parties pour chaque combinaison
                score, max_tile = play_game(weights, solver_instance)
                scores.append(score)
                max_tiles.append(max_tile)
            log_games("grid_search_log.csv", weights, scores, max_tiles)

"""
# Fonction "travailleuse" qui sera exécutée sur chaque cœur
def worker_game(weights_data):
    # ATTENTION : Si ton solver C a besoin d'une initialisation par process,
    # fais-la ici. Chaque process enfant aura sa propre mémoire.
    # solver_instance = solver.init() 
    solver_instance = solver.Solver()  # Crée une instance du solver pour ce process
    ctypes_weights = solver.Weights(*weights_data)  # Convertir les données en structure C
    score, max_tile = play_game(ctypes_weights, solver_instance)
    return score, max_tile

"""
def grid_search(x0, y0, x1, y1, step0, step1):
    # Création d'un pool de 5 ou 10 workers
    # Si tu as 10 cœurs, mets 10. Si tu en as 5, mets 5.
    pool = mp.Pool(processes=5) 

    for g_w in range(x0, y0 + 1, step0):
        for m_w in range(x1, y1 + 1, step1):
            weights = solver.Weights(g_w, m_w)
            
            print(f"Testing weights: Gradient={g_w}, Merge={m_w}...")

            # Lancement de 10 parties en parallèle
            # pool.map va distribuer les 10 tâches aux workers disponibles
            results = pool.map(worker_game, [weights] * 10)

            # Extraction des résultats
            scores = [r[0] for r in results]
            max_tiles = [r[1] for r in results]

            log_games("grid_search_log.csv", weights, scores, max_tiles)

    pool.close()
    pool.join()

"""
def grid_search(x0, y0, x1, y1, step0, step1,processes=5):

    pool = mp.Pool(processes=processes)

    for x in range(x0, y0 + 1, step0):
        for y in range(x1, y1 + 1, step1):
            if (x == 0 or (x==10 and y<=30)):
                continue

            print(f"Testing weights: Gradient={x}, Merge={y}...")

            tasks = [(x, y) for _ in range(10)]
            results = pool.map(worker_game, tasks)

            scores = [r[0] for r in results]
            max_tiles = [r[1] for r in results]

            weights = solver.Weights(x, y)

            log_games("grid_search_log.csv", weights, scores, max_tiles)

    pool.close()
    pool.join()

if __name__ == "__main__":
    # Exemple de recherche de poids entre 0 et 1 avec un pas de 0.1
    #solver_instance = solver.Solver()
    #play_game(solver.Weights(40.0, 20.0), solver_instance)  # Test rapide avec les poids par défaut
    #init_log_file("game_log.csv")
    #init_log_games_file("grid_search_log.csv")
    grid_search(0, 100, 0, 100, 10, 10, processes=10)