# master_read_plan

This repository contains the Kernel Module desciribed in **Level Three: Kernel / Plan Input**. Most notably, it contains 3 symlinked files.

1. **config.h:** Pointing to the configuration file for the plan based scheduler
2. **pbs_entities.h:** Definition for the plan data structures
3. **pb-scheduler.h** Refering to the kernel functionality that gives access to the plan struct that resides inside kernel memory

For simplicity reasons, this repo contains copies of those files, but as soon as some relevant data is changed in one of those three files, this module might stop working.

To execute this module:

1. run `make` in the base directory, it should compile the lkm `pbs_plan_input.ko`
2. run `make` in `write_plan_userland`, it should compile the binary `w`
3. run `sudo ./insert_plan.sh` in the base directory, checking the dmesg-log, it should verify the plan was successfully inserted:

```

[ 2497.135784] [PBS_plan_write]0: fixing pointers, before: cur_task=00007ffe05883170, cur_process=00007ffe05881eb0
[ 2497.135784] [PBS_plan_write]0: fixing pointers, after: cur_task=ffffffff81a82650, cur_process=ffffffff81a81390
[ 2497.135785] [PBS_plan_write]0: plan_ptr=ffffffff81a81380, processes=ffffffff81a81390, tasks=ffffffff81a82650
[ 2497.135785] [PBS_plan_write]0: 1st_process=ffffffff81a81390, 2nd_process=ffffffff81a813c0
[ 2497.135785] [PBS_plan_write]0: 1st_task=ffffffff81a82650, last_task=ffffffff81a87e58
[ 2497.135786] [PBS_plan_write]0: 1st_process: id=0, num_tasks_remaining=100, instructions_retired=0, buffer=7297012721
[ 2497.135786] [PBS_plan_write]0: 1st_task: id=0, instructions_planned=1140367204, instructions_retired=0, lateness=0


```
