#!/bin/sh

[ -e a.out ] && rm a.out
gcc -Wall -Wextra driver.c rbtree.c
#gcc -Wall driver.c rbtree.c

