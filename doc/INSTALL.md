
1 Installing on Linux/Unix
==========================

When in the directory that contains michi.c and Makefile, just type the command (whithout the $ sign)

$ make

This will build the michi executable. 
If you get some compilation errors see section 4.1.

Now it is time to perform simple tests to verify that michi works on your system.

1.1 Run the tests of the board module
-------------------------------------

If you have gogui (http://gogui.sourceforge.net/) installed, 
define the GOGUI variable 

$ export GOGUI=/path/to/gogui/bin

with the location where the gogui executables can be found. Then

$ make test

will perform a few (quick) regression tests of the board module. 
The result should be a single line

tests/run

If you get message like *Program died* go to section 4 "Troubleshooting" for a
few advices of what you can do to try to solve the problem.

If you want a list of all the tests that are performed try

$ make test_verbose

1.2 Run the tests of the michi playout and tree search
------------------------------------------------------

After installing the large pattern definitions in the build directory of michi
(see section 3), you can run simple non-regression test 

$ make test_michi

This test compares the results on your system with a reference that has been 
computed on my Linux box. 
It is fast (takes less than 20 sec on my old computer).

It could even be run if you do not have the large patterns installed.
In this case, it will only test the computation of the random playouts
and you will get a message that you can ignore

*files tests/ref/tsdebug.txt and tests/tst/tsdebug.txt differ*


2 Installing on other systems
=============================

This version has been verified to work on Windows Seven 64 bits under mingw.
The same procedure as for UNIX/Linux should work for you.

3 Download the large patterns definitions
=========================================

One important thing is to download the Michi large-scale pattern files 
(patterns.prob.xz, patterns.spat.xz) that Petr Baudis provides at :

http://pachi.or.cz/michi-pat/

Unpack them and place them (or a link to them) in the working directory 
where you run michi-c, otherwise it will be much weaker.

4 Troubleshooting
=================

4.1 Problems with compilation
-----------------------------

Having tried to respect the C standards (C99) and simple coding practices, 
I hope that you will not experience compilation errors nor even a lot of 
warnings.

That said, for performance, I have used some gcc intrinsics that your compiler
could not know. In that case, you have two solutions :

- define the macro PORTABLE and a fully portable code (less efficient) 
  will be used instead of these intrinsics,
- find the equivalent for your compiler and place these definitions in the file
  non_portable.h (pull requests on GitHub are welcomed). 

If you get other errors please tell me.

4.2 Problems with the tests of the board module
-----------------------------------------------

If you get a "Program died" message, the first thing to do is to know 
in which of the 4 tests this happens. 
For this you can go into the tests directory and try::

    $ ../michi gtp < fix_atari.tst
    $ ../michi gtp < large_patterns.tst
    $ ../michi gtp < board.tst
    $ ../michi gtp < undo.tst

This will run michi exactly as gogui-regress do 
(with the same working directory but without checking the results)
but now you will see the messages that gogui-regress captured before. 
This could help. 

The second step is to read the michi.log file 
(warning: this file is overwritten each time michi is restarted)

If these steps did not help to solve the problem, or if the results are 
different from those expected in the test files, you can reactivate the 
internal tests that are available in the code (uncomment the lines 
michi_assert()) and compile without defining the variable NDEBUG.

Note: the execution time is now much much longer ...
This is the reason why I have commented these lines in order to avoid mistakes.

4.3 Problems with the tests of the random playouts and tree search
------------------------------------------------------------------

Nothing special to say except that the code is deterministic and it should give
the same answer on all platforms when given the same inputs (except if you
have given the special value -1 to the random_seed parameter).

If problem, I know no better solutions than to make detailed comparaisons of
the executions of **michi mcdebug** and **michi tsdebug** on different systems.
