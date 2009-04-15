//----------------------------------------------------------------------------
// $Id: ConnectionBuilder.cpp 1913 2009-02-13 02:03:21Z ph $
//----------------------------------------------------------------------------

#include "Hex.hpp"
#include "Time.hpp"
#include "BitsetIterator.hpp"
#include "GraphUtils.hpp"
#include "ChangeLog.hpp"
#include "ConnectionBuilder.hpp"
#include "Connections.hpp"
#include "VCPattern.hpp"
#include "VCUtils.hpp"

//----------------------------------------------------------------------------

ConnectionBuilderParam::ConnectionBuilderParam()
    : max_ors(4),
      and_over_edge(false),
      use_patterns(false),
      use_push_rule(false),
      use_greedy_union(true),
      abort_on_winning_connection(false)
{
}

//----------------------------------------------------------------------------

ConnectionBuilder::ConnectionBuilder(ConnectionBuilderParam& param)
    : m_param(param)
{
}

ConnectionBuilder::~ConnectionBuilder()
{
}

//----------------------------------------------------------------------------

// Static VC construction

void ConnectionBuilder::Build(Connections& con, const GroupBoard& brd)
{
    m_con = &con;
    m_color = con.Color();
    m_brd = &brd;
    m_log = 0;

    double s = HexGetTime();
    m_con->Clear();
    m_statistics = ConnectionBuilderStatistics();
    m_queue.clear();

    AddBaseVCs();
    if (m_param.use_patterns)
        AddPatternVCs();
    DoSearch();

    double e = HexGetTime();
    hex::log << hex::fine << "  " << (e-s) << "s to build vcs." << hex::endl;
}

/** Computes the 0-connections defined by adjacency.*/
void ConnectionBuilder::AddBaseVCs()
{
    HexColorSet not_other = HexColorSetUtil::ColorOrEmpty(m_color);
    for (BoardIterator x(m_brd->Groups(not_other)); x; ++x) 
    {
        bitset_t adj = m_brd->Nbs(*x, EMPTY);
        for (BitsetIterator y(adj); y; ++y) 
        {
            VC vc(*x, *y);
            m_statistics.base_attempts++;
            if (m_con->Add(vc, m_log))
            {
                m_statistics.base_successes++;
                m_queue.push(std::make_pair(vc.x(), vc.y()));
            }
        }
    }
}

/** Adds vcs obtained by pre-computed patterns. */
void ConnectionBuilder::AddPatternVCs()
{
    const VCPatternSet& patterns 
        = VCPattern::GetPatterns(m_brd->width(), m_brd->height(), m_color);
    for (std::size_t i=0; i<patterns.size(); ++i) 
    {
        const VCPattern& pat = patterns[i];
        if (pat.Matches(m_color, *m_brd)) 
        {
            bitset_t carrier = pat.NotOpponent() - m_brd->getColor(m_color);
            carrier.reset(pat.Endpoint(0));
            carrier.reset(pat.Endpoint(1));
            VC vc(pat.Endpoint(0), pat.Endpoint(1), carrier, VC_RULE_BASE);

            m_statistics.pattern_attempts++;
            if (m_con->Add(vc, m_log))
            {
                m_statistics.pattern_successes++;
                m_queue.push(std::make_pair(vc.x(), vc.y()));
            }
        }
    }
}

//----------------------------------------------------------------------------
// Incremental VC construction

void ConnectionBuilder::Build(Connections& con, const GroupBoard& brd,
                              bitset_t added[BLACK_AND_WHITE],
                              ChangeLog<VC>* log)
{
    HexAssert((added[BLACK] & added[WHITE]).none());

    m_con = &con;
    m_color = con.Color();
    m_brd = &brd;
    m_log = log;

    double s = HexGetTime();
    m_statistics = Statistics();
    m_queue.clear();

    Merge(added);
    if (m_param.use_patterns)
        AddPatternVCs();
    DoSearch();

    double e = HexGetTime();
    hex::log << hex::fine
             << "  " << (e-s) << "s to build vcs incrementally." 
             << hex::endl;
}

