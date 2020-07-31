#!/bin/bash

make clean && \
make && \
qemu-system-x86_64 -nographic -smp 8 -d int,cpu_reset -D build/qemu.log -serial mon:stdio -drive format=raw,file=build/kernel-x86_64-qemu