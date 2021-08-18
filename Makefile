obj-m+= pbs_plan_input.o
plan_input-objs+= pbs_plan_parse.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
in:
	sudo insmod pbs_plan_input.ko
out:
	sudo rmmod pbs_plan_input
reload:
	sudo rmmod pbs_plan_input && sudo insmod pbs_plan_in.ko
