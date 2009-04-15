//----------------------------------------------------------------------------
// $Id: OpeningBook.hpp 1789 2008-12-14 02:52:48Z broderic $
//----------------------------------------------------------------------------

#ifndef OPENINGBOOK_HPP
#define OPENINGBOOK_HPP

#include "Hex.hpp"
#include "HexBoard.hpp"
#include "HashDB.hpp"
#include "HexEval.hpp"

//----------------------------------------------------------------------------
/** This code is based on Thomas R. Lincke's paper "Strategies for the
    Automatic Construction of Opening Books" published in 2001.
    
    We make the following adjustments:
    1) Neither side is assumed to be the book player, so the expansion
       formula is identical for all nodes (see page 80 of the paper). In other
       words, both sides can play sub-optimal moves.
    2) We do not include the swap rule as a move, since this would lead to
       redundant evaluation computations (such as a2-f6 and a2-swap-f6). This
       is fine since the value of the root position indicates whether or not
       to swap.
    3) Each node stores its successors for propagation value and expansion
       priority.
    4) These nodes will be referenced by a map or DB so that transpositions
       are not re-computed. So it is not a tree, but a DAG.
    
    We also think there is a typo with respect to the formula of epo_i on
    page 80. Namely, since p_i is the negamax of p_{s_j}s, then we should
    sum the values to find the distance from optimal, not subtract. That is,
    we use epo_i = 1 + min(s_j) (epb_{s_j} + alpha*(p_i + p_{s_j}) instead. */
//----------------------------------------------------------------------------

/** A node in the Opening Book. */
class OpeningBookNode
{

public:

    //------------------------------------------------------------------------

    static const int DUMMY_VALUE = -9999999;
    static const int DUMMY_PRIORITY = 9999999;
    static const int LEAF_PRIORITY = 0;
    static const HexPoint DUMMY_SUCC = INVALID_POINT;
    static const HexPoint LEAF_SUCC = INVALID_POINT;

    //------------------------------------------------------------------------

    /** Data to be stored in the db. */
    struct Packed
    {
        float heurValue;
        float propValue;
        HexPoint propSucc;
        float expPriority;
        HexPoint expSucc;
        bitset_t children;

        Packed()
            : heurValue(DUMMY_VALUE),
              propValue(DUMMY_VALUE),
              propSucc(DUMMY_SUCC),
              expPriority(DUMMY_PRIORITY),
              expSucc(DUMMY_SUCC),
              children()
        { }

        Packed(float heuristicValue)
            : heurValue(heuristicValue),
              propValue(heuristicValue),
              propSucc(LEAF_SUCC),
              expPriority(LEAF_PRIORITY),
              expSucc(LEAF_SUCC),
              children()
        { }

        Packed(float lowerBound, float upperBound)
            : heurValue(lowerBound),
              propValue(0.5 * (lowerBound + upperBound)),
              propSucc(LEAF_SUCC),
              expPriority(LEAF_PRIORITY),
              expSucc(LEAF_SUCC),
              children()
        { }

        /** @bug Using == on floats! */
        bool operator==(const Packed& o) const
        {
            return (heurValue == o.heurValue && 
                    propValue == o.propValue && 
                    propSucc == o.propSucc && 
                    expPriority == o.expPriority && 
                    expSucc == o.expSucc && 
                    children == o.children); 
        }
    };

    //------------------------------------------------------------------------

    // Constructors. Note that we should only construct leaves.
    OpeningBookNode()
	: m_packed(),
          m_hash(0)
    { }
    OpeningBookNode(hash_t hash, float heuristicValue)
	: m_packed(heuristicValue),
          m_hash(hash)
    { }
    OpeningBookNode(hash_t hash, float lowerBound, float upperBound)
	: m_packed(lowerBound, upperBound),
          m_hash(hash)
    { }
    
    // Accessor methods
    float getHeurValue() const;
    float getPropValue() const;
    HexPoint getPropSucc() const;
    float getExpPriority() const;
    HexPoint getExpSucc() const;
    bitset_t getChildren() const;
    hash_t Hash() const;
    
    // Update methods
    void InvalidateSuccBasedValues();
    void UpdatePropValue(HexPoint succ, float succPropValue);
    void UpdateExpPriority(HexPoint succ, float succPropValue,
			   float alpha, float succExpPriority);
    float ComputeExpPriority(float succPropValue, float alpha,
			     float succExpPriority);
    void SetChildren(const bitset_t& children);

    // Additional properties
    bool IsLeaf() const;
    bool IsDummy() const;
    bool IsTerminal() const;
    
    // Output
    std::string toString() const;

    // Methods for PackableConcept (so it can be used in a HashDB)
    int PackedSize() const;
    byte* Pack() const;
    void Unpack(const byte* t);

    void SetHash(hash_t hash);

private:
    Packed m_packed;
    hash_t m_hash;
};

inline float OpeningBookNode::getHeurValue() const 
{ 
    return m_packed.heurValue; 
}

inline float OpeningBookNode::getPropValue() const 
{ 
    return m_packed.propValue; 
}

inline HexPoint OpeningBookNode::getPropSucc() const 
{ 
    return m_packed.propSucc; 
}