/** @page mergeshrink Incremental Update Algorithm
    
    The connection set is updated to the new state of the board in a
    single pass. In this pass connections touched by opponent stones
    are destroyed, connections touched by friendly stones are resized,
    and connections in groups that are merged into larger groups are
    merged into the proper connection lists. This entire process is
    called the "merge".
    
    The merge begins by noting the set of "affected" stones. These are
    the stones that were just played as well as those groups adjacent
    to the played stones.

    Any list with either endpoint in the affected set will need to
    either pass on its connections to the list now responsible for
    that group or recieve connections from other lists that it is now
    responsible for. Lists belonging to groups that are merge into
    other groups are not destroyed, they remain so that undoing this
    merge is more efficient.

    Every list needs to be checked for shrinking.
    
    TODO Finish this documentation!
            
*/
void ConnectionBuilder::Merge(bitset_t added[BLACK_AND_WHITE])
{
    /** NOTE: ConnectionBuilder takes a constant board, and so we need
        to guarantee that the board is the same when we leave this
        method as it was when we entered. This is the only method that
        should be modifying the board!

        Here we check that the cells are the same at exit as on
        entering, but it's possible the group/pattern info is
        corrupted and this check misses it.

        Ideally, we want to de-couple the group info from the board so
        this would not be a problem. In which case, ConnectionBuilder
        would not even see any board at all.
    */
#ifndef NDEBUG
    StoneBoard old(*m_brd);
#endif
    GroupBoard* brd = const_cast<GroupBoard*>(m_brd);

    // Remove added stones and compute groups for the original state.
    brd->setColor(m_color, m_brd->getColor(m_color) - added[m_color]);
    brd->absorb();
        
    // Kill connections containing stones the opponent just played.
    // NOTE: This *must* be done in the original state, not in the
    // state with the newly added stones. 
    RemoveAllContaining(*brd, added[!m_color]);
        
    // Find groups adjacent to any played stone of color; add them to
    // the affected set along with the played stones.
    bitset_t affected = added[m_color];
    for (BitsetIterator x(added[m_color]); x; ++x)
        for (BoardIterator y(brd->ConstNbs(*x)); y; ++y)
            if (brd->getColor(*y) == m_color)
                affected.set(brd->getCaptain(*y));                    

    // Replace removed stones and update group info
    brd->addColor(m_color, added[m_color]);
    brd->absorb(added[m_color]);

    MergeAndShrink(affected, added[m_color]);

#ifndef NDEBUG
    HexAssert(*m_brd == old);
#endif
}

void ConnectionBuilder::MergeAndShrink(const bitset_t& affected,
                                       const bitset_t& added)
{
    HexColorSet not_other = HexColorSetUtil::NotColor(!m_color);
    for (BoardIterator x(m_brd->Stones(not_other)); x; ++x) 
    {
        if (!m_brd->isCaptain(*x) && !affected.test(*x)) 
            continue;
        for (BoardIterator y(m_brd->Stones(not_other)); *y != *x; ++y) 
        {
            if (!m_brd->isCaptain(*y) && !affected.test(*y)) 
                continue;
            HexPoint cx = m_brd->getCaptain(*x);
            HexPoint cy = m_brd->getCaptain(*y);

            // Lists between (cx, cx) are never used, so only do work
            // if it's worthwhile. This can occur if y was recently
            // played next to group x, now they both have the same
            // captain, so no point merging old connections into
            // (captain, captain).
            if (cx != cy) 
            {
                m_queue.push(std::make_pair(cx, cy));
                MergeAndShrink(added, *x, *y, cx, cy);
            }
        }
    }
}

