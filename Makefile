
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

OBJS=ui.o sgf.o control.o michi.o params.o board.o board_util.o patterns.o debug.o main.o
BIN=michi

all: $(BIN) 

michi: $(OBJS) michi.h board.h
	gcc $(CFLAGS) -std=gnu99 -o michi $(OBJS) -lm

%.o: %.c michi.h board.h
	gcc $(GENCODE) $(CFLAGS) -c -std=gnu99 $< 

test: all
	tests/run

test_verbose:
	tests/run -long

test_michi:
	tests/run_michi

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
	rm -rf tests/output tests/tst tests/michi.log michi.log
