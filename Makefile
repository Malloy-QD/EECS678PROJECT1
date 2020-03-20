quash: main.o
	gcc main.c -o quash -lreadline

main.o: main.c
	gcc -c -g main.c -lreadline
clean:
	rm -f *.o quash
