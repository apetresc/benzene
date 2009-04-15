//----------------------------------------------------------------------------
// $Id: Connections.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef CONNECTIONS_HPP
#define CONNECTIONS_HPP

#include "Hex.hpp"
#include "GroupBoard.hpp"
#include "VC.hpp"
#include "VCList.hpp"
#include "VCPattern.hpp"

//----------------------------------------------------------------------

/**
 * Builds the Virtual Connections (VCs) between groups of stones for
 * one color. 
 *
 * VCs can be built from scratch or incrementally from a previous
 * state.  We use Anchelevich's rules for VC computation. This means
 * that between each pair of cells on the board, we store a VCList
 * of FULL connections and another VCList of SEMI connections.  
 *
 * IMPORTANT: Take a list of semis between x and y.  If any subset of
 * of these semis has an empty intersection, we require that the list
 * of full connections between x and y has at least one connection.
 *
 */
class Connections
{
public:

    /** Creates a Connections class on the given board for color. */
    Connections(GroupBoard& brd, HexColor color, 
                const VCPatternSet& patterns,
                ChangeLog<VC>& log);

    /** Destructor. */
    virtual ~Connections();

    //------------------------------------------------------------

    /** Force access to the proper hex::setting variable to
        obtain the default value.  Used the maximum number of semis
        that can be used at a time in an OR computation. */
    static const int DEFAULT = -1;

    //------------------------------------------------------------

    /** Determines if there is at least one valid connection between
        the given pair of cells for the color and VC type.  Note
        that both x and y are not of our opponent's color. */
    bool doesVCExist(HexPoint x, HexPoint y, VC::Type type) const;

    /** Copies the smallest connection between x and y of type into
        out, returns true if successful.  Returns false if
        no connection exists between x and y. */
    bool getSmallestVC(HexPoint x, HexPoint y, VC::Type type, VC& out) const;

    /** Stores the valid connections between x and y for color in
        out (which is cleared beforehand). */
    void getVCs(HexPoint x, HexPoint y, VC::Type type,
                std::vector<VC>& out) const;

    /** Returns the list of VCs between x and y of type. */
    const VCList& getVCList(HexPoint x, HexPoint y, VC::Type type) const;

    /** Determines the cells with which the given cell has a valid
        connection.  This set will include all members of a group, not
        just the group captain. */
    bitset_t ConnectedTo(HexPoint x, VC::Type type) const;

    /** Dumps the vcs to a string. */
    std::string dump(VC::Type type) const;

    /** Stores the total number of fulls and semis. */
    void size(int& fulls, int& semis) const;

    //----------------------------------------------------------------------

    /** Returns true if other is isomorphic to us. */
    bool operator==(const Connections& other) const;

    /** Returns true if other is not isomorphic to us. */
    bool operator!=(const Connections& other) const;

    //----------------------------------------------------------------------
    
    /** Clears the connections. */
    void clear();

    /** Reverts vcs to state at last marker in the changelog. */
    void revert();

    /** Old connections are removed prior to starting.  Logging is
        turned off.  Considers up to max_ors 1-connections when
        building 0-connections.  */
    void build(int max_ors = DEFAULT);

    /** Constant for passing into buildVCs() below. */
    static const bool DO_NOT_MARK_LOG = false;
    
    /** Computes connections on this board for the given set of added
        stones.  Assumes existing vc data is valid for the state prior
        to these stones being played.  Logging is turned on.  If
        mark_the_log is true, marker is played on log to tell
        revertVCs() when to stop, if false, no such marker is placed.
        
        Breaks all connections whose carrier contains a new stone
        unless a 1-connection of player color and p is the key; these
        are upgraded to 0-connections for player p.  Considers up to
        max_ors 1-connections when building 0-connections.
    */
    void build(bitset_t added[BLACK_AND_WHITE],
               int max_ors = DEFAULT, 
               bool mark_the_log = true);

protected:

private:

    /** Called by construtor. */
    void init();

    /** Turns the ChangeLog on or off. */
    void logging(bool flag);

    /** Marks the ChangeLog. */
    void mark_log();

    //----------------------------------------------------------------------
    // VC Statistics
    //----------------------------------------------------------------------
    
    /** Statistics for the current call to build(). */
    struct Statistics
    {
        /** Number of adds tried in addBaseVCs(). */
        int base_attempts;

        /** Number of adds that succeeded in addBaseVCs(). */
        int base_successes;

        /** Number of adds tried in AddPatternVCs(). */
        int pattern_attempts;
        
        /** Number of adds that succeeded in AddPatternVCs(). */
        int pattern_successes;

        /** Number of fulls tried in doAnd(). */
        int and_full_attempts;

        /** Number of fulls successfully added in doAnd(). */
        int and_full_successes;

        /** Number of semis tried in doAnd(). */
        int and_semi_attempts;
        
        /** Number of semis successfully added in doAnd(). */
        int and_semi_successes;


        /** Number of adds that succeeded in doAnd(). */
        int and_successes;


        /** Number of adds tried in doOr(). */
        int or_attempts;

        /** Number of addes that succeeded in doOr(). */
        int or_successes;


        /** Number of adds tried in doPushRule(). */
        int push_attempts;

        /** Number of addes that succeeded in doPushRule(). */
        int push_successes;


        /** Number of calls to doOr(). */
        int doOrs;

        /** Number of calls to doOr() that succeeded in adding at
            least one full to the list of fulls. */
        int goodOrs;


        /** Number of fulls shrunk. */
        int shrunk0;

