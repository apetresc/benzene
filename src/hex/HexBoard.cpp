//----------------------------------------------------------------------------
// $Id: HexBoard.cpp 1699 2008-09-24 23:55:56Z broderic $
//----------------------------------------------------------------------------

#include "Time.hpp"
#include "BoardUtils.hpp"
#include "BitsetIterator.hpp"
#include "HexBoard.hpp"
#include "VCUtils.hpp"

//----------------------------------------------------------------------------

HexBoard::HexBoard(int size, const ICEngine& ice)
    : PatternBoard(size), 
      m_ice(ice)
{
    init();
}

HexBoard::HexBoard(int width, int height, const ICEngine& ice)
    : PatternBoard(width, height), 
      m_ice(ice)
{
    init();
}

void HexBoard::init()
{
    hex::log << hex::fine << "--- HexBoard" << hex::endl;
    hex::log << "sizeof(HexBoard) = " << sizeof(HexBoard) << hex::endl;

    for (BWIterator c; c; ++c) 
    {
        m_cons[*c].reset(new Connections(*this, *c, 
                                         VCPattern::GetPatterns(width(), 
                                                                height(), 
                                                                *c),
                                         m_log[*c]));

    }

    ClearHistory();
}

HexBoard::~HexBoard()
{
}

//----------------------------------------------------------------------------

bitset_t HexBoard::getMustplay(HexColor color) const
{
    bitset_t empty;
    HexColor other = !color;
    HexPoint edge1 = HexPointUtil::colorEdge1(other);
    HexPoint edge2 = HexPointUtil::colorEdge2(other);

    if (Cons(other).doesVCExist(edge1, edge2, VC::FULL)) {
        return empty;
    }

    const VCList& semi = Cons(other).getVCList(edge1, edge2, VC::SEMI);
    bitset_t intersection = semi.hardIntersection();
    intersection &= getEmpty(); // FIXME: need this line?

    return intersection;
}

//----------------------------------------------------------------------------

void HexBoard::ComputeInferiorCells(HexColor color_to_move, 
                                    EndgameFillin endgame_mode)
{
    if (m_settings.flags & USE_ICE) {
        InferiorCells inf;
        m_ice.computeInferiorCells(color_to_move, *this, inf);

        if (endgame_mode == REMOVE_WINNING_FILLIN && isGameOver()) {
            HexColor winner = getWinner();
            HexAssert(winner != EMPTY);
        
            hex::log << hex::fine 
                     << "Captured cells caused win! Removing..." 
                     << hex::endl;

            // Need to remove winner's captured, since may cause game
            // to be over, resulting in a finished game with no
            // available moves.  Likewise, after removing the captured
            // the dead may also provide a win, so we need to remove
            // them as well. This is because the dead are a function
            // of the captured.
            removeColor(winner, inf.Captured(winner));
            removeColor(winner, inf.PermInf(winner));
            removeColor(DEAD_COLOR, inf.Dead());

            inf.ClearPermInf(winner);
            inf.ClearCaptured(winner);
            inf.ClearDead();
            
            update();
            absorb();
            
            HexAssert(!isGameOver());
        } 

        // update the set of inferior cells with this new data
        IceUtil::Update(m_inf, inf, *this);

        // clear the set of backed-up domination arcs
        m_backedup.clear();
    }
}

void HexBoard::BuildVCs(int max_ors)
{
    for (BWIterator c; c; ++c)
        m_cons[*c]->build(max_ors);
}

void HexBoard::BuildVCs(bitset_t added[BLACK_AND_WHITE], int max_ors,
                        bool mark_the_log)
{
    HexAssert((added[BLACK] & added[WHITE]).none());
    for (BWIterator c; c; ++c)
        m_cons[*c]->build(added, max_ors, mark_the_log);
}

void HexBoard::RevertVCs()
{
    for (BWIterator c; c; ++c)
        m_cons[*c]->revert();
}

void HexBoard::HandleVCDecomposition(HexColor color_to_move, 
                                     EndgameFillin endgame_mode, 
                                     int max_ors)
{
    // do nothing if decomps are turned off for this board. */
    if (!(m_settings.flags & USE_DECOMPOSITIONS)) {
        hex::log << hex::fine << "Skipping decomp check." << hex::endl;
        return;
    }

    /** @todo check for a vc win/loss here instead of 
	just solid chains. */
    if (isGameOver()) {
        hex::log << hex::fine 
                 << "Game is over; skipping decomp check." 
                 << hex::endl;
        return;
    }

    int decompositions = 0;
    for (;;) {
        bool found = false;
        for (BWIterator c; c; ++c) {
            bitset_t captured;
            if (BoardUtils::FindCombinatorialDecomposition(*this, *c,
							   captured))
            {
                hex::log << hex::fine;
                hex::log << "Decomposition " << decompositions << ": for " 
                         << *c << "." << hex::endl;
                hex::log << printBitset(captured) << hex::endl;
            
                AddStones(*c, captured, color_to_move, endgame_mode, max_ors);
                m_inf.AddCaptured(*c, captured);
            
                hex::log << hex::fine
                         << "After decomposition " << decompositions 
                         << ": " << *this 
                         << hex::endl;
                
                decompositions++;
                found = true;
                break;
            } 
        }
        if (!found) break;
    }

    hex::log << hex::fine << "Found " << decompositions << " decompositions."
             << hex::endl;
}