void ConnectionBuilder::MergeAndShrink(const bitset_t& added, 
                                       HexPoint xin, HexPoint yin,
                                       HexPoint xout, HexPoint yout)
{
    HexAssert(xin != yin);
    HexAssert(xout != yout);

    VCList* fulls_in = &m_con->GetList(VC::FULL, xin, yin);
    VCList* semis_in = &m_con->GetList(VC::SEMI, xin, yin);
    VCList* fulls_out= &m_con->GetList(VC::FULL, xout, yout);
    VCList* semis_out= &m_con->GetList(VC::SEMI, xout, yout);

    HexAssert((fulls_in == fulls_out) == (semis_in == semis_out));
    bool doing_merge = (fulls_in != fulls_out);

    std::list<VC> removed;
    std::list<VC>::iterator it;

    // 
    // Shrink all 0-connections.
    //
    // If (doing_merge) transfer remaining connections over as well. 
    //
    fulls_in->removeAllContaining(added, removed, m_log);
    if (doing_merge) 
    { 
        // Copied vc's will be set to unprocessed explicitly.
        /** @bug There could be supersets of these fulls in semis_out! */
        fulls_out->add(*fulls_in, m_log);
    }

    for (it = removed.begin(); it != removed.end(); ++it) 
    {
        VC v = VC::ShrinkFull(*it, added, xout, yout);
        /** @bug There could be supersets of these fulls in semis_out! */
        if (fulls_out->add(v, m_log))
            m_statistics.shrunk0++;
    }

    //
    // Shrink all 1-connections.
    // if (doing_merge) transfer remaining connections
    // over as well. 
    //
    removed.clear();
    semis_in->removeAllContaining(added, removed, m_log);
    if (doing_merge) 
    {
        // Copied vc's will be set to unprocessed explicitly.
        /** @bug These could be supersets of fulls_out. */
        semis_out->add(*semis_in, m_log);
    }

    // Shrink connections that touch played cells.
    // Do not upgrade during this step. 
    for (it = removed.begin(); it != removed.end(); ++it) 
    {
        if (!added.test(it->key())) 
        {
            VC v = VC::ShrinkSemi(*it, added, xout, yout);
            /** @bug These could be supersets of fulls_out. */
            if (semis_out->add(v, m_log))
                m_statistics.shrunk1++;
        }
    }

    // Upgrade semis. Need to do this after shrinking to ensure
    // that we remove all sc supersets from semis_out.
    for (it = removed.begin(); it != removed.end(); ++it) 
    {
        if (added.test(it->key())) 
        {
            VC v = VC::UpgradeSemi(*it, added, xout, yout);
            if (fulls_out->add(v, m_log))
            {
                // Remove supersets from the semi-list; do not
                // invalidate list intersection since this semi was a
                // member of the list. Actually, this probably doesn't
                // matter since the call to removeAllContaining()
                // already clobbered the intersections.
                semis_out->removeSuperSetsOf(v.carrier(), m_log, false);
                m_statistics.upgraded++;
            }
        }
    }
}

/** Removes all connections whose intersection with given set is
    non-empty. Any list that is modified is added to the queue, since
    some unprocessed connections could have been brought under the
    softlimit.
*/
void ConnectionBuilder::RemoveAllContaining(const GroupBoard& brd,
                                            const bitset_t& bs)
{
    HexColorSet not_other = HexColorSetUtil::NotColor(!m_color);
    for (BoardIterator x(brd.Groups(not_other)); x; ++x) 
    {
        for (BoardIterator y(brd.Groups(not_other)); *y != *x; ++y) 
        {
            int cur0 = m_con->GetList(VC::FULL,*x, *y)
                .removeAllContaining(bs, m_log);
            m_statistics.killed0 += cur0; 
            int cur1 = m_con->GetList(VC::SEMI,*x, *y)
                .removeAllContaining(bs, m_log);
            m_statistics.killed1 += cur1;
            if (cur0 || cur1)
                m_queue.push(std::make_pair(*x, *y));
        }
    }
}

//----------------------------------------------------------------------------
// VC Construction methods
//----------------------------------------------------------------------------

