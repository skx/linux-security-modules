
can-exec: can-exec.c
	gcc -Wall -Werror -std=c99 -o can-exec can-exec.c

install: can-exec
	install --mode=0755 -owner=root --group=root can-exec /sbin/can-exec

clean:
	rm -f can-exec
