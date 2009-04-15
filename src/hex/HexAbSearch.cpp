//----------------------------------------------------------------------------
// $Id: HexAbSearch.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include <math.h>
#include "Hex.hpp"
#include "Time.hpp"
#include "HexAbSearch.hpp"
#include "HexBoard.hpp"
#include "PlayerUtils.hpp"
#include "SequenceHash.hpp"

//----------------------------------------------------------------------------

/** Local utilities. */
namespace
{

/** Dump state info so the gui can display progress. */
void DumpGuiFx(std::vector<HexMoveValue> finished, int num_to_explore,
               std::vector<HexPoint> pv, HexColor color)
{
    std::ostringstream os;
    os << "gogui-gfx:\n";
    os << "ab\n";
    os << "VAR";
    for (std::size_t i=0; i<pv.size(); ++i) 
    {
        os << " " << ((color == BLACK) ? "B" : "W");
        os << " " << pv[i];
        color = !color;
    }
    os << "\n";
    os << "LABEL";
    for (std::size_t i=0; i<finished.size(); ++i) 
    {
        os << " " << finished[i].point();
        HexEval value = finished[i].value();
        if (HexEvalUtil::IsWin(value))
            os << " W";
        else if (HexEvalUtil::IsLoss(value))
            os << " L";
        else 
            os << " " << std::fixed << std::setprecision(2) << value;
    }
    os << "\n";
    os << "TEXT";
    os << " " << finished.size() << "/" << num_to_explore;
    os << "\n";
    os << "\n";
    std::cout << os.str();
    std::cout.flush();
}

}

//----------------------------------------------------------------------------

HexAbSearch::HexAbSearch(HexBoard& board, HexColor color)
    : m_brd(board),
      m_toplay(color),
      m_tt((TransTable<SearchedState>*)0)
{
}

HexAbSearch::~HexAbSearch()
{
}

//----------------------------------------------------------------------------

void HexAbSearch::EnteredNewState() {}

void HexAbSearch::OnStartSearch() {}
    
void HexAbSearch::OnSearchComplete() {}

void HexAbSearch::AfterStateSearched() {}

HexEval HexAbSearch::Search(const std::vector<int>& plywidth, 
                            const std::vector<int>& depths_to_search, 
                            int timelimit,
                            std::vector<HexPoint>& pv)
{
    ClearStats();

    double start = HexGetTime();

    hex::log << hex::info << hex::endl;
    hex::log << "AlphaBeta Search Settings:" << hex::endl;
    hex::log << "Timelimit: " << timelimit << hex::endl;
    hex::log << "Iterative Deepening: " 
             << (depths_to_search.size()>1) << hex::endl;

    OnStartSearch();

    for (std::size_t d=0; d<depths_to_search.size(); ++d) {
        int depth = depths_to_search[d];

        hex::log << hex::info << "----- Depth " << depth << " -----" 
                 << hex::endl;

        /** @bug Revert back to previous ply's eval if we abort from
            the upcoming search before completion!. */
        m_eval.clear();
        m_current_depth = 0;
        m_sequence.clear();
        m_value = -EVAL_INFINITY;
        m_value = search_state(plywidth, depth, 
                               IMMEDIATE_LOSS, IMMEDIATE_WIN, 
                               pv);
        m_pv = pv;

        // print off the PV for this depth
        if (depth < (int)plywidth.size()) {
            hex::log << hex::info << DumpPV(m_value, m_pv) << hex::endl
                     << hex::endl;
        }
    }
    
    OnSearchComplete();

    double end = HexGetTime();
    m_elapsed_time = end - start;

    return m_value;
}

//----------------------------------------------------------------------------

HexEval HexAbSearch::CheckTerminalState()
{
    if (PlayerUtils::IsWonGame(m_brd, m_toplay))
        return IMMEDIATE_WIN - m_current_depth;
    
    if (PlayerUtils::IsLostGame(m_brd, m_toplay))
        return IMMEDIATE_LOSS + m_current_depth;

    return 0;
}

