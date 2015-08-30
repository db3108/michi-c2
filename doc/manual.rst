=======================
User Manual for michi-c
=======================

Denis Blumstein

Revision : 1.4

1. Introduction
***************

Michi-c is a *Go gtp engine*. This means that 

- it is a program that plays Go, the ancient asian game.
- it takes its inputs and provides its outputs in text mode using the 
  so-called *Go Text Protocol* (gtp) [1].
  
Put simply, this means that the program reads commands like

*play black C3*  
    which tells him to put a black stone on intersection C3

*genmove white*
    which tells him to answer its move in the current position
 
It is perfectly possible to play games with michi-c that way.
But it becomes fastidious very soon. 

Luckily Markus Enzenberger has built a wonderful program named *gogui* [2] 
that allows us to play interactively with michi using the mouse and a graphical
display of a board (and much more as we will see in the next section).

So this manual is for you if you need information

- on how to use michi-c with gogui (section 2)
- on the behaviour of the gtp commands understood by michi-c (section 3)
- the command line arguments (section 4)

**References**

[1] Gtp Go Text Protocol : http://www.lysator.liu.se/~gunnar/gtp/

[2] Gogui : http://gogui.sourceforge.net/

[3] Smart Game Format : http://www.red-bean.com/sgf 

2. Playing with michi-c through gogui
*************************************

**Prerequisite** we suppose that gogui has been installed on your system and 
is working well (tested by playing manually a few moves for example) 

2.1 Making gogui aware of michi-c
---------------------------------

The first thing is to add michi-c to the list of programs known by gogui.
This is done by selecting "New Program" in the "Program" menu 
as shown in figure 1 below (See the gogui documentation chapter 4.)

Then a windows pops up with two text fields to fill (fig.2). 
In the *command* field enter 

    /absolute/path/to/michi gtp

This will run michi-c with the default parameters. In section 4 you will see 
how these parameters can be changed at initialization time.

I recommend that you enter the absolute path where the michi executable is. 

In the *working directory* field enter any existing directory you like.
This is where the michi log file will go.

If you get a new window as shown in fig. 3 when you click OK, then  
it seems that you are done.

.. figure:: img/img1.png
    :scale: 75 %
    :align: center

    fig1. Definition of an New Program

    ..

.. figure:: img/img2.png
    :scale: 75 %
    :align: center

    fig2. Define the command to run michi

    ..

.. figure:: img/img3.png
    :scale: 75 %
    :align: center

    fig3. The new program gogui will add in its list

    ..

What happens is that gogui has began to talk to michi.
It has asked for its name and its version. 
The fact that the window has appeared shows that michi has answered correctly.
Before you validate the introduction of michi-c in the list of programs known 
by gogui (by clicking OK), you have the opportunity to change the label
which, by default is the name of the program. This can be useful to have many
versions of the same program but with different parameter settings. 
(it is always possible to change it afterwards, using the "Edit Program" entry 
in the Program menu).

michi-c is now usable in gogui and you can begin to play with it.

But before playing, I **strongly** recommend that you install the large patterns
provided by Petr Baudis at http://pachi.or.cz/michi-pat/ (see INSTALL.md). 
If you don't do this michi-c will be much weaker and playing experience will be
much worse.
**Once the files are unpacked, patterns.prob and patterns.spat must be 
placed in the working directory** 
(or make a link on them in the working directory).

You play just by clicking on the intersection you want to play and as soon as
you have played the program begins to "think" and will reply after a few
seconds (the computer color has been set to white at initialization and can
be changed in the menu *Game*, see fig.4).

.. figure:: img/img5.png
    :scale: 75 %
    :align: center

    fig4. How to change the computer color

    ..

**Troubleshooting** If you have not been as lucky as described in the above
paragraphs, the best advice I could give is to verify that the *command* field
is correct. On Linux or MacOS X the easy way to do it is to try to execute 
this command in a terminal.

Then type **name** and the program should answer::

    = michi-c

and wait for another command. You can then try **help** to obtain a list of all
available commands. **quit** will leave michi.

Normally, if you had trouble in running michi in gogui, something wrong should 
happen in the previous sequence. This should give you enough feedback, 
either from the shell (if you made an error in the path) or from michi, for you
to undersand what the problem is and to correct it.

