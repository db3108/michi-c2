Michi-c2 --- Michi clone in C (Go gtp engine) 
=============================================

This is a recoding in C (for speed) of the michi.py code by Petr Baudis avalaible at https://github.com/pasky/michi.

A port in C of the original michi has been kept without change at https://github.com/db3108/michi-c with the goal to retain the brevity, clarity and simplicity of the original code.

Please go on the above projects pages to read more about Michi and to find some information about theory or interesting projects to do.

*NEWS*

Version 1.4 has many new features 

- variable boardsize,
- early passing
- graphics in gogui
- read sgf files
- improved portability
- plays handicap games with dynamic komi (linear)
- some speed improvements

For more details see the ChangeLog.


The current version is able to play on CGOS. A README file and example 
configuration files can be found in the new cgos directory.

Michi-c2 is distributed under the MIT licence.  Now go forth, hack and peruse!

Usage 
-----

See the user manual by opening doc/manual.html in any web browser.
If you are browsing the code from GitHub you can preferably open the 
doc/manual.rst.

Install
-------

See doc/INSTALL.md

*Important Note*  It is highly recommended that you download Michi 
large-scale pattern files (patterns.prob, patterns.spat):

http://pachi.or.cz/michi-pat/

Unpack them and place them (or a link to them) in the working directory when you run michi-c (same as michi.py), otherwise michi-c will be much weaker.