HexEval HexAbSearch::search_state(const std::vector<int>& plywidth,
                                  int depth, 
                                  HexEval alpha, HexEval beta,
                                  std::vector<HexPoint>& pv)
{
    HexAssert(m_current_depth + depth <= (int)plywidth.size());

    m_numstates++;
    pv.clear();

    // modify beta so that we abort on an immediate win
    beta = std::min(beta, IMMEDIATE_WIN - (m_current_depth+1));

    HexEval old_alpha = alpha;
    HexEval old_beta = beta;

    EnteredNewState();
    
    //
    // Check for terminal states
    //
    {
        HexEval value = CheckTerminalState();
        if (value != 0) {
            m_numterminal++;
            hex::log << hex::fine << "Terminal: " << value << hex::endl;
            return value;
        }
    }

    //
    // Evaluate if a leaf
    //
    if (depth == 0) {
        m_numleafs++;
        HexEval value = Evaluate();
        return value;
    }

    //
    // Check for transposition
    //
    std::string space(3*m_current_depth, ' ');

    m_tt_info_available = false;
    m_tt_bestmove = INVALID_POINT;
    if (m_tt) {
        SearchedState state;

        if (m_tt->get(m_brd.Hash(), state)) {

            m_tt_info_available = true;
            m_tt_bestmove = state.move;

            if (state.depth >= depth) {

                m_tt_hits++;

                hex::log << hex::info << space 
                         << "--- TT HIT ---" 
                         << hex::endl;

                if (state.bound == SearchedState::LOWER_BOUND) {
                    hex::log << hex::fine << "Lower Bound" << hex::endl;
                    alpha = std::max(alpha, state.score);
                } else if (state.bound == SearchedState::UPPER_BOUND) {
                    hex::log << hex::fine << "Upper Bound" << hex::endl;
                    beta = std::min(beta, state.score);
                } else if (state.bound == SearchedState::ACCURATE) {
                    hex::log << hex::fine << "Accurate" << hex::endl;
                    alpha = beta = state.score;
                }
                
                hex::log << "new (alpha, beta): (" << alpha
                         << ", " << beta << ")" << hex::endl;
                

                if (alpha >= beta) {
                    m_tt_cuts++;
                
                    pv.clear();
                    pv.push_back(state.move);
                    
                    return state.score;
                }
            }
        }
    }

    m_numinternal++;

    std::vector<HexPoint> moves;
    GenerateMoves(moves);
    HexAssert(moves.size());
    
    int curwidth = std::min(plywidth[m_current_depth], (int)moves.size());
    m_mustplay_branches += (int)moves.size();
    m_total_branches += curwidth;

    HexPoint bestmove = INVALID_POINT;
    HexEval bestvalue = -EVAL_INFINITY;

    for (int m=0; m<curwidth; ++m) {
	
        m_visited_branches++;
        hex::log << hex::info << space 
                 << (m+1) << "/" 
                 << curwidth << ": ("
                 << m_toplay << ", " << moves[m] << ")"
                 << ", (" << alpha << ", " << beta << ")" 
                 << hex::endl;
        
        ExecuteMove(moves[m]);
        m_current_depth++;
        m_sequence.push_back(moves[m]);
        m_toplay = !m_toplay;

        std::vector<HexPoint> cv;
        HexEval value = -search_state(plywidth, depth-1, -beta, -alpha, cv);

        m_toplay = !m_toplay;
        m_sequence.pop_back();
        m_current_depth--;
        UndoMove(moves[m]);

        if (value > bestvalue) {
            bestmove = moves[m];
            bestvalue = value;

            // compute new principal variation
            pv.clear();
            pv.push_back(bestmove);
            pv.insert(pv.end(), cv.begin(), cv.end());

            hex::log << hex::info << space 
                     << "--- New best: " << value 
                     << " PV:" << HexPointUtil::ToPointListString(pv) << " ---"
                     << hex::endl;
        }

        // store root move evaluations and output progress to gui
        if (m_current_depth == 0) { 
            m_eval.push_back(HexMoveValue(moves[m], value));
            
            if (GuiFx())
            {
                DumpGuiFx(m_eval, curwidth, pv, m_toplay);
            }
        }

        if (value >= alpha)
            alpha = value;

        if (alpha >= beta) {
            hex::log << hex::info << space << "--- Cutoff ---" << hex::endl;
            m_cuts++;
            break;
        }
    }

    HexAssert(bestmove != INVALID_POINT);

    //
    // Store in tt
    //
    if (m_tt) {

        SearchedState::Bound bound = SearchedState::ACCURATE;
        if (bestvalue <= old_alpha) bound = SearchedState::UPPER_BOUND;
        if (bestvalue >= old_beta) bound = SearchedState::LOWER_BOUND;

        SearchedState ss(m_brd.Hash(), depth, bound, bestvalue, bestmove);

        m_tt->put(ss);
    }

    AfterStateSearched();

    return bestvalue;
}

void HexAbSearch::ClearStats()
{
    m_numstates = 0;
    m_numleafs = 0;
    m_numterminal = 0;
    m_numinternal = 0;
    m_mustplay_branches = 0;
    m_total_branches = 0;
    m_visited_branches = 0;
    m_cuts = 0;
    m_tt_hits = 0;
    m_tt_cuts = 0;
}

std::string HexAbSearch::DumpPV(HexEval value, std::vector<HexPoint>& pv)
{
    std::ostringstream os;
    os << std::endl;
    os << "Value = " << value << std::endl;
    os << "PV:";
    for (unsigned i=0; i<pv.size(); ++i) {
        os << " " << pv[i];
    }
    return os.str();
}

std::string HexAbSearch::DumpStats()
{
    std::ostringstream os;
    os << std::endl;
    os << "        Leaf Nodes: " << m_numleafs << std::endl;
    os << "    Terminal Nodes: " << m_numterminal << std::endl;
    os << "    Internal Nodes: " << m_numinternal << std::endl;
    os << "       Total Nodes: " << m_numstates << std::endl;
    os << "           TT Hits: " << m_tt_hits << std::endl;
    os << "           TT Cuts: " << m_tt_cuts << std::endl;
    os << "Avg. Mustplay Size: " 
       << std::setprecision(4) << (double)m_mustplay_branches / m_numinternal
       << std::endl;
    os << "Avg. Branch Factor: " 
       << std::setprecision(4) << (double)m_total_branches / m_numinternal
       << std::endl;
    os << "       Avg. To Cut: " 
       << std::setprecision(4) << (double)m_visited_branches / m_numinternal
       << std::endl;
    os << "         Nodes/Sec: " 
       << std::setprecision(4) << (m_numstates/m_elapsed_time) 
       << std::endl;
    os << "      Elapsed Time: " 
       << std::setprecision(4) << m_elapsed_time << " secs"
       << std::endl;
    os << DumpPV(m_value, m_pv) << std::endl;
 
    return os.str();
}

//----------------------------------------------------------------------------