void ConnectionBuilder::ProcessSemis(HexPoint xc, HexPoint yc)
{
    VCList& semis = m_con->GetList(VC::SEMI, xc, yc);
    VCList& fulls = m_con->GetList(VC::FULL, xc, yc);

    // Nothing to do, so abort. 
    if (semis.hardIntersection().any())
        return;

    int soft = semis.softlimit();
    VCList::iterator cur = semis.begin();
    VCList::iterator end = semis.end();
    std::list<VC> added;

    for (int count=0; count<soft && cur!=end; ++cur, ++count) 
    {
        if (!cur->processed()) 
        {
            if (m_param.use_push_rule)
                doPushRule(*cur, &semis);

            m_statistics.doOrs++;
            if (m_orRule(*cur, &semis, &fulls, added, m_param.max_ors, 
                         m_log, m_statistics))
            {
                m_statistics.goodOrs++;
            }

            cur->setProcessed(true);
            
            if (m_log)
                m_log->push(ChangeLog<VC>::PROCESSED, *cur);
        }
    }

    // Remove supersets of added vcs. do not invalidate list
    // intersection since v is a superset of elements of semi_list.
    // THIS IS PROBABLY IMPOSSIBLE!
    // TODO: REMOVE THIS CHECK IF NO ABORTS OCCUR BY Mar 14, 2009.
    for (cur = added.begin(); cur != added.end(); ++cur) 
    {
        if (semis.isSubsetOfAny(cur->carrier()))
        {
            hex::log << hex::info << '\n' 
                     << *m_brd << '\n'
                     << semis.dump() << '\n'
                     << "---------------" << '\n'
                     << fulls.dump() << '\n'
                     << "---------------" << '\n'
                     << *cur << hex::endl;
            abort();
        }
        //semis.removeSuperSetsOf(*cur, m_log, false);
    }

    // If no full exists, create one by unioning the entire list
    if (fulls.empty()) 
    {
        bitset_t carrier = m_param.use_greedy_union 
            ? semis.getGreedyUnion() 
            : semis.getUnion();

        fulls.add(VC(xc, yc, carrier, EMPTY_BITSET, VC_RULE_ALL), m_log);
        // @note No need to remove supersets of v from the semi
        // list since there can be none!
    } 
}

void ConnectionBuilder::ProcessFulls(HexPoint xc, HexPoint yc)
{
    VCList& fulls = m_con->GetList(VC::FULL, xc, yc);
    int soft = fulls.softlimit();
    VCList::iterator cur = fulls.begin();
    VCList::iterator end = fulls.end();
    for (int count=0; count<soft && cur!=end; ++cur, ++count) 
    {
        if (!cur->processed()) 
        {
            andClosure(*cur);
            cur->setProcessed(true);
            if (m_log)
                m_log->push(ChangeLog<VC>::PROCESSED, *cur);
        }
    }
}

void ConnectionBuilder::DoSearch()
{
    bool winning_connection = false;
    while (!m_queue.empty()) 
    {
        HexPointPair pair = m_queue.front();
        m_queue.pop();

        ProcessSemis(pair.first, pair.second);
        ProcessFulls(pair.first, pair.second);
        
        if (m_param.abort_on_winning_connection && 
            m_con->Exists(HexPointUtil::colorEdge1(m_color),
                          HexPointUtil::colorEdge2(m_color),
                          VC::FULL))
        {
            winning_connection = true;
            break;
        }
    }        
    HexAssert(winning_connection || m_queue.empty());

    if (winning_connection) 
        hex::log << hex::fine << "Aborted on winning connection." << hex::endl;
            
    // Process the side-to-side semi list to ensure we have a full if
    // mustplay is empty.
    // TODO: IS THIS STILL NEEDED?
    ProcessSemis(m_brd->getCaptain(HexPointUtil::colorEdge1(m_color)),
                 m_brd->getCaptain(HexPointUtil::colorEdge2(m_color)));
}

//----------------------------------------------------------------------------

/** Computes the and closure for the vc. Let x and y be vc's
    endpoints. A single pass over the board is performed. For each z,
    we try to and the list of fulls between z and x and z and y with
    vc. This function is a major bottleneck. Every operation in it
    needs to be as efficient as possible.
*/
void ConnectionBuilder::andClosure(const VC& vc)
{
    HexColor other = !m_color;
    HexColorSet not_other = HexColorSetUtil::NotColor(other);

    HexPoint endp[2];
    endp[0] = m_brd->getCaptain(vc.x());
    endp[1] = m_brd->getCaptain(vc.y());
    HexColor endc[2];
    endc[0] = m_brd->getColor(endp[0]);
    endc[1] = m_brd->getColor(endp[1]);

    HexAssert(endc[0] != other);
    HexAssert(endc[1] != other);
    for (BoardIterator z(m_brd->Groups(not_other)); z; ++z) 
    {
        if (*z == endp[0] || *z == endp[1]) continue;
        if (vc.carrier().test(*z)) continue;
        for (int i=0; i<2; i++) 
        {
            int j = (i + 1) & 1;
            if (m_param.and_over_edge || !HexPointUtil::isEdge(endp[i])) 
            {
                VCList* fulls = &m_con->GetList(VC::FULL, *z, endp[i]);
                if ((fulls->softIntersection() & vc.carrier()).any())
                    continue;
                
                AndRule rule = (endc[i] == EMPTY) ? CREATE_SEMI : CREATE_FULL;
                doAnd(*z, endp[i], endp[j], rule, vc, 
                      &m_con->GetList(VC::FULL, *z, endp[i]));
            }
        }
    }
}