inline float OpeningBookNode::getExpPriority() const 
{ 
    return m_packed.expPriority; 
}

inline HexPoint OpeningBookNode::getExpSucc() const 
{ 
    return m_packed.expSucc; 
}

inline bitset_t OpeningBookNode::getChildren() const
{
    return m_packed.children;
}

inline hash_t OpeningBookNode::Hash() const
{ 
    return m_hash; 
}

inline void OpeningBookNode::SetHash(hash_t hash)
{ 
    m_hash = hash; 
}

inline int OpeningBookNode::PackedSize() const
{
    return sizeof(OpeningBookNode::Packed);
}

inline byte* OpeningBookNode::Pack() const
{
    static OpeningBookNode::Packed temp;
    temp = m_packed;
    HexAssert(temp == m_packed);
    return (byte*)&temp;
}

inline void OpeningBookNode::Unpack(const byte* t)
{
    m_packed = *(const OpeningBookNode::Packed*)t;
}

inline void OpeningBookNode::SetChildren(const bitset_t& children)
{
    m_packed.children = children;
}

//----------------------------------------------------------------------------

/** Opening Book class. */
class OpeningBook
{
public:

    //------------------------------------------------------------------------
    /** Settings for this book. */
    struct Settings
    {
        /** Board width for all states in this book. */
        int board_width;

        /** Board height for all states in this book. */
        int board_height;

        /** Alpha used in state expansion. */
        double alpha;

        bool operator==(const Settings& o) const 
        {
            return (board_width == o.board_width && 
                    board_height == o.board_height && 
                    alpha == o.alpha);
        }
        
        bool operator!=(const Settings& o) const
        {
            return !(*this == o);
        }

        std::string toString() const
        {
            std::ostringstream os;
            os << "["
               << "W=" << board_width << ", "
               << "H=" << board_height << ", "
               << "Alpha=" << alpha 
               << "]";
            return os.str();
        }
    };
    
    //---------------------------------------------------------------------

    /** Constructor. Creates an OpeningBook for boardsize (width,
        height) to be stored in the file filename.  The parameter
        alpha controls state expansion (big values give rise to deeper
        lines, while small values perform like a BFS).
    */
    OpeningBook(int width, int height, double alpha, std::string filename);

    /** Destructor. */
    ~OpeningBook();

    /** Returns a copy of the settings for this book. */
    Settings GetSettings() const;

    /** Returns the node for the given hash. If node is not in db,
        returns node such that node.IsDummy() is true. */
    OpeningBookNode GetNode(hash_t hash) const;

    //---------------------------------------------------------------------

    /** Expands numExpansions leaves starting from given state. Note
        that a single leaf expansion typically involves many
        evaluations (one per successor). */
    void Expand(HexBoard& brd, int numExpansions);

    /** Returns the depth of the mainline from the given position. */
    int GetMainLineDepth(const StoneBoard& pos, HexColor color) const;

    /** Returns the number of nodes in the tree rooted at the current
        position. */
    int GetTreeSize(StoneBoard& brd, HexColor color) const;

private:

    /** Returns evaluation for a state; detects vc win/loss states. */
    HexEval EvalState(HexBoard& brd) const;

    /** Creates a child node from the given parent; brd must be in the
        child state.*/
    OpeningBookNode CreateChild(HexBoard& brd, 
                                const OpeningBookNode& parent);

    /** Updates an internal node's propagation and expansion priority
        values.  Does nothing if a terminal node. */    
    void UpdateNode(HexBoard& brd);

    /** Starts at root. Follows expansion successors until a leaf
        or a terminal node. */
    void FindNextLeafToExpand(HexBoard& brd, 
                              MoveSequence& variation);

    /** Creates a node for each of the leaf's children (except for
        those that already exist). */
    void ExpandLeaf(HexBoard& brd);

    /** This method updates all nodes on variation using
        UpdateNode(). */
    void PropagateValuesUpVariation(HexBoard& brd, 
                                    MoveSequence& variation);
    
    //---------------------------------------------------------------------

    int tree_size(StoneBoard& brd, HexColor color,
                  std::map<hash_t, int>& solved) const;

    //---------------------------------------------------------------------

    Settings m_settings;
    
    //---------------------------------------------------------------------

    /** @name State data/parameters used when building books.
        Should be moved out of here when OpeningBook is split into
        data/builder classes. 
    */
    // @{
    
    mutable std::map<hash_t, OpeningBookNode> m_leafs;

    HashDB<OpeningBookNode> m_db;

    bool m_dampen_scores;
    
    int m_flush_iterations;

    bool m_use_cache;

    // @}

    //---------------------------------------------------------------------    
    
    /** Runtime statistics. */
    struct Statistics
    {
        int cache_misses;
        int cache_writes;
        int cache_reads;
        int shrinkings;
        int shrunk_cells;

        Statistics()
            : cache_misses(0), cache_writes(0), cache_reads(0),
              shrinkings(0), shrunk_cells(0)
        { }
    };

    mutable Statistics m_statistics;
};

inline OpeningBook::Settings OpeningBook::GetSettings() const
{
    return m_settings;
}

//----------------------------------------------------------------------------

#endif // OPENINGBOOK_HPP
