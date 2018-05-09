#!/bin/sh

[ -e a.out ] && rm a.out
gcc -Wall -Wextra driver.c ../src/rbtree.c
#gcc -Wall driver.c ../src/rbtree.c