/** Performs pairwise comparisons of connections between a and b, adds
    those with empty intersection into out.
*/
void ConnectionBuilder::doAnd(HexPoint from, HexPoint over, HexPoint to,
                              AndRule rule, const VC& vc, const VCList* old)
{
    if (old->empty())
        return;

    bitset_t stones;
    stones.set(m_brd->getCaptain(over));

    int soft = old->softlimit();
    VCList::const_iterator i = old->begin();
    VCList::const_iterator end = old->end();
    for (int count=0; count<soft && i!=end; ++i, ++count) 
    {
        if (!i->processed())
            continue;
        if (i->carrier().test(to))
            continue;
        if ((i->carrier() & vc.carrier()).any())
            continue;
        if (rule == CREATE_FULL)
        {
            m_statistics.and_full_attempts++;
            if (AddNewFull(VC::AndVCs(from, to, *i, vc, stones)))
                m_statistics.and_full_successes++;
        }
        else if (rule == CREATE_SEMI)
        {
            m_statistics.and_semi_attempts++;
            if (AddNewSemi(VC::AndVCs(from, to, *i, vc, over)))
                m_statistics.and_semi_successes++;
        }
    }
}

/** Runs over all subsets of size 2 to max_ors of semis containing vc
    and adds the union to out if it has an empty intersection. This
    function is a major bottleneck and so needs to be as efficient as
    possible.

    TODO: Document this more!

    TODO: Check if unrolling the recursion really does speed it up.

    @return number of connections successfully added.
*/
int ConnectionBuilder::OrRule::operator()(const VC& vc, 
                                          const VCList* semi_list, 
                                          VCList* full_list, 
                                          std::list<VC>& added, 
                                          int max_ors,
                                          ChangeLog<VC>* log, 
                                          ConnectionBuilderStatistics& stats)
{
    if (semi_list->empty())
        return 0;
    
    // copy processed semis (unprocessed semis are not used here)
    m_semi.clear();
    int soft = semi_list->softlimit();
    VCList::const_iterator it = semi_list->begin();
    VCList::const_iterator end = semi_list->end();
    for (int count=0; count<soft && it!=end; ++count, ++it)
        if (it->processed())
            m_semi.push_back(*it);

    if (m_semi.empty())
        return 0;

    std::size_t N = m_semi.size();

    // for each i in [0, N-1], compute intersection of semi[i, N-1]
    if (m_tail.size() < N)
        m_tail.resize(N);
    m_tail[N-1] = m_semi[N-1].carrier();
    for (int i=N-2; i>=0; --i)
        m_tail[i] = m_semi[i].carrier() & m_tail[i+1];

    max_ors--;
    HexAssert(max_ors < 16);

    std::size_t index[16];
    bitset_t ors[16];
    bitset_t ands[16];

    ors[0] = vc.carrier();
    ands[0] = vc.carrier();
    index[1] = 0;
    
    int d = 1;
    int count = 0;
    while (true) 
    {
        std::size_t i = index[d];

        // the current intersection (some subset from [0, i-1]) is not
        // disjoint with the intersection of [i, N), so stop.
        if ((i < N) && (ands[d-1] & m_tail[i]).any()) 
            i = N;

        if (i == N) 
        {
            if (d == 1) 
                break;
            
            ++index[--d];
            continue;
        }
        
        ands[d] = ands[d-1] & m_semi[i].carrier();
        ors[d]  =  ors[d-1] | m_semi[i].carrier();

        if (ands[d].none()) 
        {
            /** Create a new full.
                
                @note We do no use AddNewFull() because if add is
                successful, it checks for semi-supersets and adds the
                list to the queue. Both of these operations are not
                needed here.
            */
            VC v(full_list->getX(), full_list->getY(), ors[d], 
                 EMPTY_BITSET, VC_RULE_OR);

            stats.or_attempts++;
            if (full_list->add(v, log) != VCList::ADD_FAILED) 
            {
                count++;
                stats.or_successes++;
                added.push_back(v);
            }
        
            ++index[d];
        } 
        else if (ands[d] == ands[d-1]) 
        {
            // this connection does not shrink intersection so skip it
            ++index[d];
        } 
        else 
        {
            // this connection reduces intersection, if not at max depth
            // see if more semis can reduce it to the empty set. 
            if (d < max_ors) 
                index[++d] = ++i;
            else
                ++index[d];
        }
    }
    return count;
}

