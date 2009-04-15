#-----------------------------------------------------------------------------
# play.tst
#
# Play a couple of quick moves as wolve and as mohex. 
#
# This test is mainly for checking that no assertion triggers.
#
# $Id: play.tst 664 2007-08-29 22:43:26Z broderic $
#-----------------------------------------------------------------------------

boardsize 8 8 
player-search-type ab
ab-max-depth 1

10 genmove b
#? [[a-k]([1-9]|1[0-1])]

20 genmove w
#? [[a-k]([1-9]|1[0-1])]

30 genmove b
#? [[a-k]([1-9]|1[0-1])]

40 genmove w
#? [[a-k]([1-9]|1[0-1])]

50 genmove b
#? [[a-k]([1-9]|1[0-1])]

60 genmove w
#? [[a-k]([1-9]|1[0-1])]

70 genmove b
#? [[a-k]([1-9]|1[0-1])]

80 genmove w
#? [[a-k]([1-9]|1[0-1])]

90 genmove b
#? [[a-k]([1-9]|1[0-1])]

100 genmove w
#? [[a-k]([1-9]|1[0-1])]

#-----------------------------------------------------------------------------
boardsize 11 11
player-search-type uct
uct-scale-num-games-to-size false
uct-num-games 1000

110 genmove b
#? [[a-k]([1-9]|1[0-1])]

120 genmove w
#? [[a-k]([1-9]|1[0-1])]

130 genmove b
#? [[a-k]([1-9]|1[0-1])]

140 genmove w
#? [[a-k]([1-9]|1[0-1])]

150 genmove b
#? [[a-k]([1-9]|1[0-1])]

160 genmove w
#? [[a-k]([1-9]|1[0-1])]

170 genmove b
#? [[a-k]([1-9]|1[0-1])]

180 genmove w
#? [[a-k]([1-9]|1[0-1])]

190 genmove b
#? [[a-k]([1-9]|1[0-1])]

200 genmove w
#? [[a-k]([1-9]|1[0-1])]
