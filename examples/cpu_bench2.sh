#!/bin/bash
# @ job_name= cpu2
# @ initialdir= .
# @ output= cpu2.out
# @ error= cpu2.err
# @ wall_clock_limit= 00:10:00
# @ total_tasks= 2
# @ cpus_per_task= 1
# @ tasks_per_node= 1
srun ./exponential_initial_state -k 0 -i 10 -d 4096 >> cpu2.out
srun ./exponential_initial_state -k 1 -i 10 -d 4096 >> cpu2.out
