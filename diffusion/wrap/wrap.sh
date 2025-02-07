#!/bin/bash


# extract all object files ar x libdiffusion.a
# extract specific file with number(required for create.o, start.o, free.o) ar xN 2 libdiffusion.a create.o
# wrap function, really required fprintf
# build static library again ar rcs libdiffusion_exp.a ../wrap/wrap_printf.o ../wrap/wrap_puts.o ../wrap/wrap_fprintf.o ../wrap/exp/res/res/res/*.o

for obj in libobj/*.o 
do 
	ld --wrap=printf $obj -r -o libobj_w/$obj
done
