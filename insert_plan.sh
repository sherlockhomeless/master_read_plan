#!/usr/bin/env bash

BINARY=./write_plan_userland/w
PLAN=/./write_plan_userland/plan.log

make in
sudo $BINARY $PLAN

