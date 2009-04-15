//----------------------------------------------------------------------------
// $Id: OpeningBook.cpp 1789 2008-12-14 02:52:48Z broderic $
//----------------------------------------------------------------------------

/** @file
    
    @todo Cleanup? Make methods return OpeningNode objects instead of
    going through the db to get/set them?
*/

#include "BitsetIterator.hpp"
#include "OpeningBook.hpp"
#include "PlayerUtils.hpp"
#include "Resistance.hpp"
#include "Time.hpp"

//----------------------------------------------------------------------------

/** Dump debug info. */
#define OUTPUT_OB_INFO 1

//----------------------------------------------------------------------------

/** This method clears the values of a node that depend on its successors.
    Used prior to updating an internal opening book node. */
void OpeningBookNode::InvalidateSuccBasedValues()
{
    m_packed.propValue    = DUMMY_VALUE;
    m_packed.propSucc     = DUMMY_SUCC;
    m_packed.expPriority  = DUMMY_PRIORITY;
    m_packed.expSucc      = DUMMY_SUCC;
}

/** Given a successor's propagated value, updates that of this node. */
void OpeningBookNode::UpdatePropValue(HexPoint succ, float succPropValue)
{
    float newPropValue = -succPropValue;
    if (newPropValue > getPropValue()) {
	m_packed.propValue = newPropValue;
	m_packed.propSucc = succ;
    }
}

/** Given a successor's data, updates the expansion priority of this node. */
void OpeningBookNode::UpdateExpPriority(HexPoint succ, float succPropValue,
					float alpha, float succExpPriority)
{
    float newExpPriority = ComputeExpPriority(succPropValue, alpha,
					      succExpPriority);
    
    if (newExpPriority < getExpPriority()) {
	m_packed.expPriority = newExpPriority;
	m_packed.expSucc = succ;
    }
}

/** Given a successor's data, computes the expansion priority of this node. */
float OpeningBookNode::ComputeExpPriority(float succPropValue, float alpha,
					  float succExpPriority)
{
    HexAssert(getPropValue() + succPropValue >= 0.0);
    HexAssert(succExpPriority >= LEAF_PRIORITY);
    HexAssert(succExpPriority < DUMMY_PRIORITY);
    return alpha * (getPropValue() + succPropValue) + 1 + succExpPriority;
}

bool OpeningBookNode::IsTerminal() const
{
    if (HexEvalUtil::IsWinOrLoss(getPropValue()))
        return true;
    return false;
}

/** Returns true iff this node is a leaf in the opening book. */
bool OpeningBookNode::IsLeaf() const
{
    bool leaf = (getExpPriority() == LEAF_PRIORITY);
    HexAssert((!leaf) ^ (getPropSucc() == LEAF_SUCC));
    HexAssert((!leaf) ^ (getExpSucc() == LEAF_SUCC));
    return leaf;
}

/** Returns true iff this node has not been added to the opening book. */
bool OpeningBookNode::IsDummy() const
{
    bool isDummyNode = (getHeurValue() == DUMMY_VALUE);
    HexAssert(!isDummyNode || (getPropValue() == DUMMY_VALUE));
    HexAssert(!isDummyNode || (getPropSucc() == DUMMY_SUCC));
    HexAssert(!isDummyNode || (getExpPriority() == DUMMY_PRIORITY));
    HexAssert(!isDummyNode || (getExpSucc() == DUMMY_SUCC));
    return isDummyNode;
}

std::string OpeningBookNode::toString() const
{
    std::ostringstream os;
    os << "Heur=" << getHeurValue() 
       << ", Prop=" << getPropValue()
       << ", PSucc=" << getPropSucc()
       << ", ExpP=" << getExpPriority() 
       << ", ESucc=" << getExpSucc();
    return os.str();
}

//----------------------------------------------------------------------------

OpeningBook::OpeningBook(int width, int height, 
                         double alpha, std::string filename)
    : m_dampen_scores(true),
      m_flush_iterations(1000),
      m_use_cache(true)
{
    m_settings.board_width = width;
    m_settings.board_height = height;
    m_settings.alpha = alpha;
    HexAssert(alpha >= 0.0);

    bool db_opened_successfully = m_db.Open(filename);
    HexAssert(db_opened_successfully);

    // Load settings from database and ensure they match the current
    // settings.  
    char key[] = "settings";
    Settings temp;
    if (m_db.Get(key, strlen(key)+1, &temp, sizeof(temp))) {
        hex::log << hex::info << "Old book." << hex::endl;
        if (m_settings != temp) {
            hex::log << "Settings do not match book settings!" << hex::endl;
            hex::log << "Book: " << temp.toString() << hex::endl;
            hex::log << "Current: " << m_settings.toString() << hex::endl;
            HexAssert(false);
        } 
    } else {
        // Read failed: this is a new database.
        // Store the settings.
        hex::log << hex::info << "New book!" << hex::endl;
        bool settings_stored_successfully = 
            m_db.Put(key, strlen(key)+1, &m_settings, sizeof(m_settings));
        HexAssert(settings_stored_successfully);
    }
}

