#-----------------------------------
# tests of the michi board subsystem
#-----------------------------------

boardsize 13

# initial board
# -------------
10 debug blocks_OK c4
#? [true]
20 debug blocks_OK a2
#? [true]
30 debug blocks_OK a13
#? [true]

# block creation
# --------------
play b c4
110 debug blocks_OK c4
#? [true]
play w d4
120 debug blocks_OK d4
#? [true]
play w a1
130 debug blocks_OK d4
#? [true]
play b a5
140 debug blocks_OK d4
#? [true]

# block extension
# ---------------
clear_board
debug setpos c4 d4
play b c5
210 debug blocks_OK c5
#? [true]
debug setpos a1 b1 pass b2
play w a2
220 debug blocks_OK a2
#? [true]
play b a4
play w a3
230 debug blocks_OK a3
#? [true]

# block merge
# -----------
clear_board
debug setpos c4 pass c6 pass
play b c5
310 debug blocks_OK c5
#? [true]
debug setpos f1 pass h1 pass g2 pass
play w g1
320 debug blocks_OK g1
#? [true]
debug setpos e5 pass f5 pass f4 pass e3 pass
play b e4
330 debug blocks_OK e4
#? [true]

# block capture
# -------------
clear_board
debug setpos a2 a1
play b b1
410 debug blocks_OK a1
#? [true]
debug setpos e5 e6 f6 f5 e7 f7
play w d6
420 debug blocks_OK d6
#? [true]
debug setpos g6 pass e4 pass d5 pass
play b e6
430 debug blocks_OK e6
#? [true]
debug setpos a8 a9 b8 b9 c9 c10 a11 b11 b10
play b a10
440 debug blocks_OK a10
#? [true]
play w b10
450 debug blocks_OK b10
#? [true]

# test suicide and ko
# -------------------
clear_board
debug setpos e5 pass f6 f5 e7 f7 d6
play w e6
510 debug blocks_OK e6
#? [true]
debug setpos g6
play w e6
520 debug blocks_OK e6
#? [true]
530 debug pos ko
#? [F6]

# Query block data
# ----------------
clear_board
debug setpos a1 e1 c4
610 debug libs 1
#? [A2 B1]
620 debug libs 2
#? [E2 D1 F1]
630 debug libs 3
#? [C5 B4 D4 C3]
