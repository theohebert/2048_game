import logic
import ctypes
import cma 
import numpy as np
import constants as c
import solver

import csv

# Ouvre un fichier CSV pour écrire les logs
def init_log_file(filename="cma_log.csv"):
    with open(filename, mode='w', newline='') as f:
        writer = csv.writer(f)
        # En-tête
        writer.writerow(["Generation", "BestFitness", "MeanFitness", "BestWeights"])
    return filename


# Fichier pour logs détaillés des tests individuels
def init_test_log_file(filename="cma_test_log.csv"):
    with open(filename, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            "EmptyWeight", "SmoothWeight", "MonoWeight", "CornerWeight", 
            "MeanScore", "MaxTile"
        ])
    return filename

def log_test(filename, weights, mean_score, max_tile):
    with open(filename, mode='a', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([
            weights.empty_weight,
            weights.smooth_weight,
            weights.mono_weight,
            weights.corner_weight,
            mean_score,
            max_tile
        ])

# Ajoute une ligne de log pour chaque génération
def log_generation(filename, generation, solutions, fitness):
    best_idx = fitness.index(min(fitness))  # CMA minimise
    best_fitness = -fitness[best_idx]       # Convertir en score positif
    mean_fitness = -np.mean(fitness)
    best_weights = solutions[best_idx]

    with open(filename, mode='a', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([generation, best_fitness, mean_fitness, list(best_weights)])


def evaluate_weights(w, solver_instance):
    weights = solver.Weights(*np.maximum(w, 0))

    scores = []
    max_tiles = []

    for _ in range(3):
        score, max_tile = play_game(weights, solver_instance)
        scores.append(score)
        max_tiles.append(max_tile)

    mean_score = np.mean(scores)
    mean_tile = np.mean(max_tiles)

    print(f"Weights: {weights.empty_weight:.1f}, {weights.smooth_weight:.1f}, "
          f"{weights.mono_weight:.1f}, {weights.corner_weight:.1f} | "
          f"Score: {mean_score:.1f}, MaxTile: {mean_tile}")

    # 💥 IMPORTANT : priorité à la tuile max
    fitness = mean_tile * 1e6 + mean_score

    return -fitness

solver_instance = solver.Solver()


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

    return logic.get_score(solver_instance.puzzle.matrix), logic.max_tile(solver_instance.puzzle.matrix)

es = cma.CMAEvolutionStrategy(
    [270, 20, 47, 1500],  # initial guess
    50,                    # sigma
    {'popsize': 12}         # population plus petite pour accélérer
)

if __name__ == "__main__":
    log_file = init_log_file("cma_optimization_log.csv")
    test_log_file = init_test_log_file("cma_test_log.csv")
    generation = 0
    while not es.stop():
        generation += 1
        solutions = es.ask()
        fitness = [evaluate_weights(s, solver_instance) for s in solutions]

        es.tell(solutions, fitness)

        # Log dans le fichier CSV
        log_generation(log_file, generation, solutions, fitness)

        print(f"Gen {generation} | Best fitness: {-min(fitness):.2f}")

    print("\nBest weights found:")
    print(es.result.xbest)