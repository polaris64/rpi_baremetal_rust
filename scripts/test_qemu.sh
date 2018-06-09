#!/bin/sh

qemu-system-aarch64 -m 256 -M raspi2 -serial stdio -kernel kernel.img
