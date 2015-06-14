The CGOS (Computer GO Server) page http://cgos.boardspace.net/ provides 
documentation and tools necessary to make a go engine play on CGOS.

This is the procedure that I have used to connect michi to CGOS.

1. Download the CGOS Engine Client cgosGtp-linux-x86_64.tar.gz

2. Prepare the michi.cfg configuration file
  - choose a name for your account
  - choose a password. The password is initialized the first time you connect 
   this account on CGOS. 

3. Run the CGOS Engine Client using the command line that is found in run

4. You can monitor the dialog between the server and the engine in real time 

    $ tail -1lf cgos.log 
   
  in the same way the michi.log can be monitored in a separate window

    $ tail -1lf michi.log

  Of course, for those who know, there are other ways to do this.
  For the others, the above commands are sufficient.
 
5. I do not use the CGOS Viewing client. I just follows what happens in the page
 results at 
  * http://cgos.boardspace.net/9x9/standings.html
  * http://cgos.boardspace.net/13x13/standings.html
  * http://cgos.boardspace.net/19x19/standings.html
 
6. To properly end the connection with the server, put the sentinel file 
  *kill.txt* in the directory where the cgos client was run. 
  It will stop as soon as the next game is finished.
