#!/bin/bash
# @ job_name= gpu32
# @ initialdir= .
# @ output= gpu32.out
# @ error= gpu32.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 32
# @ cpus_per_task= 1
# @ gpus_per_node= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 2 -i 10 -d 4096 >> gpu32.out
srun ./exponential_initial_state -k 3 -i 10 -d 4096 >> gpu32.out
srun ./exponential_initial_state -k 2 -i 10 -d 8192 >> gpu32.out
srun ./exponential_initial_state -k 3 -i 10 -d 8192 >> gpu32.out
srun ./exponential_initial_state -k 2 -i 10 -d 16384 >> gpu32.out
srun ./exponential_initial_state -k 3 -i 10 -d 16384 >> gpu32.out
