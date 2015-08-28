
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
