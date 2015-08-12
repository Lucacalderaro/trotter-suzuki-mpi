#!/bin/bash
# @ job_name= prova
# @ initialdir= .
# @ output= prova.out
# @ error= prova.err
# @ total_tasks= 2
# @ cpus_per_task= 2
# @ gpus_per_node= 2
# @ wall_clock_limit= 00:10:00
srun ./exponential_initial_state > prova.out
