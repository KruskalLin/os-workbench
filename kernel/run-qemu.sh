#!/bin/bash

#make clean && \
make && \
qemu-system-x86_64 -s -S -nographic -smp 8 -serial none -machine accel=tcg -drive format=raw,file=build/kernel-x86_64-qemu &
pid=$!
gdb \
  -ex "target remote localhost:1234" \
  -ex "set confirm off" \
  -ex "file build/kernel-x86_64-qemu.o"

kill -9 $!