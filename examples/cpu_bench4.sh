#!/bin/bash
# @ job_name= cpu4
# @ initialdir= .
# @ output= cpu4.out
# @ error= cpu4.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 4
# @ cpus_per_task= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 0 -i 10 -d 4096 >> cpu4.out
srun ./exponential_initial_state -k 1 -i 10 -d 4096 >> cpu4.out
srun ./exponential_initial_state -k 0 -i 10 -d 8192 >> cpu4.out
srun ./exponential_initial_state -k 1 -i 10 -d 8192 >> cpu4.out
srun ./exponential_initial_state -k 0 -i 10 -d 16384 >> cpu4.out
srun ./exponential_initial_state -k 1 -i 10 -d 16384 >> cpu4.out
