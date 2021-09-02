#!/usr/bin/env bash

BINARY=/home/vagrant/lkm/plan_in/write_plan_userland/w
PLAN=/home/vagrant/kernel_src/plans/plan.log
LKM=/home/vagrant/lkm/plan_in

cd $LKM
make in
sudo $BINARY $PLAN