void HexBoard::ComputeAll(HexColor color_to_move, 
                          EndgameFillin endgame_mode, 
                          int max_ors)
{
    double s = HexGetTime();
    
    update();
    absorb();
    m_inf.Clear();

    bitset_t old_black = getColor(BLACK);
    bitset_t old_white = getColor(WHITE);
    ComputeInferiorCells(color_to_move, endgame_mode);

    bitset_t added[BLACK_AND_WHITE];
    added[BLACK] = getColor(BLACK) - old_black;
    added[WHITE] = getColor(WHITE) - old_white;
    
    if (m_settings.flags & USE_VCS) {
        BuildVCs(max_ors);
        HandleVCDecomposition(color_to_move, endgame_mode, max_ors);
    }

    double e = HexGetTime();
    hex::log << hex::fine << (e-s) << "s to compute all." << hex::endl;
}

void HexBoard::PlayMove(HexColor color, HexPoint cell, int max_ors)
{
    double s = HexGetTime();
    hex::log << hex::fine << "Playing (" << color << ", " 
             << cell << ")" << hex::endl;

    PushHistory(color, cell);

    bitset_t old_black = getColor(BLACK);
    bitset_t old_white = getColor(WHITE);

    playMove(color, cell);
    update(cell);
    absorb(cell);

    ComputeInferiorCells(!color, DO_NOT_REMOVE_WINNING_FILLIN);

    bitset_t added[BLACK_AND_WHITE];
    added[BLACK] = getColor(BLACK) - old_black;
    added[WHITE] = getColor(WHITE) - old_white;

    if (m_settings.flags & USE_VCS) {
        BuildVCs(added, max_ors);
        HandleVCDecomposition(!color, DO_NOT_REMOVE_WINNING_FILLIN, max_ors);
    }

    double e = HexGetTime();
    hex::log << hex::fine << (e-s) << "s to play stones." << hex::endl;
}

void HexBoard::PlayStones(HexColor color, const bitset_t& played,
                          HexColor color_to_move,
                          int max_ors)
{
    HexAssert(BitsetUtil::IsSubsetOf(played, getEmpty()));

    double s = HexGetTime();
    hex::log << hex::fine << "Playing (" << color << ","
             << HexPointUtil::ToPointListString(played) << ")"
             << hex::endl;

    PushHistory(color, INVALID_POINT);

    bitset_t old_black = getColor(BLACK);
    bitset_t old_white = getColor(WHITE);

    addColor(color, played);
    update(played);
    absorb(played);

    ComputeInferiorCells(color_to_move, DO_NOT_REMOVE_WINNING_FILLIN);

    bitset_t added[BLACK_AND_WHITE];
    added[BLACK] = getColor(BLACK) - old_black;
    added[WHITE] = getColor(WHITE) - old_white;

    if (m_settings.flags & USE_VCS) {
        BuildVCs(added, max_ors);
        HandleVCDecomposition(color_to_move, 
                              DO_NOT_REMOVE_WINNING_FILLIN, 
                              max_ors);
    }

    double e = HexGetTime();
    hex::log << hex::fine << (e-s) << "s to play stones." << hex::endl;
}

void HexBoard::AddStones(HexColor color, const bitset_t& played,
                         HexColor color_to_move, 
                         EndgameFillin endgame_mode, 
                         int max_ors)
{
    HexAssert(BitsetUtil::IsSubsetOf(played, getEmpty()));

    double s = HexGetTime();
    hex::log << hex::fine << "Adding (" << color << ", "
             << HexPointUtil::ToPointListString(played) << ")"
             << hex::endl;

    bitset_t old_black = getColor(BLACK);
    bitset_t old_white = getColor(WHITE);

    addColor(color, played);
    update(played);
    absorb(played);

    ComputeInferiorCells(color_to_move, endgame_mode);

    bitset_t added[BLACK_AND_WHITE];
    added[BLACK] = getColor(BLACK) - old_black;
    added[WHITE] = getColor(WHITE) - old_white;

    if (m_settings.flags & USE_VCS) {
        BuildVCs(added, max_ors, Connections::DO_NOT_MARK_LOG); 
    }

    double e = HexGetTime();
    hex::log << hex::fine << (e-s) << "s to add stones." << hex::endl;
}

void HexBoard::UndoMove()
{
    double s = HexGetTime();

    PopHistory();
    update();
    absorb();
    
    double e = HexGetTime();
    hex::log << hex::fine << (e-s) << "s to undo move." << hex::endl;
}

//----------------------------------------------------------------------------

void HexBoard::ClearHistory()
{
    m_history.clear();
}

void HexBoard::PushHistory(HexColor color, HexPoint cell)
{
    History hist(*this, m_inf, m_backedup, color, cell);
    m_history.push_back(hist);
}

void HexBoard::PopHistory()
{
    HexAssert(!m_history.empty());

    History hist = m_history.back();
    m_history.pop_back();

    // restore the old board position
    startNewGame();
    setColor(BLACK, hist.board.getBlack());
    setColor(WHITE, hist.board.getWhite());
    setPlayed(hist.board.getPlayed());

    // backup the ic info
    if (hist.last_played != INVALID_POINT 
        && hex::settings.get_bool("global-backup-ice-info")) 
    {
        bitset_t a = getEmpty() - hist.inf.All();
        a &= m_inf.Dead() | m_inf.Captured(hist.to_play);

        for (BitsetIterator p(a); p; ++p) {
            hist.inf.AddDominated(*p, hist.last_played);
            hist.backedup.insert(std::make_pair(*p, hist.last_played));
        }
    }

    m_inf = hist.inf;
    m_backedup = hist.backedup;

    RevertVCs();
}

void HexBoard::AddDominationArcs(const std::set<HexPointPair>& dom)
{
    std::set<HexPointPair>::const_iterator it;
    for (it=dom.begin(); it!=dom.end(); ++it) {
        m_inf.AddDominated(it->first, it->second);
    }
}

//----------------------------------------------------------------------------
