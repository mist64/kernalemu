all:
	cc -O3 -c console.c
	cc -O3 -c runtime.c
	cc -O3 -c cbmdos.c
	cc -O3 -c screen.c
	cc -O3 -c fake6502.c
	cc -o kernalemu console.o cbmdos.o screen.o runtime.o fake6502.o

clean:
	rm *.o
