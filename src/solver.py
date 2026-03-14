import logic
from puzzle import GameGrid
import constants as c
import ctypes
import time


MOVE_MAP = {
    0: 'up',
    1: 'down',
    2: 'left',
    3: 'right'
}

class Solver:
    def __init__(self):
        self.puzzle = GameGrid(False)
        self.solver = ctypes.CDLL("./solver/libsolver.so")

        self.solver.best_move.argtypes = [ctypes.c_uint64]
        self.solver.best_move.restype = ctypes.c_int

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

    



    

if __name__ == "__main__":
    solver = Solver()
    solver.solve()

        