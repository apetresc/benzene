#-----------------------------------------------------------------------------
# Tests VC computations.
#
# Expected results are based on (svn commit 1634):
#     vc-and-over-edge=true
#     vc-use-push-rule=true 
#     vc-use-patterns=true
#     vc-full-soft-limit=40
#     vc-semi-soft-limit=50
#
# $Id: vc.tst 1657 2008-09-15 23:32:09Z broderic $
#-----------------------------------------------------------------------------

#
# Must first turn off decompositions for VC tests, so that the
# VC/SC endpoints remain in different groups.
#
global-use-decompositions false


#
# Checking if we detect basic edge patterns.
#
loadsgf sgf/vc/check-432-to-edge.sgf
vc-build black
10 vc-connected-to north black 0
#? [.*a3.*b3.*f3.*i3.*]

loadsgf sgf/vc/check-7652-to-edge.sgf
vc-build black
11 vc-connected-to north black 0
#? [.*e4.*]

loadsgf sgf/vc/check-109853-to-edge.sgf
vc-build black
12 vc-connected-to south black 0
#? [.*g7.*]

loadsgf sgf/vc/obvious-ladder.sgf
vc-build black
13 vc-connected-to south black 0
#? [.*north.*]


#
# Checking if we can find tricky connections to an edge.
# "DK" indicates a template taken from David King's website.
#
loadsgf sgf/vc/ladder-VC-to-north.sgf
vc-build black
20 vc-connected-to g4 black 0
#? [.*north.*]
21 vc-connected-to g4 black 0
#? [!.*south.*]
22 vc-connected-to g4 black 1
#? [.*south.*]

loadsgf sgf/vc/b10-is-key.sgf
vc-build white
23 vc-connected-to d4 white 0
#? [.*west.*]

loadsgf sgf/vc/b10-is-key2.sgf
vc-build black
24 vc-connected-to f8 black 0
#? [.*south.*]*

loadsgf sgf/vc/non-Anshelevich-edge-connection1.sgf
vc-build black
25 vc-connected-to e4 black 0
#? [.*south.*]

loadsgf sgf/vc/non-Anshelevich-edge-connection2.sgf
vc-build white
26 vc-connected-to h3 white 0
#? [.*east.*]

loadsgf sgf/vc/black-i4-does-not-block.sgf
vc-build white
27 vc-connected-to h3 white 0
#? [.*east.*]

loadsgf sgf/vc/e3-has-VC-to-west-edge.sgf
vc-build white
28 vc-connected-to e3 white 0
#? [.*west.*]

loadsgf sgf/vc/DK-3343.sgf
vc-build black
29 vc-connected-to f8 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-6543.sgf
vc-build black
30 vc-connected-to h8 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-65-12-3.sgf
vc-build black
31 vc-connected-to g8 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-29-10-9-6-3.sgf
vc-build black
32 vc-connected-to f8 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-98753.sgf
vc-build black
33 vc-connected-to d8 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-76642.sgf
vc-build black
34 vc-connected-to e7 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-76532.sgf
vc-build black
35 vc-connected-to f7 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-88763.sgf
vc-build black
36 vc-connected-to f7 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-65432.sgf
vc-build black
37 vc-connected-to h7 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-76543.sgf
vc-build black
38 vc-connected-to g7 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-76542.sgf
vc-build black
39 vc-connected-to h7 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-65432b.sgf
vc-build black
40 vc-connected-to g7 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-87631.sgf
vc-build black
41 vc-connected-to h7 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-654321.sgf
vc-build black
42 vc-connected-to h6 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-765432.sgf
vc-build black
43 vc-connected-to g6 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-765432b.sgf
vc-build black
44 vc-connected-to g6 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-655432.sgf
vc-build black
45 vc-connected-to h6 black 0
#? [.*south.*]

loadsgf sgf/vc/DK-766542.sgf
vc-build black
46 vc-connected-to f6 black 0
#? [.*south.*]

loadsgf sgf/vc/c4-has-VC-to-North.sgf
vc-build black
47 vc-connected-to c4 black 0
#? [.*north.*]

loadsgf sgf/vc/PH-6443.sgf
vc-build black
48 vc-connected-to south black 0
#? [.*e9.*]*


#
# Checking if we can find complicated winning SCs/VCs.
#
loadsgf sgf/vc/can-win-with-e7.sgf
vc-build white
50 vc-connected-to west white 1
#? [.*east.*]*
play white e7
vc-build white
51 vc-connected-to west white 0
#? [.*east.*]

loadsgf sgf/vc/j2-is-key.sgf
vc-build black
52 vc-connected-to south black 0
#? [.*north.*]

loadsgf sgf/vc/black-has-winning-SC-with-f7-key.sgf
vc-build black
53 vc-connected-to south black 1
#? [.*north.*]*

loadsgf sgf/vc/game-over-but-sees-4-mustplay.sgf
vc-build white
54 vc-connected-to east white 0
#? [.*west.*]*


#
# Other basic non-Anshelevich SCs/VCs.
#
loadsgf sgf/vc/small-non-Ansh-SC.sgf
vc-build black
60 vc-connected-to south black 1
#? [.*north.*]

loadsgf sgf/vc/non-Anshelevich-edge-connection3.sgf
vc-build black
61 vc-connected-to i8 black 0
#? [.*south.*]
