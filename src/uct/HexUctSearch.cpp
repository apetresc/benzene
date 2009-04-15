//----------------------------------------------------------------------------
// $Id: HexUctSearch.cpp 1674 2008-09-18 23:25:45Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "SgException.h"
#include "SgNode.h"
#include "SgMove.h"
#include "SgSList.h"

#include "BoardUtils.hpp"
#include "BitsetIterator.hpp"
#include "HexSgUtil.hpp"
#include "HexUctPolicy.hpp"
#include "HexUctSearch.hpp"
#include "HexUctState.hpp"
#include "HexUctUtil.hpp"
#include "PatternBoard.hpp"

//----------------------------------------------------------------------------

HexThreadStateFactory::HexThreadStateFactory(HexUctSharedPolicy* shared)
    : m_shared_policy(shared)
{
}

HexThreadStateFactory::~HexThreadStateFactory()
{
}

SgUctThreadState* 
HexThreadStateFactory::Create(std::size_t threadId,
                              const SgUctSearch& search)
{
    const HexUctSearch& hexSearch 
        = dynamic_cast<const HexUctSearch&>(search);

    hex::log << hex::info << "Creating thread " << threadId << hex::endl;

    HexUctState* state = new HexUctState(threadId,
					 hexSearch,
                                         hexSearch.Board(),
                                         hexSearch.InitialData());

    state->SetPolicy(new HexUctPolicy(m_shared_policy));
    return state;
}

//----------------------------------------------------------------------------

HexUctSearch::HexUctSearch(PatternBoard& bd, 
                           const HexUctInitialData* data, 
                           SgUctThreadStateFactory* factory,
			   int maxMoves)
    : SgUctSearch(factory, maxMoves),

      m_keepGames(hex::settings.get_bool("uct-save-games")),
      m_liveGfx(hex::settings.get_bool("uct-livegfx")), 
      m_liveGfxInterval(hex::settings.get_int("uct-livegfx-interval")),

      m_bd(bd),
      m_initial_data(data),

      m_root(0)
{
}

HexUctSearch::~HexUctSearch()
{
    if (m_root != 0)
        m_root->DeleteTree();
    m_root = 0;
}

#if 0
void HexUctSearch::AppendGame(const std::vector<SgMove>& sequence)
{
    HexAssert(m_root != 0);
    SgNode* node = m_root;
    HexColor color = m_initial_toPlay;

    std::vector<SgPoint>::const_iterator it = sequence.begin();

    // merge this game into the tree of games;
    // so find first move that starts a new variation.
    if (hex::settings.get_bool("uct-save-games-merge")) {
        for (; it != sequence.end(); ++it) {
            
            if (!node->HasSon())
                break;

            bool found = false;
            for (SgNode* child = node->LeftMostSon(); ;
                 child = child->RightBrother()) 
            {
                HexPoint move = HexSgUtil::SgPointToHexPoint(child->NodeMove(),
                                                             m_bd.height());

                // found it! recurse down this branch.
                if (move == *it) {
                    node = child;
                    found = true;
                    break;
                }

                if (!child->HasRightBrother()) 
                    break;
            }            

            // abort if we need to start a new variation
            if (!found) break;

            color = !color;
        }
    }

    // add the remainder of the sequence to this node
    for (; it != sequence.end(); ++it) {
        SgNode* child = node->NewRightMostSon();
        HexSgUtil::AddMoveToNode(child, color, *it, m_bd.height());
        color = !color;
        node = child;
    }
}
#endif 

void HexUctSearch::OnStartSearch()
{
    if (m_root != 0)
        m_root->DeleteTree();

    if (m_keepGames) {
        hex::log << hex::severe << "uct-save-games disabled!" << hex::endl;
        HexAssert(false);
#if 0        
        m_root = new SgNode();
        HexSgUtil::SetPositionInNode(m_root, m_bd, m_initial_toPlay);
#endif
    }

    // Limit to avoid very long games (no need in Hex)
    int size = m_bd.width() * m_bd.height();
    int maxGameLength = size+10;
    SetMaxGameLength(maxGameLength);
}

void HexUctSearch::SaveGames(const std::string& filename) const
{
    if (m_root == 0)
        throw SgException("No games to save");
    HexSgUtil::WriteSgf(m_root, "MoHex", filename.c_str(), m_bd.height()); 
}

void HexUctSearch::SaveTree(std::ostream& UNUSED(out)) const
{
    hex::log << hex::warning << "SaveTree() not implemented yet!" << hex::endl;

#if 0
    HexUctUtil::SaveTree(Tree(), m_bd.Size(), m_stones, m_initial_toPlay, out,
                         UseMoveCounts());
#endif
}

void HexUctSearch::OnSearchIteration(std::size_t gameNumber, 
                                     int UNUSED(threadId),
                                     const SgUctGameInfo& UNUSED(info))
{
    if (m_liveGfx && gameNumber % m_liveGfxInterval == 0)
    {
        std::ostringstream os;
        os << "gogui-gfx:\n";
        os << "uct\n";
        HexColor initial_toPlay = m_initial_data->root_to_play;
        HexUctUtil::GoGuiGfx(*this, 
                             HexUctUtil::ToSgBlackWhite(initial_toPlay),
			     os);
        os << "\n";
        std::cout << os.str();
        std::cout.flush();
        hex::log << hex::fine << os.str() << hex::endl;
    }
    
#if 0
    if (m_root != 0)
        AppendGame(LastGameInfo().m_sequence);
#endif
}

float HexUctSearch::UnknownEval() const
{
    // Note: 0.5 is not a possible value for a Bernoulli variable, better
    // use 0?
    return 0.5;
}

float HexUctSearch::InverseEval(float eval) const
{
    return (1 - eval);
}

std::string HexUctSearch::MoveString(SgMove move) const
{
    return HexPointUtil::toString(static_cast<HexPoint>(move));
}

//----------------------------------------------------------------------------

