CC = gcc
CFLAGS = -Wall
LIBFLAGS = -fPIC -Wall

all: build

build: libsompi.so mpirun

libsompi.so: mpi.o
	$(CC) -shared $^ -o $@ -lrt

mpi.o: mpi.c protocols.h mpi.h mpi_err.h
	$(CC) $(LIBFLAGS) -c $< -o $@ -I.

mpirun: mpirun.o
	$(CC) $^ -o $@ -lrt

mpirun.o: mpirun.c protocols.h
	$(CC) $(CFLAGS) -c $< -o $@ -I.

.PHONY: clean cleanlib cleanmpirun

clean: cleanlib cleanmpirun

cleanlib:
	rm mpi.o libsompi.so

cleanmpirun:
	rm mpirun
