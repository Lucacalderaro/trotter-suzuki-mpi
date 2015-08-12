#!/bin/bash
# @ job_name= cpu16
# @ initialdir= .
# @ output= cpu16.out
# @ error= cpu16.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 16
# @ cpus_per_task= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 0 -i 10 -d 4096 >> cpu16.out
srun ./exponential_initial_state -k 1 -i 10 -d 4096 >> cpu16.out
srun ./exponential_initial_state -k 0 -i 10 -d 8192 >> cpu16.out
srun ./exponential_initial_state -k 1 -i 10 -d 8192 >> cpu16.out
srun ./exponential_initial_state -k 0 -i 10 -d 16384 >> cpu16.out
srun ./exponential_initial_state -k 1 -i 10 -d 16384 >> cpu16.out