If you are still in trouble, maybe is it time to read the section 4 
"Troubleshooting" in the INSTALL file.

2.2 What michi think of the position ?
--------------------------------------

After you have played some moves with michi, you may want to know what is its
estimate of the position. You can obtain this information from commands 
accessible in the *Analyze commands* window (as in fig6. below).

.. figure:: img/img6.png
    :scale: 75 %
    :align: center

    fig6. Analyze commands

    ..

If this window is not visible, you can obtain it by selecting the 
"Analyze commands" entry in the Tool Menu.

.. figure:: img/img7.png
    :scale: 75 %
    :align: center

    fig7. Make the Analyze Commands window visible

    ..

Any of the analyze commands can be run either by double clicking on it or by 
selecting a command and clicking on the *Run* button.
It is also possible to have a command executed automatically after each move.
 
The first three commands are for changing parameters that control the behavior
of michi-c. They will be described in next subsection (2.4).

The other commands give meaningful answer only just after michi-c has played 
its move.

*Status Dead* 

    mark the stones estimated as dead by michi-c 
    (not always accurate as you will notice).

*Principal variation*
    shows the next 5 best continuation as estimated by mcts.

.. figure:: img/img14.png
    :scale: 75 %
    :align: center

    fig8. Best sequence

    ..

*Best moves*
    shows the winrate of the 5 best moves (i.e most visited by MCTS)

.. figure:: img/img13.png
    :scale: 75 %
    :align: center

    fig9. Best moves

    ..

*Owner_Map*
    represent for each intersection the percentage of white or black possession
    at the end of the playouts (big black square = 100 % black possession,
    big white square = 100 % white possession, nothing = around 50 %)

.. figure:: img/img11.png
    :scale: 75 %
    :align: center

    fig10. Owner Map

    ..

*Visit Count*
    represent the number of visits for each move in the root node of MCTS.
    The size of the square is maximum for the most visited move and the 
    surface of each square is proportional to the visit count (discretized by
    step of 10 %, so 0%, 1%, .., 5% are the same
    , 6 %, 7%, ..., 15 % the same , etc.)

.. figure:: img/img12.png
    :scale: 75 %
    :align: center

    fig11. Visit count

    ..

*Histogram of scores*
    This is a primitive representation of the histogram of the playout scores.
    Should find a good way to show a beautiful python graphic in next version.

.. figure:: img/img15.png
    :scale: 75 %
    :align: center

    fig12. Primitive view of histogram of playouts scores

    ..

2.3 Live graphics
-----------------

All the graphics commands (marked with a gfx prefix in the *Analyze commands*
windows) can also be updated at regular intervals
during the search providing an animation that can be fun to watch.

This is done by setting the *Live gfx* and *Live gfx interval* parameters in
the *General Parameters* setting window as seen on the figure 12 below.

2.4 Changing the michi-c parameters
-----------------------------------

By running one of the three commands *General Parameters*, 
*Tree Policy Parameters* or *Random Policy Parameters* you will get a new
window (respectively shown in fig.12, fig.13 and fig.14) that will allow you 
to change the parameters at any moment when michi is not thinking.

The modification process is natural and does not need any explanation.
It takes place after you click on OK.

There has not been any particular thought about the order of the parameters 
and it could certainly be improved.

.. figure:: img/img8.png
    :scale: 75 %
    :align: center

    fig12. General Parameters settings

    ..

