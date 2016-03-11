all:
	gcc -o lclock main.c vm.c message.c

clean:
	rm -f *.out *.sock lclock
	