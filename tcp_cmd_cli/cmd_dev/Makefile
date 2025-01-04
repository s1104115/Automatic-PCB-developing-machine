all:
	gcc -g -Wall cmd_dev.c -I./ -Wl,-Bstatic -Wl,-Bdynamic -lpthread -l wiringPi -o cmd_dev

clean:
	rm -f cmd_dev
