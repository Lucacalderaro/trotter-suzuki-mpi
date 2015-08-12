#!/bin/bash
# @ job_name= gpu4
# @ initialdir= .
# @ output= gpu4.out
# @ error= gpu4.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 4
# @ cpus_per_task= 1
# @ gpus_per_node= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 2 -i 10 -d 4096 >> gpu4.out
srun ./exponential_initial_state -k 3 -i 10 -d 4096 >> gpu4.out
srun ./exponential_initial_state -k 2 -i 10 -d 8192 >> gpu4.out
srun ./exponential_initial_state -k 3 -i 10 -d 8192 >> gpu4.out
srun ./exponential_initial_state -k 2 -i 10 -d 16384 >> gpu4.out
srun ./exponential_initial_state -k 3 -i 10 -d 16384 >> gpu4.out
