
# For compiling portable code
#GENCODE=-DPORTABLE
# For compiling more efficient code for a given compiler/processor architecture
GENCODE=

# Normal compilation options for developping
#CFLAGS= -DNDEBUG -g -march=native -msse4 -fshort-enums -Wall -Wno-char-subscripts
#CFLAGS= -g -march=native -msse4 -fshort-enums -Wall -Wno-char-subscripts
#CFLAGS= -O3 -march=native -msse4 -fshort-enums -Wall -Wno-char-subscripts

# Normal compilation options for production
CFLAGS= -DNDEBUG -O3 -march=native -msse4 -fshort-enums -Wall -Wno-char-subscripts

# Compilation options for running valgrind
#CFLAGS=-O0 -g -march=native -msse4 -Wall -std=gnu99

# Compilation options for profiling with gprof
#CFLAGS=-pg -O3 -DNDEBUG -march=native -msse4 -fshort-enums -Wall -Wno-char-subscripts

OBJS=params.o board.o patterns.o debug.o main.o
BIN=michi

all: $(BIN) 

michi: $(OBJS) michi.o michi.h
	gcc $(CFLAGS) -std=gnu99 -o michi michi.o $(OBJS) -lm

%.o: %.c michi.h
	gcc $(GENCODE) $(CFLAGS) -c -std=gnu99 $< 

test:
	tests/run

test_verbose:
	tests/run -long

valgrind: michi
	valgrind --track-origins=yes ./michi tsdebug

callgrind: michi
	valgrind --tool=callgrind ./michi mcbenchmark 

tags: $(BIN)
	ctags *.c *.h

clean:
	rm -f $(OBJS) michi.o michi-debug.o

veryclean: clean
	rm -f $(BIN)
	rm -rf tests/output tests/michi.log michi.log