Definitions:: 

    use_dynamic_komi
        dynamic komi is used in the current version of michi-c only for 
        handicap games (linear version). It can be enabled or disabled.

    komi_per_handicap_stone
        this value multiplied by the number of handicap stones will be the
        delta komi at the beginning of the game

    play_until_the_end
        when checked this option disallows early passing (useful on cgos)

    random_seed
        random seed. Should normally be a positive integer 
        -1 generate a true random seed that will be different each time michi-c is restarted

    REPORT_PERIOD
        number of playouts between each report by michi on the standard output
        note: useful only if verbosity > 0

    verbosity
        0, 1 or 2 : control the verbosity of michi on the standard output

    RESIGN_THRES
        winrate threshold (in [0.0,1.0]. 
        When the winrate becomes below this threshold, michi resign

    FASTPLAY20_THRES
        if at 20% playouts winrate is > FASTPLAY20_THRES, stop reading

    FASTPLAY5_THRES
        if at 5% playouts winrate is > FASTPLAY5_THRES, stop reading

    Live_gfx
        None, best_moves, owner_map, principal_variation or visit_count
        if different from None, gogui will display at regular intervals the
        same graphics as in figures 8 to 11.

    Live_gfx_interval
        the interval (number of playouts) between live graphics refresh


.. figure:: img/img9.png
    :scale: 75 %
    :align: center

    fig13. Tree Policy Parameters settings

    ..

Definitions:: 

    N_SIMS
        Number of simulations per move (search).
        Note: michi-c use a different value when playing with time limited constraints        

    RAVE_EQUIV
        number of visits which makes the weight of RAVE simulations and real simulations equal

    EXPAND_VISITS
        number of visits before a node is expanded

    PRIOR_EVEN
        should be even. This represent a 0.5 prior

    PRIOR_SELFATARI
        NEGATIVE prior if the move is a self-atari

    PRIOR_CAPTURE_ONE
        prior if the move captures one stone

    PRIOR_CAPTURE_MANY
        prior if the move captures many stones

    PRIOR_PAT3
        prior if the move match a 3x3 pattern

    PRIOR_LARGEPATTERN
        multiplier for the large patterns probability
        note: most moves have relatively small probability

    PRIOR_CFG[]
        prior for moves in cfg distance 1, 2, 3

    PRIOR_EMPTYAREA
        prior for moves in empty area.
        Negative for move on the first and second lines
        Positive for move on the third and fourth lines


.. figure:: img/img10.png
    :scale: 75 %
    :align: center

    fig14. Random Policy Parameters settings

    ..

Definitions::

    PROB_HEURISTIC_CAPTURE (0.90)
        probability of heuristic capture suggestions being taken in playout

    PROB_HEURISTIC_PAT3 (0.95)
        probability of heuristic 3x3 pattern suggestions being taken in playout

    PROB_SSAREJECT (0.90)
        probability of rejecting suggested self-atari in playout

    PROB_RSAREJECT (0.50)
        probability of rejecting random self-atari in playout
        this is lower than above to allow nakade


3. Reference of michi-c gtp commands
************************************

The list of all gtp commands understood by michi-c (version 1.4) is
described in the following sections.

3.1 Standard gtp commands
-------------------------

The following standard gtp commands are implemented. 
Please refer to [1] for the specification of each command.

    *protocol_version,
    name,
    version,
    known_command,
    list_commands,
    quit,
    boardsize,
    clear_board,
    komi,
    play,
    genmove,
    set_free_handicap,
    loadsgf,
    time_left,
    time_settings,
    final_score,
    final_status_list,
    undo*

The standard is implemented exept for 

*time_settings*
    only absolute time setting is implemented yet

*loadsgf*
    michi-c can only read simple SGF files, i.e. files with no
    variations nor games collections  (see [3])
    but this is not carefully checked so
    expect some crash if you try to play with the limits.

These limitations will be removed in some next release.

3.2 Gogui specific commands (or extensions used by gogui)
---------------------------------------------------------

Please refer to [2] for the specification of each command.

    *gogui-analyze_commands,
    gogui-play_sequence,
    gogui-setup,
    gogui-setup_player,
    gg-undo,
    kgs-genmove_cleanup*

3.3 Commands to get or set parameters
-------------------------------------

Maybe the most important commands to know from a user point of view are the
three commands *param_general*, *param_playout* and *param_tree* that allow us
to change the parameters controlling the behavior of michi-c during the game.

If you give no argument to the command it will simply print the current value
of all the parameters it controls, else you must give two arguments : the name
of a parameter and the value you want to give to it.

If the given name is not known from the command, it is ignored and the command
behaves as if it was called without argument.

The names recognized by these commands are those described in section 2.4.

*param_general*::

    Function:  Read or write internal parameters of michi-c (General)
    Arguments: name value or nothing
    Fails:     No value (if only a name was given)
    Returns:   A string formatted for gogui analyze command of param type

*param_playout*::

    Function:  Read or write internal parameters of michi-c (Playout Policy)
    Arguments: name value or nothing
    Fails:     No value (if only a name was given)
    Returns:   A string formatted for gogui analyze command of param type

*param_tree*::

    Function:  Read or write internal parameters of michi-c (Tree Policy)
    Arguments: name value or nothing
    Fails:     No value (if only a name was given)
    Returns:   A string formatted for gogui analyze command of param type



3.4 Rest of michi-c specific commands
-------------------------------------

*best_moves*::

    Function:  Build a list of the 5 best moves (5 couples: point winrate)
    Arguments: none
    Fails:     never
    Returns:   A string formatted for gogui analyze command of pspair type

*cputime*
    This is a command used by the gogui tool gogui-twogtp.
    It returns a time in seconds, whose origin is unspecified.

*debug <subcmd>*
    This command is only used for debugging purpose and regression testing.
    People that need it are able to read the code and I believe it is not
    necessary to make this manual longer in order to describe it.

*help*
    This is a synonym for list_commands

*owner_map*::

    Function:  Compute a value in [-1,1] for each point: -1 = 100% white, 1=100 % black
    Arguments: none
    Fails:     never
    Returns:   A string formatted for gogui analyze command of gfx/INFLUENCE type

*principal_variation*::

    Function:  Compute the best sequence (5 moves) from the current position
    Arguments: none
    Fails:     never
    Returns:   A string formatted for gogui analyze command of gfx VAR type


*score_histogram*::

    Function:  Build histogram of playout scores 
    Arguments: none
    Fails:     never
    Returns:   A string formatted for gogui analyze command of hstring type

*visit_count*::

    Function:  Compute a value in [0,1] for each point: 0 = never, 1= most visited
    Arguments: none
    Fails:     never
    Returns:   A string formatted for gogui analyze command of gfx/INFLUENCE type

4. Michi-c command line arguments
*********************************

When michi is run from the command line without any parameter or as::

    $ ./michi -h

it will write a simple usage message::

    usage: michi mode [config.gtp]

    where mode = 
       * gtp         play gtp commands read from standard input
       * mcbenchmark run a series of playouts (number set in config.gtp)
       * mcdebug     run a series of playouts (verbose, nb of sims as above)
       * tsdebug     run a series of tree searches
       * defaults    write a template of config file on stdout (defaults values)
    and
       * config.gtp  an optional file containing gtp commands


The most commonly used mode values are **gtp** and **defaults**.

Mode **gtp** launches the gtp loop that will be ended by sending the 
command *quit* to michi. 

Mode **defaults** print on the standard output the current default value 
of every modifiable parameter in michi and leave immediately.::

    param_general                 use_dynamic_komi 0
    param_general          komi_per_handicap_stone 7.0
    param_general               play_until_the_end 0
    param_general                      random_seed 1
    param_general                    REPORT_PERIOD 200000
    param_general                        verbosity 2
    param_general                     RESIGN_THRES 0.20
    param_general                 FASTPLAY20_THRES 0.80
    param_general                  FASTPLAY5_THRES 0.95
    param_general                         Live_gfx None
    param_general                Live_gfx_interval 1000
    param_tree                              N_SIMS 2000
    param_tree                          RAVE_EQUIV 3500
    param_tree                       EXPAND_VISITS 8
    param_tree                          PRIOR_EVEN 10
    param_tree                     PRIOR_SELFATARI 10
    param_tree                   PRIOR_CAPTURE_ONE 15
    param_tree                  PRIOR_CAPTURE_MANY 30
    param_tree                          PRIOR_PAT3 10
    param_tree                  PRIOR_LARGEPATTERN 100
    param_tree                        PRIOR_CFG[0] 24
    param_tree                        PRIOR_CFG[1] 22
    param_tree                        PRIOR_CFG[2] 8
    param_tree                     PRIOR_EMPTYAREA 10
    param_playout           PROB_HEURISTIC_CAPTURE 0.90
    param_playout              PROB_HEURISTIC_PAT3 0.95
    param_playout                   PROB_SSAREJECT 0.90
    param_playout                   PROB_RSAREJECT 0.50

This list is interesting in itself.

In addition by redirecting it to a file, you obtain a configuration file that
you can use as the optional parameter (config.gtp). 

When michi is used in **gtp** mode with this second argument, all the gtp 
commands placed in the config.gtp file will be executed at initialization.

This feature can be used to modify the default parameters or for example to 
load a given position from a SGF file.

The three other modes **mcbenchmark**, **mcdebug** or **tsdebug** are, as their
name suggest useful only for debugging and benchmarking.
