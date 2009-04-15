#-----------------------------------------------------------------------------
# Tests solver engine.
#
#
# Note 1: solver-find-winning will crash on many of the tests below
#         because of MovesToConsider asserts. This needs to be fixed, and
#         the solutions below should be changed to winning moves rather
#         than simply winning colour.
#
# Note 2: Later on we should have tests for speed as well, not just
#         correctness.
#
#-----------------------------------------------------------------------------

# Bert Enderton puzzles

loadsgf sgf/solver/puzzle01.sgf
10 solver-find-winning white
#? [d5]

loadsgf sgf/solver/puzzle02.sgf
11 solve-state white
#? [white]

loadsgf sgf/solver/puzzle03.sgf
12 solver-find-winning white
#? [e3 e6]

loadsgf sgf/solver/puzzle04.sgf
13 solve-state black
#? [black]


# Piet Hein puzzles

loadsgf sgf/solver/puzzle05.sgf
20 solve-state white
#? [white]

loadsgf sgf/solver/puzzle06.sgf
21 solve-state white
#? [white]

loadsgf sgf/solver/puzzle07.sgf
22 solve-state white
#? [white]

loadsgf sgf/solver/puzzle08.sgf
23 solve-state white
#? [white]

loadsgf sgf/solver/puzzle09.sgf
24 solve-state white
#? [white]

loadsgf sgf/solver/puzzle10.sgf
25 solve-state white
#? [white]

loadsgf sgf/solver/puzzle15.sgf
26 solve-state white
#? [white]


# Cameron Browne puzzles

loadsgf sgf/solver/puzzle11.sgf
40 solve-state black
#? [black]

loadsgf sgf/solver/puzzle12.sgf
41 solve-state white
#? [white]

loadsgf sgf/solver/puzzle13.sgf
42 solve-state white
#? [white]


# Ryan Hayward puzzles

loadsgf sgf/solver/puzzle14.sgf
63 solve-state white
#? [white]
