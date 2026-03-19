import logic
from puzzle import GameGrid
import constants as c
import ctypes
import time


MOVE_MAP = {
    0: 'left',
    1: 'up',
    2: 'right',
    3: 'down'
}

class Weights(ctypes.Structure):
    _fields_ = [
        ("empty_weight", ctypes.c_double),
        ("smooth_weight", ctypes.c_double),
        ("mono_weight", ctypes.c_double),
        ("corner_weight", ctypes.c_double),
    ]

class Solver:
    def __init__(self):
        self.puzzle = GameGrid(False)
        self.solver = ctypes.CDLL("./solver/libsolver.so")

        self.solver.best_move.argtypes = [ctypes.c_uint64]
        self.solver.best_move.restype = ctypes.c_int

        self.solver.best_move_with_weights.argtypes = [ctypes.c_uint64, ctypes.POINTER(Weights)]
        self.solver.best_move_with_weights.restype = ctypes.c_int
        self.solver.solver_init() #initialise les tables de déplacement

    def solve(self):
        #on appelle notre code c pour trouver la meilleure solution
        #on applique la solution à notre puzzle
        #on recommence jusqu'à ce que le jeu soit terminé
        while True:
            move = self.get_best_move()
            move_str = MOVE_MAP[move]
            self.puzzle.matrix, done = self.puzzle.commands[move_str](self.puzzle.matrix)
            if done:
                self.puzzle.matrix = logic.add_two(self.puzzle.matrix)
            self.puzzle.update_grid_cells()
            if logic.game_state(self.puzzle.matrix) != 'not over':
                if logic.game_state(self.puzzle.matrix) == 'win':
                    wait = input("You win! Press Enter to continue...")
                    self.puzzle.grid_cells[1][1].configure(text="You", bg=c.BACKGROUND_COLOR_CELL_EMPTY)
                    self.puzzle.grid_cells[1][2].configure(text="Win!", bg=c.BACKGROUND_COLOR_CELL_EMPTY)
                else:
                    wait = input("Game over! Press Enter to continue...")
                    self.puzzle.grid_cells[1][1].configure(text="You", bg=c.BACKGROUND_COLOR_CELL_EMPTY)
                    self.puzzle.grid_cells[1][2].configure(text="Lose!", bg=c.BACKGROUND_COLOR_CELL_EMPTY)
                break
            time.sleep(0.05)
        
    def get_best_move(self):
        bitboard = self.puzzle.matrix_to_bitboard()
        move = self.solver.best_move(bitboard)

        return move
    
    def get_best_move_with_weights(self, weights):
        bitboard = self.puzzle.matrix_to_bitboard()
        move = self.solver.best_move_with_weights(bitboard, ctypes.byref(weights))

        return move

    def solve_with_weights(self, weights):
        #on appelle notre code c pour trouver la meilleure solution
        #on applique la solution à notre puzzle
        #on recommence jusqu'à ce que le jeu soit terminé
        while True:
            move = self.get_best_move_with_weights(weights)
            move_str = MOVE_MAP[move]
            self.puzzle.matrix, done = self.puzzle.commands[move_str](self.puzzle.matrix)
            if done:
                self.puzzle.matrix = logic.add_two(self.puzzle.matrix)
            self.puzzle.update_grid_cells()
            if logic.game_state(self.puzzle.matrix) != 'not over':
                if logic.game_state(self.puzzle.matrix) == 'win':
                    wait = input("You win! Press Enter to continue...")
                    self.puzzle.grid_cells[1][1].configure(text="You", bg=c.BACKGROUND_COLOR_CELL_EMPTY)
                    self.puzzle.grid_cells[1][2].configure(text="Win!", bg=c.BACKGROUND_COLOR_CELL_EMPTY)
                else:
                    wait = input("Game over! Press Enter to continue...")
                    self.puzzle.grid_cells[1][1].configure(text="You", bg=c.BACKGROUND_COLOR_CELL_EMPTY)
                    self.puzzle.grid_cells[1][2].configure(text="Lose!", bg=c.BACKGROUND_COLOR_CELL_EMPTY)
                break
            time.sleep(0.05)



    

if __name__ == "__main__":
    solver = Solver()
    solver.solve()

        