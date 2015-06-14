Michi-c2 --- Michi faster and stronger
======================================

This is a recoding in C (for speed) of the michi.py code by Petr Baudis avalaible at https://github.com/pasky/michi.

A port in C of the original michi has been kept without change at https://github.com/db3108/michi-c with the goal to retain the brevity, clarity and simplicity of the original code.

Please go on the above project pages to read more about Michi and to find some information about theory or interesting projects to do.

The goals of the current project are to improve from this basis. 
The foreseen evolutions are :
- improve the speed of the program
  * parallelization,
- add some functionalities that will increase the usability of the program, eg. 
  * variable boardsize, 
  * time management (only sudden death is implemented up to now),
  * "intelligent" early passing 

There is still room for improvements in term of speed but I focus first on the
other modifications.

The current version is able to play on CGOS. A README file and example 
configuration files can be found in the new cgos directory.

Michi-c2 is distributed under the MIT licence.  Now go forth, hack and peruse!

Installing
----------

On Linux/Unix, when in the directory that contains michi.c and Makefile, just type the command (whithout the $ sign)

$ make

This will build the michi executable.

If you have gogui (http://gogui.sourceforge.net/) installed on your system, define the GOGUI variable (export GOGUI=/path/to/gogui/bin) with the location where the gogui executables can be found. Then

$ make test

will perform a few (quick) regression tests. The result should be a single line

    tests/run

If you want a list of all the tests that are performed try

$ make test_verbose

Usage (on Linux/Unix)
---------------------

$ ./michi gtp

will allow to play a game using the gtp protocol. Type help to get the list of
available commands.

However it's easier to use michi through the gogui graphical interface.

    http://gogui.sourceforge.net/

With gogui, you can also let michi play GNUGo:

    gogui/bin/gogui-twogtp -black './michi.py gtp' -white 'gnugo --mode=gtp --chinese-rules --capture-all-dead' -size 9 -komi 7.5 -verbose -auto

It is *highly* recommended that you download Michi large-scale pattern files
(patterns.prob, patterns.spat) at:

    http://pachi.or.cz/michi-pat/

Store and unpack them in the current directory for Michi to find.

You can also try

$ ./michi mcbenchmark

this will run 2000 random playouts

$ ./michi tsdebug

this will run 1 MCTS tree search.

