.DEFAULT_GOAL :=gcc

gcc: src/main.c
	gcc src/main.c  -o what-is-my-ip
