#!/bin/bash
# @ job_name= cpu32
# @ initialdir= .
# @ output= cpu32.out
# @ error= cpu32.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 32
# @ cpus_per_task= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 0 -i 10 -d 4096 >> cpu32.out
srun ./exponential_initial_state -k 1 -i 10 -d 4096 >> cpu32.out
srun ./exponential_initial_state -k 0 -i 10 -d 8192 >> cpu32.out
srun ./exponential_initial_state -k 1 -i 10 -d 8192 >> cpu32.out
srun ./exponential_initial_state -k 0 -i 10 -d 16384 >> cpu32.out
srun ./exponential_initial_state -k 1 -i 10 -d 16384 >> cpu32.out
