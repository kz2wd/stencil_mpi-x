# Utiliser clang-format

Le fichier .clang-format est à la racine du projet.

Exemple:
clang-format -i  myblas/stencil.c --style=file:.clang-format

# Scripts

Le script `./get_gflops.sh` génère un fichier au format csv contenant les mesures de performance des différents stencils pour différentes tailles de matrices et répartitions des données entre les processus MPI. Ce fichier csv est écrit dans le répertoire `results`. Une aide est disponible pour ce script en exécutant la commande `./get_gflops.sh -h`.

Une fois les données d'exécution des stencils calculées, des graphes peuvent être générés avec `python3 plot_gflops.py results/[exemple.csv]`. Ces graphes sont écrit au format png dans le répertoire `plots`.
