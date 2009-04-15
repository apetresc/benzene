//----------------------------------------------------------------------------
// $Id: HexAbSearch.cpp 1900 2009-02-07 04:31:32Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

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

std::string DumpPV(HexEval value, const std::vector<HexPoint>& pv)
{
    std::ostringstream os;
    os << "PV: [" << std::fixed << std::setprecision(4) << value << "]";
    for (std::size_t i=0; i<pv.size(); ++i)
        os << " " << pv[i];
    return os.str();
}

//----------------------------------------------------------------------------

} // anonymous namespace

//----------------------------------------------------------------------------

HexAbSearch::HexAbSearch()
    : m_brd(0),
      m_tt(0)
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

HexEval HexAbSearch::Search(HexBoard& brd, HexColor color,
                            const std::vector<int>& plywidth, 
                            const std::vector<int>& depths_to_search, 
                            int UNUSED(timelimit),
                            std::vector<HexPoint>& outPV)
{
    double start = HexGetTime();

    m_brd = &brd;
    m_toplay = color;
    m_statistics = Statistics();

    OnStartSearch();

    std::vector<HexMoveValue> outEval;
    double outValue = -EVAL_INFINITY;
    outPV.clear();
    outPV.push_back(INVALID_POINT);

    m_aborted = false;
    for (std::size_t d=0; !m_aborted && d < depths_to_search.size(); ++d) 
    {
        int depth = depths_to_search[d];
        hex::log << hex::info << "---- Depth " << depth << " ----" << hex::endl;

        double beganAt = HexGetTime();

        m_eval.clear();
        m_current_depth = 0;
        m_sequence.clear();
        std::vector<HexPoint> thisPV;

        double thisValue = SearchState(plywidth, depth, IMMEDIATE_LOSS, 
                                       IMMEDIATE_WIN, thisPV);

        double finishedAt = HexGetTime();

        // copy result only if search was not aborted
        if (!m_aborted)
        {
            outPV = thisPV;
            outValue = thisValue;
            outEval = m_eval;

            m_statistics.value = thisValue;
            m_statistics.pv = thisPV;

            hex::log << hex::info 
                     << DumpPV(thisValue, thisPV) << hex::endl
                     << "Time: " << std::fixed << std::setprecision(4) 
                     << (finishedAt - beganAt) << hex::endl;
        }
        else
        {
            hex::log << hex::info << "Throwing away current iteration..."
                     << hex::endl;
        }
    }
    
    OnSearchComplete();

    double end = HexGetTime();
    m_statistics.elapsed_time = end - start;
    
    // Copy the root evaluations back into m_eval; these will be printed
    // when DumpStats() is called.
    m_eval = outEval;

    return outValue;
}

//----------------------------------------------------------------------------

HexEval HexAbSearch::CheckTerminalState()
{
    if (PlayerUtils::IsWonGame(*m_brd, m_toplay))
        return IMMEDIATE_WIN - m_current_depth;
    
    if (PlayerUtils::IsLostGame(*m_brd, m_toplay))
        return IMMEDIATE_LOSS + m_current_depth;

    return 0;
}

bool HexAbSearch::CheckAbort()
{
    if (SgUserAbort())
    {
        hex::log << hex::info << "HexAbSearch::CheckAbort(): Abort flag!"
                 << hex::endl;
        m_aborted = true;
        return true;
    }

    // @todo CHECK TIMELIMIT
    return false;
}

