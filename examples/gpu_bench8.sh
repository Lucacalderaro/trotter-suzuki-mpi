#!/bin/bash
# @ job_name= gpu8
# @ initialdir= .
# @ output= gpu8.out
# @ error= gpu8.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 8
# @ cpus_per_task= 1
# @ gpus_per_node= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 2 -i 10 -d 4096 >> gpu8.out
srun ./exponential_initial_state -k 3 -i 10 -d 4096 >> gpu8.out
srun ./exponential_initial_state -k 2 -i 10 -d 8192 >> gpu8.out
srun ./exponential_initial_state -k 3 -i 10 -d 8192 >> gpu8.out
srun ./exponential_initial_state -k 2 -i 10 -d 16384 >> gpu8.out
srun ./exponential_initial_state -k 3 -i 10 -d 16384 >> gpu8.out