        /** Number of semis shrunk. */
        int shrunk1;

        /** Number of semis upgraded to fulls. */
        int upgraded;

        /** Number of fulls killed by opponent stones. */
        int killed0;
        
        /** Number of semis killed by opponent stones. */
        int killed1;

        //------------------------------------------------------------------
        
        /** Dump statistics to a string. */
        std::string toString() const
        {
            std::ostringstream os;
            os << "["
               << "base:" << base_successes << "/" << base_attempts << ", "
               << "pat:" << pattern_successes << "/" << pattern_attempts << ", " 
               << "and-f:" << and_full_successes << "/" << and_full_attempts << ", "
               << "and-s:" << and_semi_successes << "/" << and_semi_attempts << ", "
               << "push-s:" << push_successes << "/" << push_attempts << ", "

               << "or:" << or_successes << "/" << or_attempts << ", "
               << "doOr():" << goodOrs << "/" << doOrs << ", "
               << "s0/s1/u1:" << shrunk0 << "/" << shrunk1 << "/"<< upgraded << ", "
               << "killed0/1:" << killed0 << "/" << killed1 
               << "]";
            return os.str();
        }
    };

    /** Clears all statistics. */
    void clearStats();
    
    //----------------------------------------------------------------------
    // VC Queue
    //----------------------------------------------------------------------

    /** Queue of endpoint pairs that need processing. */
    class VCQueue
    {
    public:
        bool empty() const;
        const HexPointPair& front() const;

        void clear();
        void pop();
        void add(const HexPointPair& pair);

    private:
        std::queue<HexPointPair> m_queue;
        bool m_seen[BITSETSIZE][BITSETSIZE];
    };
    
    //----------------------------------------------------------------------
    // VC Computation methods
    //----------------------------------------------------------------------

    /** The types of VC to create when using the AND rule. */
    typedef enum {CREATE_FULL, CREATE_SEMI} AndRule;

    /** Performs pairwise comparisons of connections between a and b,
        adds those with empty intersection into out.
        
        @return number of connections successfully added.
    */
    int doAnd(HexPoint from, HexPoint over, HexPoint to,
              AndRule rule, const VC& vc, const VCList* old);

    /** Runs over all subsets of size 2 to max_ors of semis containing
        vc and adds the union to out if it has an empty intersection.
        
        @return number of connections successfully added.
    */
    int doOr(const VC& vc, 
             const VCList* semis, VCList* fulls, 
             std::list<VC>& added, 
             int max_ors);

    /** Computes the and closure for the vc.  Let x and y be vc's
        endpoints. A single pass over the board is performed. For each
        z, we try to and the list of fulls between z and x and z and y
        with vc.
        
        @return number of connections successfully added.
    */
    void andClosure(const VC& vc);

    /** Performs push rule. */
    int doPushRule(const VC& vc, const VCList* semi_list);


    /** Performs the VC search. */
    void do_search(int max_ors);
    void process_semis(HexPoint xc, HexPoint yc, int max_ors);
    void process_fulls(HexPoint p1, HexPoint p2);

    //------------------------------------------------------------

    /** Tries to add a new full-connection to list between 
        (vc.x(), vc.y()).

        If vc is successfully added, then:

        1) Removes any semi-connections between (vc.x(), vc.y()) that
        are supersets of vc.

        2) Adds (vc.x(), vc.y()) to the queue if vc was added inside
        the soft-limit.  
    */
    bool AddNewFull(const VC& vc);
    
    /** Tries to add a new semi-connection to list between 
        (vc.x(), vc.y()). 
        
        Does not add if vc is a superset of some full-connection
        between (vc.x(), and vc.y()).

        If vc is successfully added and intersection on semi-list is
        empty, then:

        1) if vc added inside soft limit, adds (vc.x(), vc.y()) to queue.
        
        2) otherwise, if no full exists between (vc.x(), vc.y()), 
        adds the or over the entire semi list. 
    */
    bool AddNewSemi(const VC& vc);

    //------------------------------------------------------------
    // Static VC helper methods

    /** Computes the 0-connections defined by adjacency for color. */
    void addBaseVCs();

    /** Adds vc's obtained by pre-computed patterns. */
    void AddPatternVCs();

    //------------------------------------------------------------
    // Incremental VC helper methods

    /** Absrobs, shrinks, upgrades all connections given the stones
        that were recently added.  This is performed in a single pass. */
    void AbsorbMergeShrinkUpgrade(const bitset_t& added_black,
                                  const bitset_t& added_white);

    void MergeAndShrink(const bitset_t& affected,
                        const bitset_t& added);
    
    void MergeAndShrink(const bitset_t& added, 
                        HexPoint xin, HexPoint yin,
                        HexPoint xout, HexPoint yout);
    
    /** Removes all connections whose interesting with added is
        non-empty. */
    void RemoveAllContaining(const bitset_t& added);
    
    //------------------------------------------------------------

    void VerifyIntegrity() const;

    //------------------------------------------------------------

    GroupBoard& m_brd;

    ChangeLog<VC>& m_log;

    const VCPatternSet& m_patterns;

    HexColor m_color;

    VCQueue m_queue;

    Statistics m_statistics;
    
    /** The lists of vcs. @todo use actual boardsize instead of
        BITSETSIZE? */
    VCList* m_vc[VC::NUM_TYPES][BITSETSIZE][BITSETSIZE];

};

//----------------------------------------------------------------------------

#endif // CONNECTIONS_HPP
