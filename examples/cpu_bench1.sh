#!/bin/bash
# @ job_name= cpu1
# @ initialdir= .
# @ output= cpu1.out
# @ error= cpu1.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 1
# @ cpus_per_task= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 0 -i 10 -d 4096 >> cpu1.out
srun ./exponential_initial_state -k 1 -i 10 -d 4096 >> cpu1.out
