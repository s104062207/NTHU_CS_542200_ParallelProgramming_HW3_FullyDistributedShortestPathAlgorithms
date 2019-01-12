#!/bin/bash
#SBATCH -p batch -N 1 -n 12
time srun ./apsp ./cases/input/dense_100.in out
