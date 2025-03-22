.DEFAULT_GOAL :=gcc

gcc: src/main.c
	gcc src/main.c  -o what-is-my-ip
install: what-is-my-ip
	cp what-is-my-ip /usr/local/bin
	cp what-is-my-ip.service /lib/systemd/system

uninstall:
	rm /usr/local/bin/what-is-my-ip
	rm /lib/systemd/system/what-is-my-ip.service
