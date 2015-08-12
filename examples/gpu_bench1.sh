#!/bin/bash
# @ job_name= gpu1
# @ initialdir= .
# @ output= gpu1.out
# @ error= gpu1.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 1
# @ cpus_per_task= 1
# @ gpus_per_node= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 2 -i 10 -d 4096 >> gpu1.out
srun ./exponential_initial_state -k 3 -i 10 -d 4096 >> gpu1.out
