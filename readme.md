# master_read_plan

This repository contains the Kernel Module desciribed in **Level Three: Kernel / Plan Input**. Most notably, it contains 3 symlinked files.

1. **config.h:** Pointing to the configuration file for the plan based scheduler
2. **pbs_entities.h:** Definition for the plan data structures
3. **pb-scheduler.h** Refering to the kernel functionality that gives access to the plan struct that resides inside kernel memory

For simplicity reasons, this repo contains copies of those files, but as soon as some relevant data is changed in one of those three files, this module might stop working.
