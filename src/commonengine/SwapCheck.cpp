//----------------------------------------------------------------------------
/** @file SwapCheck.cpp
 */
//----------------------------------------------------------------------------

#include "BoardUtils.hpp"
#include "SwapCheck.hpp"

using namespace benzene;

//----------------------------------------------------------------------------

namespace {

//----------------------------------------------------------------------------

bool s_swapLoaded = false;
    
/** Contains moves to swap for each boardsize. 
    Use strings of the form "nxn" to index the map for an (n, n)
    board. */
std::map<std::string, std::set<HexPoint> > s_swapMoves;

/** Loads swap moves for each boardsize from the given file.
    Ignores lines begining with '#'. On lines not begining with '#',
    expects a string of the form "nxn" and the name of a HexPoint:
    this pair denotes a move to swap on an nxn board. The remainder of
    the line is not looked at. */
void LoadSwapMoves(const std::string& name)
{
    using namespace boost::filesystem;
    path swap_path = path(ABS_TOP_SRCDIR) / "share";
    path swap_list = swap_path / name;
    swap_list.normalize();
    std::string swap_file = swap_list.string();
    LogInfo() << "SwapCheck: Loading swap moves: '" << swap_file << "'...\n";
    s_swapMoves.clear();
    std::ifstream s(swap_file.c_str());
    if (!s)
        throw BenzeneException("SwapCheck: could not open list!\n");
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(s, line))
    {
        lineNumber++;
        if (line[0] == '#')
            continue;
        if (line.size() < 6) // skip (nearly) empty lines
            continue;
        std::string boardSizeStr;
        std::string pointStr;
        std::istringstream ss(line);
        ss >> boardSizeStr;
        ss >> pointStr;
        HexPoint point = HexPointUtil::FromString(pointStr);
        if (point == INVALID_POINT)
            LogWarning() << "SwapCheck: line " << lineNumber 
                         << ": invalid cell!\n";
        else
            s_swapMoves[boardSizeStr].insert(point);
    }
    s.close();
    s_swapLoaded = true;
}

//----------------------------------------------------------------------------

} // anonymous namespace

//----------------------------------------------------------------------------

bool SwapCheck::PlaySwap(const Game& gameState, HexColor toPlay)
{
    if (gameState.AllowSwap()
        && (1 == gameState.History().size())
        && (!FIRST_TO_PLAY == toPlay))
    {
        const StoneBoard& brd = gameState.Board();
	HexAssert(1 == brd.NumStones());
	
	// If board has unequal dimensions, we want to traverse the
	// shorter distance.
	if (brd.Width() != brd.Height()) 
        {
	    if ((brd.Width() > brd.Height() && toPlay == !VERTICAL_COLOR) ||
		(brd.Width() < brd.Height() && toPlay == VERTICAL_COLOR))
            {
                LogInfo() << "SwapCheck: swapping to get shorter side.\n";
		return true;
            }
	}
        else 
        {
            if (!s_swapLoaded)
                LoadSwapMoves("swap-moves.txt");
	    HexPoint firstMove = gameState.History().back().point();
	    if (toPlay == VERTICAL_COLOR)
                // Swap decisions assume VERTICAL_COLOR was FIRST_TO_PLAY,
                // so we mirror the first move if this is not the case
                // (i.e. to consider an equivalent decision).
                firstMove = BoardUtils::Mirror(brd.Const(), firstMove);
            std::ostringstream os;
            os << brd.Width() << 'x' << brd.Height();
            if (s_swapMoves[os.str()].count(firstMove) > 0)
            {
                LogInfo() << "SwapCheck: playing swap.\n";
                return true;
            }
        }
        LogInfo() << "SwapCheck: opting not to swap.\n";
    }
    return false;
}

//----------------------------------------------------------------------------
