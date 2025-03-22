.DEFAULT_GOAL := compile

compile: src/main.c
	gcc src/main.c -o what-is-my-ip
