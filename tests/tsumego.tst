#-------------------------------------------------------------
# tests of the bgo mcts subsystem (testing answer to tsumegos)
#-------------------------------------------------------------

param_general play_until_the_end 1

# Simple tests
# ------------
loadsgf sgf/tsumego/simple.sgf

010 genmove b
#? [E5]

loadsgf sgf/tsumego/simple.sgf

play b pass
015 genmove w
#? [E5]

loadsgf sgf/tsumego/simple_white.sgf

020 genmove w
#? [E5]

loadsgf sgf/tsumego/simple_white.sgf
play w pass
025 genmove b
#? [E5]

loadsgf sgf/tsumego/simple_4spaces.sgf

030 genmove w
#? [resign]

play w pass
033 genmove b
#? [B1|C1]

loadsgf sgf/tsumego/simple_4spaces.sgf
play b C1
036 genmove W
#? [resign]

# Seki detection
# --------------
loadsgf sgf/tsumego/simple_seki.sgf
040 genmove w
#? [pass]

loadsgf sgf/tsumego/simple_seki.sgf
play w pass
045 genmove b
#? [pass|resign]
