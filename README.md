Michi-c2 --- Michi faster and stronger
======================================

This is a recoding in C (for speed) of the michi.py code by Petr Baudis avalaible at https://github.com/pasky/michi.

A port in C of the original michi has been kept without changes at https://github.com/db3108/michi-c with the goal to retain the brevity, clarity and simplicity of the original code.

Please go on the above project pages to read more about Michi and to find some information about theory or interesting projects to do.

The goals of the current project are to improve from this basis. 
The foreseen evolutions are :
- improve the speed of the program
  * parallelization,
  * fast board implementation with incremental update of blocks and liberties,
- add some functionalities that will increase the usability of the program, eg. 
  * time management, 
  * variable parameters modifiable by gtp commands for CLOP tuning, 
  * variable boardsize, 
  * "intelligent" early passing 

Michi-c2 is distributed under the MIT licence.  Now go forth, hack and peruse!