HexEval HexAbSearch::SearchState(const std::vector<int>& plywidth,
                                 int depth, HexEval alpha, HexEval beta,
                                 std::vector<HexPoint>& pv)
{
    HexAssert(m_current_depth + depth <= (int)plywidth.size());

    if (CheckAbort()) 
        return -EVAL_INFINITY;

    m_statistics.numstates++;
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
            m_statistics.numterminal++;
            hex::log << hex::fine << "Terminal: " << value << hex::endl;
            return value;
        }
    }

    //
    // Evaluate if a leaf
    //
    if (depth == 0) {
        m_statistics.numleafs++;
        HexEval value = Evaluate();
        return value;
    }

    //
    // Check for transposition
    //
    std::string space(3*m_current_depth, ' ');

    m_tt_info_available = false;
    m_tt_bestmove = INVALID_POINT;
    if (m_tt) 
    {
        SearchedState state;
        if (m_tt->get(m_brd->Hash(), state)) 
        {
            m_tt_info_available = true;
            m_tt_bestmove = state.move;

            if (state.depth >= depth) 
            {
                m_statistics.tt_hits++;

                hex::log << hex::fine << space 
                         << "--- TT HIT ---" << hex::endl;

                if (state.bound == SearchedState::LOWER_BOUND) 
                {
                    hex::log << hex::fine << "Lower Bound" << hex::endl;
                    alpha = std::max(alpha, state.score);
                } 
                else if (state.bound == SearchedState::UPPER_BOUND) 
                {
                    hex::log << hex::fine << "Upper Bound" << hex::endl;
                    beta = std::min(beta, state.score);
                } 
                else if (state.bound == SearchedState::ACCURATE) 
                {
                    hex::log << hex::fine << "Accurate" << hex::endl;
                    alpha = beta = state.score;
                }
                
                hex::log << "new (alpha, beta): (" << alpha
                         << ", " << beta << ")" << hex::endl;
                
                if (alpha >= beta) 
                {
                    m_statistics.tt_cuts++;
                
                    pv.clear();
                    pv.push_back(state.move);
                    
                    return state.score;
                }
            }
        }
    }

    m_statistics.numinternal++;

    std::vector<HexPoint> moves;
    GenerateMoves(moves);
    HexAssert(moves.size());
    
    int curwidth = std::min(plywidth[m_current_depth], (int)moves.size());
    m_statistics.mustplay_branches += (int)moves.size();
    m_statistics.total_branches += curwidth;

    HexPoint bestmove = INVALID_POINT;
    HexEval bestvalue = -EVAL_INFINITY;

    for (int m = 0; !m_aborted && m < curwidth; ++m) 
    {
        m_statistics.visited_branches++;
        hex::log << hex::fine << space 
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
        HexEval value = -SearchState(plywidth, depth-1, -beta, -alpha, cv);

        m_toplay = !m_toplay;
        m_sequence.pop_back();
        m_current_depth--;
        UndoMove(moves[m]);

        if (value > bestvalue) 
        {
            bestmove = moves[m];
            bestvalue = value;

            // compute new principal variation
            pv.clear();
            pv.push_back(bestmove);
            pv.insert(pv.end(), cv.begin(), cv.end());

            hex::log << hex::fine << space 
                     << "--- New best: " << value 
                     << " PV:" << HexPointUtil::ToPointListString(pv) << " ---"
                     << hex::endl;
        }

        // store root move evaluations and output progress to gui
        if (m_current_depth == 0) 
        { 
            m_eval.push_back(HexMoveValue(moves[m], value));
            if (m_use_guifx)
                DumpGuiFx(m_eval, curwidth, pv, m_toplay);
        }

        if (value >= alpha)
            alpha = value;

        if (alpha >= beta) 
        {
            hex::log << hex::fine << space << "--- Cutoff ---" << hex::endl;
            m_statistics.cuts++;
            break;
        }
    }

    if (m_aborted)
        return -EVAL_INFINITY;

    //
    // Store in tt
    //
    HexAssert(bestmove != INVALID_POINT);
    if (m_tt) 
    {
        SearchedState::Bound bound = SearchedState::ACCURATE;
        if (bestvalue <= old_alpha) bound = SearchedState::UPPER_BOUND;
        if (bestvalue >= old_beta) bound = SearchedState::LOWER_BOUND;
        SearchedState ss(m_brd->Hash(), depth, bound, bestvalue, bestmove);
        m_tt->put(ss);
    }

    AfterStateSearched();

    return bestvalue;
}

std::string HexAbSearch::DumpStats()
{
    std::ostringstream os;
    os << m_statistics.Dump() << '\n';

    std::vector<HexMoveValue> root_evals(m_eval);
    stable_sort(root_evals.begin(), root_evals.end(),
                std::greater<HexMoveValue>());

    os << '\n';
    std::size_t num = 10;
    for (std::size_t i=0; i<num && i<root_evals.size(); ++i) {
        if (i && i%5==0)
            os << '\n';
        os << "(" 
           << root_evals[i].point() << ", " 
           << std::fixed << std::setprecision(3) << root_evals[i].value() 
           << ") ";
    }
    os << '\n';

    return os.str();
}

std::string HexAbSearch::Statistics::Dump() const
{
    std::ostringstream os;
    os << '\n'
       << "        Leaf Nodes: " << numleafs << '\n'
       << "    Terminal Nodes: " << numterminal << '\n'
       << "    Internal Nodes: " << numinternal << '\n'
       << "       Total Nodes: " << numstates << '\n'
       << "           TT Hits: " << tt_hits << '\n'
       << "           TT Cuts: " << tt_cuts << '\n'
       << "Avg. Mustplay Size: " << std::setprecision(4) 
       << (double)mustplay_branches / numinternal << '\n'
       << "Avg. Branch Factor: " << std::setprecision(4) 
       << (double)total_branches / numinternal << '\n'
       << "       Avg. To Cut: " << std::setprecision(4) 
       << (double)visited_branches / numinternal << '\n'
       << "         Nodes/Sec: " << std::setprecision(4) 
       << (numstates/elapsed_time) << '\n'
       << "      Elapsed Time: " << std::setprecision(4) 
       << elapsed_time << "s" << '\n'
       << '\n'
       << DumpPV(value, pv);
    return os.str();
}

//----------------------------------------------------------------------------
