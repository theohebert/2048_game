# 2048 avec solver

## Projet IA pour les jeux

Ce projet utilise une base de code trouvée sur github pour le jeu de 2048 en pyrhon. Le code de base a été modifier pour implementer les règles originelles du 2048 (seul des 2 pouvaient apparaître, le code a été modifié pour que des 4 puissent aussi apparaître).

Le code a aussi été modifié pour qu'un algorithme puisse jouer et echanger avec le jeu.


Heuristique : https://stackoverflow.com/questions/22342854/what-is-the-optimal-algorithm-for-the-game-2048/22498940#22498940 

## Compilation et lancement

Il faut dans un premier temps compiler le solver codé en C avec la commande suivante si ce n'est pas déjà fait:

* gcc -O3 -shared -fPIC solver.c -o libsolver.so

On peut ensuite lancer le solver avec la commande suivante :

* python3 solver.py