/** Performs Push-rule. 

    DOCUMENT ME, PHIL!

    TODO: Reuse std::vectors to reduce dynamic allocations?
*/
void ConnectionBuilder::doPushRule(const VC& vc, const VCList* semi_list)
{
    if (m_brd->getColor(vc.x()) != EMPTY || m_brd->getColor(vc.y()) != EMPTY)
        return;
    
    // copy processed semis
    std::vector<VC> semi;
    int soft = semi_list->softlimit();
    VCList::const_iterator it = semi_list->begin();
    VCList::const_iterator end = semi_list->end();
    for (int count=0; count<soft && it!=end; ++count, ++it) {
        if (it->processed())
            semi.push_back(*it);
    }
    if (semi.empty()) 
        return;

    // the endpoints will be the keys to any semi-connections we create
    std::vector<HexPoint> keys;
    keys.push_back(vc.x());
    keys.push_back(vc.y());
    
    // track if an SC has empty mustuse and get captains for vc's mustuse
    bitset_t mu[3];
    bool has_empty_mustuse0 = vc.stones().none();
    mu[0] = m_brd->CaptainizeBitset(vc.stones());

    for (std::size_t i=0; i<semi.size(); ++i) {
        const VC& vi = semi[i];

        bool has_empty_mustuse1 = has_empty_mustuse0;
        
        bool has_miai1 = false;
        HexPoint miai_endpoint = INVALID_POINT;
        HexPoint miai_edge = INVALID_POINT;

        {
            bitset_t I = vi.carrier() & vc.carrier();
            if (I.none()) 
            {
                // good!
            } 
            else if (I.count() == 2)
            {
                HexPoint k,e;
                if (VCUtils::ValidEdgeBridge(*m_brd, I, k, e) 
                    && (k==keys[0] || k==keys[1]))
                {
                    has_miai1 = true;
                    miai_endpoint = k;
                    miai_edge = e;
                }
                else
                {
                    continue;
                }
            }
            else 
            {
                continue;
            }
        }        
        
        if (vi.stones().none()) {
            if (has_empty_mustuse1) continue;
            has_empty_mustuse1 = true;
        }
                
        // get captains for vi's mustuse and check for intersection
        // with mu[0].
	mu[1] = m_brd->CaptainizeBitset(vi.stones());
        if ((mu[0] & mu[1]).any()) continue;
	

        //////////////////////////////////////////////////////////////
        for (std::size_t j=i+1; j<semi.size(); ++j) {
            const VC& vj = semi[j];

            bool has_empty_mustuse2 = has_empty_mustuse1;

            bool has_miai = has_miai1;

            {
                bitset_t I = vj.carrier() & vc.carrier();
                if (I.none()) 
                {
                    // good!
                } 
                else if (I.count() == 2 && !has_miai)
                {
                    HexPoint k,e;
                    if (VCUtils::ValidEdgeBridge(*m_brd, I, k, e) 
                        && (k==keys[0] || k==keys[1]))
                    {
                        has_miai = true;
                        miai_endpoint = k;
                        miai_edge = e;
                    }
                    else
                    {
                        continue;
                    }
                }
                else 
                {
                    continue;
                }
            }

            {
                bitset_t I = vj.carrier() & vi.carrier();
                if (I.none()) 
                {
                    // good!
                } 
                else if (I.count() == 2 && !has_miai)
                {
                    HexPoint k,e;
                    if (VCUtils::ValidEdgeBridge(*m_brd, I, k, e) 
                        && (k==keys[0] || k==keys[1]))
                    {
                        has_miai = true;
                        miai_endpoint = k;
                        miai_edge = e;
                    }
                    else 
                    {
                        continue;
                    }
                }    
                else
                {
                    continue;
                }
            }

            if (vj.stones().none()) {
                if (has_empty_mustuse2) continue;
                has_empty_mustuse2 = true;
            }
	    
            // get captains for vj's mustuse and check for intersection
            // with mu[0] and mu[1].
	    mu[2] = m_brd->CaptainizeBitset(vj.stones());
	    if ((mu[2] & (mu[0] | mu[1])).any()) continue;
            
            // (vc, vi, vj) are pairwise disjoint and only one of the
            // three potentially has an empty mustuse set, and their
            // mustuses are all disjoint.
            bitset_t carrier = vi.carrier() | vj.carrier() | vc.carrier();
	    HexAssert(!carrier.test(vc.x()));
	    HexAssert(!carrier.test(vc.y()));
            carrier.set(vc.x());
            carrier.set(vc.y());

            // add a new full-connection between endpoints and
            // used stones
            for (BitsetIterator p(mu[0] | mu[1] | mu[2]); p; ++p)
            {
                for (size_t k=0; k<keys.size(); ++k) 
                {
                    bitset_t our_carrier = carrier;
                    our_carrier.reset(keys[k]);

                    AddNewFull(VC(keys[k], *p, our_carrier, 
                                  EMPTY_BITSET, VC_RULE_PUSH));
                }
            }

            // find all valid endpoints for the new semi-connections
            std::set<HexPointPair> ends;
            for (int a=0; a<2; ++a) {
                for (int b=a+1; b<3; ++b) {
                    for (BitsetIterator p1(mu[a]); p1; ++p1) {
                        for (BitsetIterator p2(mu[b]); p2; ++p2) {
			    HexAssert(*p1 != *p2);

                            // if using miai, must use the miai edge
                            if (has_miai 
                                && *p1 != miai_edge
                                && *p2 != miai_edge)
                                continue;

                            ends.insert(std::make_pair(std::min(*p1, *p2),
                                                       std::max(*p1, *p2)));

                        }                        
                    }                    
                }
            }

            for (std::size_t k=0; k<keys.size(); ++k) 
            {
                HexPoint key = keys[k];

                // if we have a miai, the only valid key is the miai endpoint
                if (has_miai && key != miai_endpoint)
                    continue;

                // add semi to all unique pairs found
                std::set<HexPointPair>::iterator it;
                for (it = ends.begin(); it != ends.end(); ++it) 
                {
                    HexPoint p1 = it->first;
                    HexPoint p2 = it->second;

                    bitset_t empty; // no mustuse when generated from push rule
                    VC new_semi(p1, p2, key, carrier, empty, VC_RULE_PUSH);

                    m_statistics.push_attempts++;
                    if (AddNewSemi(new_semi))
                    {
                        m_statistics.push_successes++;
#if 0
                        hex::log << hex::info << "PushRule: "
                                 << new_semi << "\n"
                                 << vc << "\n"
                                 << vi << "\n"
                                 << vj << hex::endl;
#endif
                    }
                }
            }
        }
    }
}

