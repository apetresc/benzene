//----------------------------------------------------------------------------
// $Id: ICEngine.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef ICENGINE_H
#define ICENGINE_H

#include "Digraph.hpp"
#include "InferiorCells.hpp"
#include "IcePatternSet.hpp"
#include "HandCodedPattern.hpp"
#include "PatternBoard.hpp"

//----------------------------------------------------------------------------

/** Inferior Cell Engine. 
    Finds inferior cells on a given boardstate.
*/
class ICEngine
{
public:

    //------------------------------------------------------------------------

    /** Initializes the engine.  Loads patterns from
        "uct-pattern-file". */
    ICEngine(const std::string& config_dir);

    /** Destructor. */
    virtual ~ICEngine();

    //------------------------------------------------------------------------

    /** Returns the dead cells in the given board. */
    bitset_t findDead(const PatternBoard& board, 
                      const bitset_t& consider) const;
    
    /** Compute regions that are sealed from one or more of the edges.
        These areas are dead, but may not be identified via patterns,
        etc. Thus we use a BFS-type algorithm, checking which areas we
        can reach from an edge without going through the opposite edge
        or stones of the opponent's colour. Note that if the game is
        already decided, all remaining empty cells are dead.
    */
    bitset_t computeEdgeUnreachableRegions(const StoneBoard& brd,
					   HexColor c,
					   const bitset_t& stopSet,
					   bool flowFromEdge1=true,
					   bool flowFromEdge2=true) const;
    bitset_t computeDeadRegions(const GroupBoard& brd) const;
    bitset_t findType1Cliques(const GroupBoard& brd) const;
    bitset_t findType2Cliques(const GroupBoard& brd) const;
    bitset_t findType3Cliques(const GroupBoard& brd) const;
    bitset_t findThreeSetCliques(const GroupBoard& brd) const;
    
    /** Computes only the dead and captured cells; board will be
        modified to have the captured cells filled-in.  Dead cells
        will be filled in with color DEAD_COLOR.  Returns number of
        cells filled-in.
        
        Pattern and group info must be up-to-date.

        Pattern and group info we be up-to-date upon returning from
        this method.
    */
    int computeDeadCaptured(PatternBoard& board, 
                            InferiorCells& inf, 
                            HexColorSet colors_to_capture) const;
    

    /** Fills in all cells it can; calls computeDeadCaptured, then
        fills-in presimplicial pairs, iterates until convergence.
        Unreachable regions are then filled-in using a BFS. 
    */
    void computeFillin(HexColor color,
                       PatternBoard& board, 
                       InferiorCells& out,
                       HexColorSet colors_to_capture=ALL_COLORS) const;

    /** Categorizes cells as dead, captured, etc.  board will be
        modified to have the captured cells filled in.  Dead cells
        will be filled in with color's stones.
        
        Pattern and group info must be up-to-date (group info do to
        pre-simplicial pair fill-in).

        Pattern and group info we be up-to-date upon returning from
        this method. 
    */
    void computeInferiorCells(HexColor color, 
                              PatternBoard& board, 
                              InferiorCells& out) const;
    
    /** Finds vulnerable cells among the consider set.
        Note: only public for htp/gui access.
    */
    void findVulnerable(const PatternBoard& board,
			HexColor col,
			const bitset_t& consider,
			InferiorCells& inf) const;
    
    /** Finds dominated cells among the consider set.
        Note: only public for htp/gui access.
    */
    void findDominated(const PatternBoard& board, 
                       HexColor color, 
                       const bitset_t& consider,
                       InferiorCells& inf) const;
    
private:

    void LoadHandCodedPatterns();
    void LoadPatterns();

    void CheckHandCodedDominates(const StoneBoard& brd, HexColor color,
                                 const HandCodedPattern& pattern,
                                 const bitset_t& consider,
                                 bitset_t& out, HexPoint* dominating) const;


    //------------------------------------------------------------

    int BackupOpponentDead(HexColor color, 
                           const PatternBoard& board, 
                           InferiorCells& out) const;
    
    /** Methods to find various types of inferior cells. */
    bitset_t findPresimplicialPairs(PatternBoard& brd, 
                                    HexColor color) const;

    int handlePermanentlyInferior(PatternBoard& board, 
                                  InferiorCells& out, 
                                  HexColorSet colors_to_capture) const; 
    
    int fillInUnreachable(PatternBoard& board, 
                          InferiorCells& out) const;


    /** Finds vulnerable cells for color and finds presimplicial pairs
        and fills them in for the other color.  Simplicial stones will
        be added as dead and played to the board as DEAD_COLOR. */
    int fillInVulnerable(HexColor color, 
                         PatternBoard& board, 
                         InferiorCells& inf, 
                         HexColorSet colors_to_capture) const;

    bitset_t findCaptured(const PatternBoard& board, HexColor color, 
                          const bitset_t& consider) const;

    bitset_t findPermanentlyInferior(const PatternBoard& board, 
                                     HexColor color, 
                                     const bitset_t& consider,
                                     bitset_t& carrier) const;

    void findHandCodedDominated(const StoneBoard& board, 
                                HexColor color,
                                const bitset_t& consider, 
                                InferiorCells& inf) const;

    void CheckHandCodedDominates(const StoneBoard& brd,
                                 HexColor color,
                                 const HandCodedPattern& pattern, 
                                 const bitset_t& consider, 
                                 InferiorCells& inf) const;
        
    //------------------------------------------------------------

    /** Configuration directory; used to load patterns. */
    std::string m_config_dir;

    /** Hand coded patterns. */
    std::vector<HandCodedPattern> m_hand_coded;

    /** Patterns. */
    IcePatternSet m_patterns;
};

//----------------------------------------------------------------------------

/** Utilities needed by ICE. */
namespace IceUtil 
{
    /** Returns true if the vector of points forms a clique on brd,
        while excluding any checks on exclude (to find pre-simplicial
        cells exclude should be in vn). */
    bool IsClique(const ConstBoard& brd, 
                  const std::vector<HexPoint>& vn, 
                  HexPoint exclude=INVALID_POINT);


    /** Finds simplicial/presimplicial cells. Simplicial cells are
        added as dead to inf and DEAD_COLOR to the board. */
    void FindPresimplicial(HexColor color, 
                           PatternBoard& brd,
                           InferiorCells& inf);

    /** Add the inferior cell info from in to the inferior cell info
        of out. Ensures no dead cells are added into
        perm.inf. carriers; they are added as captured instead.  If
        this occurs, the board is updated to reflect this change. */
    void Update(InferiorCells& out, 
                const InferiorCells& in, 
                PatternBoard& brd);
}

//----------------------------------------------------------------------------

#endif // ICENGINE_H