/** Destructor */
OpeningBook::~OpeningBook()
{
}

//----------------------------------------------------------------------------

OpeningBookNode OpeningBook::GetNode(hash_t hash) const
{
    OpeningBookNode ret;

    // check db for internal nodes
    if (m_db.Get(hash, ret)) {
        
        /** db doesn't store hash so set it here. */
        ret.SetHash(hash);
        
    } else {

        // check hashtable for leaf
        ret = m_leafs[hash];
        m_statistics.cache_reads++;

        // if not the node we're looking for
        if (ret.Hash() != hash) {
            ret = OpeningBookNode();
        }
    }
    HexAssert(ret.IsDummy() || ret.Hash() == hash);
    return ret;
}

void OpeningBook::Expand(HexBoard& brd, int numExpansions)
{
    HexColor rootToPlay = brd.WhoseTurn();
    brd.ComputeAll(rootToPlay, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    // Do nothing in determined states!
    if (PlayerUtils::IsDeterminedState(brd, rootToPlay))
    {
        hex::log << hex::info << "Root state is determined!" << hex::endl
                 << "Aborting book expand from this state." << hex::endl;
        return;
    }

    // Ensure root node exists before we start.
    // Create one if it does not exist. 
    OpeningBookNode root = GetNode(brd.Hash());
    if (root.IsDummy()) 
    {
        hex::log << hex::info << "Creating root node. " << hex::endl;
        root = OpeningBookNode(brd.Hash(), EvalState(brd));
        m_db.Put(brd.Hash(), root);
    }
    
    double s = HexGetTime();

    int i;
    m_statistics = Statistics();

    for (i=0; i<numExpansions; i++) {

#if OUTPUT_OB_INFO
	hex::log << hex::info << "\n--Iteration " << i << "--" << hex::endl;
#endif
	
        // Flush the db if we've performed enough iterations
        if (i && (i % m_flush_iterations)==0) {
            hex::log<< hex::info << "Flushing DB..." << hex::endl;
            m_db.Flush();
        }
        
	// If root position becomes a known win or loss, then there's
	// no point in continuing to expand the opening book.
        OpeningBookNode node = GetNode(brd.Hash());
        if (node.IsTerminal()) {
#if OUTPUT_OB_INFO
	    hex::log << "Position solved! Terminating expansion."
		     << hex::endl;
#endif
	    break;
	}
	
	/** Find leaf with highest expansion priority, expand it, and
            update the opening book nodes on the path from that leaf
            to the root.

            @note FindNextLeafToExpand() will stop at non-leaf
            terminal nodes (it can get there by following a different
            variation than the first to that node), since there is no
            point continuing down a chain of already solved nodes.
        */
        MoveSequence variation;
	FindNextLeafToExpand(brd, variation);

        {
            // expand only if it is a leaf and non-terminal
            OpeningBookNode node = GetNode(brd.Hash());
            if (node.IsLeaf() && !node.IsTerminal())
                ExpandLeaf(brd);
        }
        
        PropagateValuesUpVariation(brd, variation);
        HexAssert(variation.empty());
    }

    hex::log<< hex::info << "Flushing DB..." << hex::endl;
    m_db.Flush();

    double e = HexGetTime();

    hex::log << hex::info << hex::endl;
    hex::log << "  Expansions: " << i << hex::endl;
    hex::log << " Cache Reads: " << m_statistics.cache_reads << hex::endl;
    hex::log << "Cache Writes: " << m_statistics.cache_writes << hex::endl;
    hex::log << "Cache Misses: " << m_statistics.cache_misses << hex::endl;
    hex::log << "  Shrinkings: " << m_statistics.shrinkings << hex::endl;
    if (m_statistics.shrinkings) {
        hex::log << " Avg. Shrink: " 
                 << std::fixed << std::setprecision(4) 
                 << ((double)m_statistics.shrunk_cells / m_statistics.shrinkings) 
                 << hex::endl;
    }
    hex::log << "  Total Time: " << FormattedTime(e - s) << hex::endl;
    hex::log << "     Exp/sec: " << (i/(e-s)) << hex::endl;
}

int OpeningBook::GetMainLineDepth(const StoneBoard& pos, HexColor color) const
{
    int depth = 0;
    StoneBoard brd(pos);
    for (;;) {
        OpeningBookNode node = GetNode(brd.Hash());
        HexPoint move = node.getPropSucc();
        if (move == INVALID_POINT)
            break;
        brd.playMove(color, move);
        color = !color;
        depth++;
    }
    return depth;
}

int OpeningBook::GetTreeSize(StoneBoard& brd, HexColor color) const
{
    std::map<hash_t, int> solved;
    return tree_size(brd, color, solved);
}

int OpeningBook::tree_size(StoneBoard& brd, HexColor color,
                           std::map<hash_t, int>& solved) const
{
    if (solved.find(brd.Hash()) != solved.end()) {
        return solved[brd.Hash()];
    }

    OpeningBookNode node = GetNode(brd.Hash());
    if (node.IsDummy())
        return 0;
    if (node.getPropSucc() == INVALID_POINT)
        return 0;
   
    int ret = 1;

    for (BitsetIterator p(node.getChildren()); p; ++p) {
        brd.playMove(color, *p);
        ret += tree_size(brd, !color, solved);
        brd.undoMove(*p);
    }

    solved[brd.Hash()] = ret;
    return ret;
}

//----------------------------------------------------------------------------

HexEval OpeningBook::EvalState(HexBoard& brd) const
{
    HexEval value;
    if (PlayerUtils::IsDeterminedState(brd, brd.WhoseTurn(), value))
        return value;

    Resistance resist;
    resist.Evaluate(brd);
    double eval = resist.Score();
    if (brd.WhoseTurn() == WHITE)
	eval *= -1.0;

    return eval;
}

OpeningBookNode OpeningBook::CreateChild(HexBoard& brd, 
                                         const OpeningBookNode& parent)
{
    HexEval value = EvalState(brd);

    if (!HexEvalUtil::IsWinOrLoss(value) && m_dampen_scores)
        return OpeningBookNode(brd.Hash(), value, -parent.getHeurValue());

    return OpeningBookNode(brd.Hash(), value);
}

void OpeningBook::UpdateNode(HexBoard& brd)
{
    OpeningBookNode obn = GetNode(brd.Hash());
    HexAssert(!obn.IsDummy());

    // If node is terminal, just abort: there's nothing to do here.
    if (obn.IsTerminal()) {
        return;
    }

    // clear the old propagated values
    obn.InvalidateSuccBasedValues();
    HexAssert(obn.getPropValue() == OpeningBookNode::DUMMY_VALUE);
    
    bitset_t children = 
        obn.getChildren() & PlayerUtils::MovesToConsider(brd, brd.WhoseTurn());

    if (children != obn.getChildren()) {
        m_statistics.shrinkings++;
        m_statistics.shrunk_cells 
            += obn.getChildren().count() - children.count();
        hex::log << hex::info << "Shrunk children!" << hex::endl;
        hex::log << "Original: " << brd.printBitset(obn.getChildren()) << hex::endl;
        hex::log << "New: " << brd.printBitset(children) << hex::endl;
    }
    obn.SetChildren(children);
    HexAssert(children.any());
    
    // Update propagation value first
    for (BitsetIterator i(children); i; ++i) {
	brd.playMove(brd.WhoseTurn(), *i);
	OpeningBookNode child = GetNode(brd.Hash());
	
        // Child may not exist since cleared from hash table
	if (m_use_cache && child.IsDummy()) {
	    brd.undoMove(*i);
	    brd.PlayMove(brd.WhoseTurn(), *i);
	    
	    child = CreateChild(brd, obn);
            m_leafs[brd.Hash()] = child;
            m_statistics.cache_writes++;
            m_statistics.cache_misses++;
            hex::log << hex::info << "$$$ cache miss [" 
                     << HashUtil::toString(brd.Hash()) << "] in propagate $$$" 
                     << hex::endl;
	    HexAssert(!child.IsDummy());
	    
	    brd.UndoMove();
	    brd.playMove(brd.WhoseTurn(), *i);
	}
	obn.UpdatePropValue(*i, child.getPropValue());
	brd.undoMove(*i);
    }
    
    // Update expansion priority next
    for (BitsetIterator i(children); i; ++i) {
	brd.playMove(brd.WhoseTurn(), *i);
	OpeningBookNode child = GetNode(brd.Hash());
	HexAssert(!child.IsDummy());
	obn.UpdateExpPriority(*i, child.getPropValue(),
			      m_settings.alpha, child.getExpPriority());
	brd.undoMove(*i);
    }
    
    // 
    // @todo Children on variations A and B are disjoint sets.
    // Originaly we expand children after following A.  Then, after
    // following variation B, we will hit no children.
    // If this occurs then this state is a losing move. 
    // 
    HexAssert(obn.getPropValue() != OpeningBookNode::DUMMY_VALUE);

    m_db.Put(brd.Hash(), obn);

#if OUTPUT_OB_INFO
    hex::log << hex::info << "Updated " << obn.toString() << hex::endl;
#endif

}

void OpeningBook::FindNextLeafToExpand(HexBoard& brd,
                                       MoveSequence& variation)
{
    OpeningBookNode parent;
    OpeningBookNode obn = GetNode(brd.Hash());
    HexAssert(!obn.IsDummy());

    variation.clear();
    while (!obn.IsLeaf() && !obn.IsTerminal()) {

        parent = obn;
	HexPoint p = obn.getExpSucc();
        variation.push_back(p);
	brd.PlayMove(brd.WhoseTurn(), p);
	obn = GetNode(brd.Hash());

        // Node may not exist if it was pushed out of the cache.
        // Recalculate it if that is the case. 
        if (m_use_cache && obn.IsDummy()) {
            HexAssert(!parent.IsDummy());
            obn = CreateChild(brd, parent);
            m_leafs[brd.Hash()] = obn;
            m_statistics.cache_writes++;
            m_statistics.cache_misses++;
            hex::log << hex::info << "$$$ cache miss ["
                     << HashUtil::toString(brd.Hash()) << "] in expand $$$" 
                     << hex::endl;
        }
        HexAssert(!obn.IsDummy());
    }
    
#if OUTPUT_OB_INFO
    for (unsigned i=0; i<variation.size(); ++i) {
	hex::log << hex::info << " " << variation[i];
    }
    hex::log << hex::endl;
#endif

}

void OpeningBook::ExpandLeaf(HexBoard& brd)
{
#if OUTPUT_OB_INFO
    hex::log << hex::fine << "\n***ExpandLeaf***" << hex::endl;
    hex::log << brd << hex::endl;
#endif
    
    /** Check for terminal leafs.  Normally, we cannot get here if
        this state is terminal. However, it is possible the first
        variation did not detect this was a terminal leaf, but the
        current variation did (this can happen because order of
        vc-computations matters). In this case, we mark this node as a
        terminal node and abort.
    */
    HexEval value;
    HexColor toMove = brd.WhoseTurn();
    if (PlayerUtils::IsDeterminedState(brd, toMove, value))
    {
        OpeningBookNode node = GetNode(brd.Hash());
        hex::log << hex::info << node.toString() << hex::endl;
        if (!node.IsTerminal()) {
            hex::log << hex::info << "---- Different variation made "
                     << "non-terminal leaf terminal!" << hex::endl;
            node = OpeningBookNode(brd.Hash(), value);
            m_db.Put(brd.Hash(), node);
            return;
        } 
        hex::log << hex::severe << "Re-expanding terminal leaf!!" << hex::endl;
        HexAssert(false);
    }
   
    // Evaluate each child
    bitset_t children = PlayerUtils::MovesToConsider(brd, toMove);
    HexAssert(children.any());

    OpeningBookNode parent = GetNode(brd.Hash());
    for (BitsetIterator p(children); p; ++p) 
    {
	brd.PlayMove(toMove, *p);
	OpeningBookNode child = GetNode(brd.Hash());
	
	// Only need to evaluate child if not in DB (i.e. via transposition)
	if (child.IsDummy()) 
        {
            child = CreateChild(brd, parent);
	    
            if (m_use_cache)
            {
                m_leafs[brd.Hash()] = child;
                m_statistics.cache_writes++;
            } 
            else 
            {
                m_db.Put(brd.Hash(), child);
            }
	}

#if OUTPUT_OB_INFO
	hex::log << hex::fine 
                 << "Added " << *p << " with value "
		 << child.getHeurValue() << ", " << child.getPropValue()
		 << hex::endl;
#endif
	brd.UndoMove();
    }

    // Now that we've looked at all the children, store the pruned
    // set of children (backing-up ice info can shrink this set, etc).
    {
        bitset_t new_children;
        hex::log << hex::fine << "Getting smaller mustplay..." << hex::endl;
        if (PlayerUtils::IsDeterminedState(brd, toMove)) 
        {
            // this is probably never reached.
            HexAssert(false);
            new_children.reset();
            new_children.set(PlayerUtils::PlayDeterminedState(brd, toMove));
        }
        else 
        {
            new_children = PlayerUtils::MovesToConsider(brd, toMove);
        }
        HexAssert(BitsetUtil::IsSubsetOf(new_children, children));
        children = new_children;
    }

    parent.SetChildren(children);
    m_db.Put(brd.Hash(), parent);
}

void OpeningBook::PropagateValuesUpVariation(HexBoard& brd,
                                             MoveSequence& variation)
{
    // perform n+1 updates, where n is the number of moves played in
    // the variation.
    UpdateNode(brd);
    while(!variation.empty()) {
        variation.pop_back();
	brd.UndoMove();
        UpdateNode(brd);
    }
}

//----------------------------------------------------------------------------