/** Tries to add a new full-connection to list between (vc.x(), vc.y()).

    If vc is successfully added, then:

    1) Removes any semi-connections between (vc.x(), vc.y()) that are
    supersets of vc.
    
    2) Adds (vc.x(), vc.y()) to the queue if vc was added inside the
    softlimit.
*/
bool ConnectionBuilder::AddNewFull(const VC& vc)
{
    HexAssert(vc.type() == VC::FULL);
    VCList::AddResult result = m_con->Add(vc, m_log);
    if (result != VCList::ADD_FAILED) 
    {
        // a semi that is a superset of a full is useless, so remove
        // any that exist.
        m_con->GetList(VC::SEMI, vc.x(), vc.y())
            .removeSuperSetsOf(vc.carrier(), m_log);
                
        // add this list to the queue if inside the soft-limit
        if (result == VCList::ADDED_INSIDE_SOFT_LIMIT)
            m_queue.push(std::make_pair(vc.x(), vc.y()));

        return true;
    }
    return false;
}

/** Tries to add a new semi-connection to list between (vc.x(), vc.y()). 
        
    Does not add if vc is a superset of some full-connection between
    (vc.x(), and vc.y()).
    
    If vc is successfully added and intersection on semi-list is
    empty, then:

    1) if vc added inside soft limit, adds (vc.x(), vc.y()) to queue.
    
    2) otherwise, if no full exists between (vc.x(), vc.y()), adds the
    or over the entire semi list.
*/
bool ConnectionBuilder::AddNewSemi(const VC& vc)
{
    VCList* out_full = &m_con->GetList(VC::FULL, vc.x(), vc.y());
    VCList* out_semi = &m_con->GetList(VC::SEMI, vc.x(), vc.y());
    
    if (!out_full->isSupersetOfAny(vc.carrier())) 
    {
        VCList::AddResult result = out_semi->add(vc, m_log);
        if (result != VCList::ADD_FAILED) 
        {
            if (out_semi->hardIntersection().none())
            {
                if (result == VCList::ADDED_INSIDE_SOFT_LIMIT) 
                {
                    m_queue.push(std::make_pair(vc.x(), vc.y()));
                } 
                else if (out_full->empty())
                {
                    bitset_t carrier = m_param.use_greedy_union 
                        ? out_semi->getGreedyUnion() 
                        : out_semi->getUnion();
                    
                    VC v(out_full->getX(), out_full->getY(), 
                         carrier, EMPTY_BITSET, VC_RULE_ALL);

                    out_full->add(v, m_log);
                }
            }
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------

std::string ConnectionBuilderStatistics::toString() const
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

//----------------------------------------------------------------------------

/** @page workqueue ConnectionBuilder Work Queue

    WorkQueue stores the endpoints of any VCLists that need further
    processing. Endpoints are pushed onto the back of the queue and
    popped off the front, in FIFO order. It also ensures only unique
    elements are added; that is, a list is added only once until it is
    processed.

    The implementation here is a simple vector with an index
    simulating the front of the queue; that is, push() uses
    push_back() to add elements to the back and pop() increments the
    index of the front. This means the vector will need to be as large
    as the number of calls to push(), not the maximum number of
    elements in the queue at any given time.
    
    On 11x11, the vector quickly grows to hold 2^14 elements if anding
    over the edge, and 2^13 if not. Since only unique elements are
    added, in the worst case this value will be the smallest n such
    that 2^n > xy, where x and y are the width and height of the
    board.

    This implementation was chosen for efficiency: a std::deque uses
    dynamic memory, and so every push()/pop() requires at least one
    call to malloc/free. The effect is small, but can be as
    significant as 1-3% of the total run-time, especially on smaller
    boards.
*/
ConnectionBuilder::WorkQueue::WorkQueue()
    : m_head(0), 
      m_array(128)
{
}

bool ConnectionBuilder::WorkQueue::empty() const
{
    return m_head == m_array.size();
}

const HexPointPair& ConnectionBuilder::WorkQueue::front() const
{
    return m_array[m_head];
}

std::size_t ConnectionBuilder::WorkQueue::capacity() const
{
    return m_array.capacity();
}

void ConnectionBuilder::WorkQueue::clear()
{
    memset(m_seen, 0, sizeof(m_seen));
    m_array.clear();
    m_head = 0;
}

void ConnectionBuilder::WorkQueue::pop()
{
    m_seen[front().first][front().second] = false;
    m_head++;
}

void ConnectionBuilder::WorkQueue::push(const HexPointPair& p)
{
    HexPoint a = std::min(p.first, p.second);
    HexPoint b = std::max(p.first, p.second);
    if (!m_seen[a][b]) 
    {
        m_seen[a][a] = true;
        m_array.push_back(std::make_pair(a, b));
    }
}

//----------------------------------------------------------------------------
