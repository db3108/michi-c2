#-------------------------------------------
# tests of undo in the michi board subsystem
#-------------------------------------------

boardsize 13

# -------------------
# undo block creation
# -------------------
play b c4
undo
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

# --------------------
# undo block extension
# --------------------
clear_board
debug setpos c4 d4
play b c5
undo
210 debug blocks_OK c5
#? [true]
debug setpos a1 b1 pass b2
play b a2
undo
220 debug blocks_OK a2
#? [true]
debug setpos n2 m1 m2  
play w n1
undo
230 debug blocks_OK n1
#? [true]

# ----------------
# undo block merge
# ----------------
debug setpos k1 a3
play w l1
undo
310 debug blocks_OK l1
#? [true]
play w e5
play b a2
undo
320 debug blocks_OK a2
#? [true]
debug setpos a9 f4 a8 e3 a7
play w e4
undo
320 debug blocks_OK e4
#? [true]

# ------------------
# undo block capture
# ------------------
clear_board
debug setpos b1 a1
play b a2
undo
410 debug blocks_OK a2
#? [true]
debug setpos n1 n2 m1 m2 l1 l2 k1 k2 pass 
play w j1
undo
420 debug blocks_OK j1
#? [true]
debug setpos b2 pass a3
play b a2
430 debug pos ko
#? [A1]
undo
431 debug blocks_OK a2
#? [true]
432 debug pos ko
#? []
debug setpos a2 pass pass a1
440 debug pos ko
#? [A2]
play b a9
undo
441 debug blocks_OK a1
#? [true]
442 debug pos ko
#? [A2]
play b pass
undo
450 debug blocks_OK a1
#? [true]

# ---------------------------
# Multiple undo in succession
# ---------------------------

clear_board
debug setpos b1 a1 a9

510 debug pos last
#? [A9]
511 debug pos last2
#? [A1]
512 debug pos last3
#? [B1]
513 debug pos ko
#? []

undo
520 debug pos last
#? [A1]
521 debug pos last2
#? [B1]
522 debug pos last3
#? [B1]
523 debug pos ko
#? []

play b a2
undo
530 debug pos last
#? [A1]
531 debug pos last2
#? [B1]
532 debug pos last3
#? [B1]
533 debug pos ko
#? []

play b a9
play w a3
play b a8
play w b2
play b a2
play w j1
play b j2
play w a1
play b h1
play w c1

undo
540 debug pos last
#? [H1]
541 debug pos last2
#? [A1]
542 debug pos last3
#? [J2]
543 debug pos ko
#? []

undo
550 debug pos last
#? [A1]
551 debug pos last2
#? [J2]
552 debug pos last3
#? [J1]
553 debug pos ko
#? [A2]

undo
560 debug pos last
#? [J2]
561 debug pos last2
#? [J1]
562 debug pos last3
#? [A2]
563 debug pos ko
#? []

undo
undo
570 debug pos last
#? [A2]
571 debug pos last2
#? [B2]
572 debug pos last3
#? [A8]
573 debug pos ko
#? [A1]

# --------------------------------------------
# undo moves with the same color in succession
# --------------------------------------------
play b g7
play b g6
undo
600 debug pos to_play
#? [WHITE]

