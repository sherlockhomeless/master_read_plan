#!/usr/bin/env bash

BINARY=/home/vagrant/kernel_src/pbs_plan_copy/write_plan_userland/w
PLAN=/home/vagrant/kernel_src/plans/plan.log
LKM=/home/vagrant/kernel_src/pbs_plan_copy

cd $LKM
make in
sudo $BINARY $PLAN
