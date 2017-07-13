all:
	cc -c console.c
	cc -c runtime.c
	cc -c fake6502.c
	cc -o kernalemu console.o runtime.o fake6502.o

clean:
	rm *.o
