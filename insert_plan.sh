#!/usr/bin/env bash

echo "RUN THIS SCRIPT IN THE BASE DIRECTORY OF master_read_plan repository"

BINARY=./write_plan_userland/w
PLAN=./write_plan_userland/plan.log

make in
sudo $BINARY $PLAN

