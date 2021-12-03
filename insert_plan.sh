#!/usr/bin/env bash

BINARY=/home/vagrant/lkm/plan_in/write_plan_userland/w
PLAN=/home/vagrant/lkm/plan_in/write_plan_userland/plan.log
LKM=/home/vagrant/lkm/plan_in

cd $LKM
make in
sudo $BINARY $PLAN

