//----------------------------------------------------------------------------
// $Id: Connections.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "Hex.hpp"
#include "Time.hpp"
#include "BitsetIterator.hpp"
#include "GraphUtils.hpp"
#include "Connections.hpp"
#include "VCUtils.hpp"

//----------------------------------------------------------------------------

/** Show debuging output. */
#define SHOW_OUTPUT  0

//----------------------------------------------------------------------------

Connections::Connections(GroupBoard& brd, HexColor color, 
                         const VCPatternSet& patterns,
                         ChangeLog<VC>& log)
    : m_brd(brd), m_log(log), m_patterns(patterns), m_color(color)
{
    init();
}

void Connections::init()
{
    hex::log << hex::fine << "--- Connections (" << m_color << ")" << hex::endl;
    hex::log << "sizeof(VC) = " << sizeof(VC) << hex::endl;
    hex::log << "sizeof(Connections) = " << sizeof(Connections) << hex::endl;
    hex::log << "Allocating lists..." << hex::endl;

    int full_soft = hex::settings.get_int("vc-full-soft-limit");
    int semi_soft = hex::settings.get_int("vc-semi-soft-limit");
    
    // create a list for each valid pair; also create lists
    // for pairs (x,x) for ease of use later on.  These lists
    // between the same point will also be empty.
    for (BoardIterator y(m_brd.const_locations()); y; ++y) {
        for (BoardIterator x(m_brd.const_locations()); x; ++x) {
            m_vc[VC::FULL][*x][*y] = 
            m_vc[VC::FULL][*y][*x] = 
                new VCList(*y, *x, full_soft, m_log);

            m_vc[VC::SEMI][*x][*y] = 
            m_vc[VC::SEMI][*y][*x] = 
                new VCList(*y, *x, semi_soft, m_log);

            if (*x == *y) break;
        }
    }
}

Connections::~Connections()
{
    for (BoardIterator y = m_brd.const_locations(); y; ++y) {
        for (BoardIterator x = m_brd.const_locations(); x; ++x) {
            for (int i=0; i<VC::NUM_TYPES; ++i) {
                delete m_vc[i][*x][*y];
            }
            if (*x == *y) break;
        }
    }
}

//----------------------------------------------------------------------------

bool Connections::doesVCExist(HexPoint x, HexPoint y, VC::Type type) const
{
    HexAssert(VCTypeUtil::IsValidType(type));

    HexColor other = !m_color;
    if (m_brd.getColor(x) == other || m_brd.getColor(y) == other)
        return false;

    return (m_vc[type][m_brd.getCaptain(x)][m_brd.getCaptain(y)]->size() > 0);
}

bool Connections::getSmallestVC(HexPoint x, HexPoint y, 
                                VC::Type type, VC& out) const
{
    HexAssert(VCTypeUtil::IsValidType(type));
    if (!doesVCExist(x, y, type)) return false;
    out = *m_vc[type][m_brd.getCaptain(x)][m_brd.getCaptain(y)]->begin();
    return true;
}

void Connections::getVCs(HexPoint x, HexPoint y, VC::Type type,
                         std::vector<VC>& out) const
{
    HexAssert(VCTypeUtil::IsValidType(type));

    out.clear();
    HexColor other = !m_color;
    if (m_brd.getColor(x) == other || m_brd.getColor(y) == other)
        return;

    const VCList* who = m_vc[type][m_brd.getCaptain(x)][m_brd.getCaptain(y)];

    VCList::const_iterator it;
    for (it = who->begin();  it != who->end(); ++it) {
        out.push_back(*it);
    }
}

const VCList& Connections::getVCList(HexPoint x, HexPoint y, 
                                     VC::Type type) const
{
    HexAssert(VCTypeUtil::IsValidType(type));
    /** @bug Use getCaptain() here? */
    return *m_vc[type][x][y];
}

bitset_t Connections::ConnectedTo(HexPoint x, VC::Type type) const
{
    HexAssert(VCTypeUtil::IsValidType(type));

    bitset_t ret;
    for (BoardIterator y = m_brd.const_locations(); y; ++y) {
        if (doesVCExist(x, *y, type))
            ret.set(*y);
    }
    return ret;
}

std::string Connections::dump(VC::Type type) const
{
    std::ostringstream os;
    HexColorSet not_other = HexColorSetUtil::ColorOrEmpty(m_color);
    for (BoardIterator x(m_brd.Groups(not_other)); x; ++x) 
    {
        for (BoardIterator y(m_brd.Groups(not_other)); *y != *x; ++y) 
        {
            os << *x << " and " << *y << std::endl;
            VCList* lt = m_vc[type][*x][*y];
            os << lt->dump() << std::endl;
        }
    }
    return os.str();
}

void Connections::size(int& fulls, int& semis) const
{
    fulls = semis = 0;
    HexColorSet not_other = HexColorSetUtil::ColorOrEmpty(m_color);
    for (BoardIterator x(m_brd.Groups(not_other)); x; ++x) {
        for (BoardIterator y(m_brd.Groups(not_other)); *y != *x; ++y) {
            fulls += m_vc[VC::FULL][*x][*y]->size();
            semis += m_vc[VC::SEMI][*x][*y]->size();
        }
    }
}

//----------------------------------------------------------------------------

void Connections::clear()
{
    for (BoardIterator y = m_brd.const_locations(); y; ++y) {
        for (BoardIterator x = m_brd.const_locations(); *x != *y; ++x) {
            for (int i=0; i<VC::NUM_TYPES; ++i) {
                m_vc[i][*x][*y]->clear();
            }
        }
    }
    m_log.clear();
}

void Connections::logging(bool flag)
{
    m_log.activate(flag);
}

void Connections::mark_log()
{
    VC empty;
    m_log.push(ChangeLog<VC>::MARKER, empty);
}

void Connections::revert()
{
    HexAssert(!m_log.activated());

    while (!m_log.empty()) {
        int action = m_log.topAction();
        if (action == ChangeLog<VC>::MARKER) {
            m_log.pop();
            break;
        }
        
        VC vc(m_log.topData());
        m_log.pop();

        VCList* list = m_vc[vc.type()][vc.x()][vc.y()];
        if (action == ChangeLog<VC>::ADD) {
            bool res = list->remove(vc);
            HexAssert(res);
        } else if (action == ChangeLog<VC>::REMOVE) {
            list->simple_add(vc);
        } else if (action == ChangeLog<VC>::PROCESSED) {
            VCList::iterator it = list->find(vc);
            HexAssert(it != list->end());
            HexAssert(it->processed());
            it->setProcessed(false);
        }
    }    

#if SHOW_OUTPUT
    int fulls, semis;
    size(fulls, semis);
    hex::log << hex::fine 
             << "    After revert: " 
             << fulls << " fulls, " << semis << " semis; " 
             << hex::endl;
#endif

}

//----------------------------------------------------------------------------

void Connections::clearStats()
{
    m_statistics = Statistics();
}

//----------------------------------------------------------------------------
// Static VC construction

void Connections::build(int max_ors)
{
    if (max_ors == Connections::DEFAULT)
        max_ors = hex::settings.get_int("vc-default-max-ors");

    hash_t original_hash = m_brd.Hash();
    double s = HexGetTime();

    logging(false);
    clear();

    clearStats();
    m_queue.clear();

    addBaseVCs();
    AddPatternVCs();
    do_search(max_ors);

    HexAssert(original_hash == m_brd.Hash());

    double e = HexGetTime();
    hex::log << hex::fine << "  " << (e-s) << "s to build vcs." << hex::endl;
}

void Connections::addBaseVCs()
{
    HexColorSet not_other = HexColorSetUtil::ColorOrEmpty(m_color);
    for (BoardIterator x(m_brd.Groups(not_other)); x; ++x) {
        bitset_t adj = m_brd.Nbs(*x, EMPTY);

        for (BitsetIterator y(adj); y; ++y) {
            VC vc(*x, *y);
            m_statistics.base_attempts++;
            if (m_vc[VC::FULL][*x][*y]->add(vc)) {
                m_statistics.base_successes++;
                m_queue.add(std::make_pair(vc.x(), vc.y()));
            }
        }
    }
}

void Connections::AddPatternVCs()
{
    if (!hex::settings.get_bool("vc-use-patterns"))
        return;

    for (std::size_t i=0; i<m_patterns.size(); ++i) {
        const VCPattern& pat = m_patterns[i];
        if (pat.Matches(m_color, m_brd)) {
            bitset_t carrier = pat.NotOpponent() - m_brd.getColor(m_color);
            carrier.reset(pat.Endpoint(0));
            carrier.reset(pat.Endpoint(1));
            VC vc(pat.Endpoint(0), pat.Endpoint(1), carrier, VC_RULE_BASE);

            m_statistics.pattern_attempts++;
            if (m_vc[VC::FULL][vc.x()][vc.y()]->add(vc)) {
                m_statistics.pattern_successes++;
                m_queue.add(std::make_pair(vc.x(), vc.y()));
            }
        }
    }
}

//----------------------------------------------------------------------------
// Incremental VC construction

void Connections::build(bitset_t added[BLACK_AND_WHITE], int max_ors, 
                        bool mark_the_log)
{
    HexAssert((added[BLACK] & added[WHITE]).none());
    if (max_ors == Connections::DEFAULT)
        max_ors = hex::settings.get_int("vc-default-max-ors");

    hex::log << hex::finer << "build" << hex::endl;
    hex::log << m_brd << hex::endl;
    hex::log << "black: " << m_brd.printBitset(added[BLACK]) << hex::endl;
    hex::log << "white: " << m_brd.printBitset(added[WHITE]) << hex::endl;

    hash_t original_hash = m_brd.Hash();
    double s = HexGetTime();

    clearStats();
    m_queue.clear();

    logging(true);
    if (mark_the_log) mark_log();

    {
        // take new stones off and compute group info for the original
        // state.
        m_brd.setColor(m_color, m_brd.getColor(m_color) - added[m_color]);
        m_brd.absorb();
        
        // kill connections containing stones the opponent just played
        RemoveAllContaining(added[!m_color]);
        
        // find groups adjacent to any played stone of our color.
        // add them to the affected set along with the played stones.
        bitset_t affected = added[m_color];
        for (BitsetIterator x(added[m_color]); x; ++x) {
            for (BoardIterator y(m_brd.const_nbs(*x)); y; ++y) {
                if (m_brd.getColor(*y) == m_color)
                    affected.set(m_brd.getCaptain(*y));                    
            }
        }

        // replace removed stones and update group info
        m_brd.addColor(m_color, added[m_color]);
        m_brd.absorb(added[m_color]);

        // merge and shrink in one pass
        MergeAndShrink(affected, added[m_color]);
    }

    AddPatternVCs();
    do_search(max_ors);
    logging(false);

    double e = HexGetTime();

    HexAssert(original_hash == m_brd.Hash());

#if SHOW_OUTPUT
    hex::log << hex::fine
             << "    " << m_color << ": " << m_log.size() << " entries on log." 
             << hex::endl;
#endif

    hex::log << hex::fine
             << "  " << (e-s) << "s to build vcs incrementally." 
             << hex::endl;
}

void Connections::MergeAndShrink(const bitset_t& affected,
                                 const bitset_t& added)
{
    HexColorSet not_other = HexColorSetUtil::NotColor(!m_color);
    for (BoardIterator x(m_brd.Stones(not_other)); x; ++x) {
        if (!m_brd.isCaptain(*x) && !affected.test(*x)) continue;

        for (BoardIterator y(m_brd.Stones(not_other)); *y != *x; ++y) {
            if (!m_brd.isCaptain(*y) && !affected.test(*y)) continue;

            HexPoint cx = m_brd.getCaptain(*x);
            HexPoint cy = m_brd.getCaptain(*y);

            // lists between (cx,cx) are never used, so only do work
            // if it's worthwhile. This can occur if y was recently
            // played next to group x, now they both have the same
            // captain, so no point merging old connections into
            // (captain, captain).
            if (cx != cy) {

                // add (captain, captain) lists to queue exactly once
                if (cx == *x && cy == *y)
                    m_queue.add(std::make_pair(cx, cy));

                MergeAndShrink(added, *x, *y, cx, cy);

            }
        }
    }
}

void Connections::MergeAndShrink(const bitset_t& added, 
                                 HexPoint xin, HexPoint yin,
                                 HexPoint xout, HexPoint yout)
{
    HexAssert(xin != yin);
    HexAssert(xout != yout);

    VCList* fulls_in = m_vc[VC::FULL][xin][yin];
    VCList* semis_in = m_vc[VC::SEMI][xin][yin];
    VCList* fulls_out= m_vc[VC::FULL][xout][yout];
    VCList* semis_out= m_vc[VC::SEMI][xout][yout];

    HexAssert((fulls_in == fulls_out) == (semis_in == semis_out));
    bool doing_merge = (fulls_in != fulls_out);

    std::list<VC> removed;
    std::list<VC>::iterator it;

    // 
    // Shrink all 0-connections.
    //
    // If (doing_merge) transfer remaining connections over as well. 
    //
    fulls_in->removeAllContaining(added, removed);
    if (doing_merge) { 
        // Copied vc's will be set to unprocessed explicitly.
        fulls_out->add(*fulls_in);
    }

    for (it = removed.begin(); it != removed.end(); ++it) {
        VC v = VC::ShrinkFull(*it, added, xout, yout);
        if (fulls_out->add(v)) {
            m_statistics.shrunk0++;
        }
    }

    //
    // shrink all 1-connections.
    // if (doing_merge) transfer remaining connections
    // over as well. 
    //
    removed.clear();
    semis_in->removeAllContaining(added, removed);
    if (doing_merge) { 
        // Copied vc's will be set to unprocessed explicitly.
        /** @bug Some supersets of vcs could be added here. */
        semis_out->add(*semis_in);
    }

    // shrink connections that touch played cells.
    // do not upgrade during this step. 
    for (it = removed.begin(); it != removed.end(); ++it) {
        if (!added.test(it->key())) {
            VC v = VC::ShrinkSemi(*it, added, xout, yout);
            if (semis_out->add(v)) {
                m_statistics.shrunk1++;
            }
        }
    }

    // upgrade semis.  Need to do this after shrinking to ensure
    // that we remove all sc supersets from semis_out.
    for (it = removed.begin(); it != removed.end(); ++it) {
        if (added.test(it->key())) {
            VC v = VC::UpgradeSemi(*it, added, xout, yout);
            if (fulls_out->add(v)) {
                m_statistics.upgraded++;

                // remove supersets from the semi-list; do not
                // invalidate list intersection since this semi was a
                // member of the list.
                //
                // Actually, this doesn't really matter, since the
                // call to removeAllContaining() already clobbered the
                // intersections.
                semis_out->removeSuperSetsOf(v, false);
            }
        }
    }
}

//----------------------------------------------------------------------------
// VC Construction methods
//----------------------------------------------------------------------------

/** This is used in process_semis it's too slow to access hex::settings
    because process_semis is called so much. The value is grabbed from
    hex::settings in do_search(). */
bool g_use_greedy_union = true;

/** Used in process_semi. */
bool g_use_push_rule = false;

/** This is used in andClosure(); but it's too slow to get its value
    from hex::settings each time that method is called, so we do it
    once in do_search(). */
bool g_and_over_edge = false;

void Connections::process_semis(HexPoint xc, HexPoint yc, int max_ors)
{
    VCList* semis = m_vc[VC::SEMI][xc][yc];
    VCList* fulls = m_vc[VC::FULL][xc][yc];

    // if intersection on list is empty, we can't do anything so just
    // abort.
    if (semis->hardIntersection().any()) {
        return;
    }

    int soft = semis->softlimit();
    VCList::iterator cur = semis->begin();
    VCList::iterator end = semis->end();
    std::list<VC> added;

    for (int count=0; count<soft && cur!=end; ++cur, ++count) {
        
        if (!cur->processed()) {
        
            if (g_use_push_rule)
                doPushRule(*cur, semis);

            m_statistics.doOrs++;
            if (doOr(*cur, semis, fulls, added, max_ors)) {
                m_statistics.goodOrs++;
            }

            cur->setProcessed(true);
            m_log.push(ChangeLog<VC>::PROCESSED, *cur);
        }
    }

    // remove supersets of added vcs. do not invalidate list
    // intersection since v is a superset of elements of semi_list.
    for (cur = added.begin(); cur != added.end(); ++cur) 
        semis->removeSuperSetsOf(*cur, false);

    // if no full exists, create one by unioning the entire list
    if (fulls->empty()) {

        // use greedy union to make the smallest union possible.
        bitset_t carrier = g_use_greedy_union 
            ? semis->getGreedyUnion() 
            : semis->getUnion();

        bitset_t stones;
        VC v(xc, yc, carrier, stones, VC_RULE_ALL);
        fulls->add(v);
        
        // @note no need to remove supersets of v from the semi
        // list since there can be none!

//         if (fulls->add(v))
//             semis->removeSuperSetsOf(v);

    } 
}

void Connections::process_fulls(HexPoint xc, HexPoint yc)
{
    VCList* fulls = m_vc[VC::FULL][xc][yc];
    int soft = fulls->softlimit();
    VCList::iterator cur = fulls->begin();
    VCList::iterator end = fulls->end();
    for (int count=0; count<soft && cur!=end; ++cur, ++count) {
        if (!cur->processed()) {
            andClosure(*cur);
            cur->setProcessed(true);
            m_log.push(ChangeLog<VC>::PROCESSED, *cur);
        }
    }

}

void Connections::do_search(int max_ors)
{

#if SHOW_OUTPUT
    double s = HexGetTime();
#endif

    bool abort_on_win = hex::settings.get_bool("vc-abort-on-win");
    g_use_greedy_union = hex::settings.get_bool("vc-use-greedy-union");
    g_and_over_edge = hex::settings.get_bool("vc-and-over-edge");
    g_use_push_rule = hex::settings.get_bool("vc-use-push-rule");

#ifndef KEEP_NO_SUPERSETS
    /** @note The push rule can be used only if we are keeping no
        semi-supersets around. */
    HexAssert(!g_use_push_rule);
#endif

    bool winning_connection = false;
    while (!m_queue.empty()) {

        HexPointPair pair = m_queue.front();
        m_queue.pop();

        process_semis(pair.first, pair.second, max_ors);
        process_fulls(pair.first, pair.second);
        
        if (abort_on_win && 
            doesVCExist(HexPointUtil::colorEdge1(m_color),
                        HexPointUtil::colorEdge2(m_color),
                        VC::FULL))
        {
            winning_connection = true;
            break;
        }
    }        
    HexAssert(winning_connection || m_queue.empty());

    if (winning_connection) {
        hex::log << hex::fine 
                 << "Aborted on winning connection." 
                 << hex::endl;
    }
            
    // 
    // Do a processing of the side-to-side semi list to ensure
    // we have a full if mustplay is empty.  
    //
    HexPoint xc = m_brd.getCaptain(HexPointUtil::colorEdge1(m_color));
    HexPoint yc = m_brd.getCaptain(HexPointUtil::colorEdge2(m_color));
    process_semis(xc, yc, max_ors);

#if SHOW_OUTPUT
    double e = HexGetTime();

    int fulls, semis;
    size(fulls, semis);
    hex::log << hex::fine 
             << "    " << (e-s) << "s to do search; "
             << fulls << " fulls, " << semis << " semis; " 
             << hex::endl
             << "    " << m_statistics.toString() 
             << hex::endl;
#endif

    // run diagnostics if required
    if (hex::settings.get_bool("vc-verify-integrity")) {
        VerifyIntegrity();
    }
}

//----------------------------------------------------------------------------

void Connections::andClosure(const VC& vc)
{
    HexColor other = !m_color;
    HexColorSet not_other = HexColorSetUtil::NotColor(other);

    HexPoint endp[2];
    endp[0] = m_brd.getCaptain(vc.x());
    endp[1] = m_brd.getCaptain(vc.y());
    HexColor endc[2];
    endc[0] = m_brd.getColor(endp[0]);
    endc[1] = m_brd.getColor(endp[1]);

    HexAssert(endc[0] != other);
    HexAssert(endc[1] != other);

    for (BoardIterator z(m_brd.Groups(not_other)); z; ++z) {
        if (*z == endp[0] || *z == endp[1]) continue;
        if (vc.carrier().test(*z)) continue;

        // And over vc's endpoints.
        // NOTE: and over edges only if the setting is turned on.
        for (int i=0; i<2; i++) {
            int j = (i+1)&1;
            if (g_and_over_edge || !HexPointUtil::isEdge(endp[i])) {

                VCList* fulls = m_vc[VC::FULL][*z][endp[i]];
                if ((fulls->softIntersection() & vc.carrier()).any())
                    continue;
                
                AndRule rule = (endc[i] == EMPTY) ? CREATE_SEMI : CREATE_FULL;

                doAnd(*z, endp[i], endp[j], rule, vc, 
                      m_vc[VC::FULL][*z][endp[i]]);
            }
        }
    }
}

// the old list is between from and over. 
int Connections::doAnd(HexPoint from, HexPoint over, HexPoint to,
                       AndRule rule, const VC& vc, const VCList* old)
{
    // abort early if this list is empty
    if (old->empty())
        return 0;

    int count = 0;
    int soft = old->softlimit();
    VCList::const_iterator i = old->begin();
    VCList::const_iterator end = old->end();

    bitset_t stones;
    stones.set(m_brd.getCaptain(over));

    for (int count=0; count<soft && i!=end; ++i, ++count) {

        if (!i->processed())
            continue;
        
        if (i->carrier().test(to))
            continue;

        if ((i->carrier() & vc.carrier()).any())
            continue;

        VC nvc;
        switch(rule) {

        /** Create the full and if it was successfully added to the
            list of fulls between (x,y), add (x,y) to the queue for
            processing. */
        case CREATE_FULL:
            nvc = VC::AndVCs(from, to, *i, vc, stones);
            m_statistics.and_full_attempts++;
            if (AddNewFull(nvc))
            {
                count++;
                m_statistics.and_full_successes++;
            }
            break;
        
        /** Newly created semis between (x,y) that are a superset of
            an existing full between (x,y) are discarded.  If the
            semi was successfully added to the list of semis, then:

                1) if intersection of semis between (x,y) is
                non-empty, do nothing.

                2) if intersection is empty:
                    
                    a) if semi was added inside the soft limit, add
                    (x,y) to the queue.

                    b) if semi was added outside the soft limit and
                    the list of fulls between (x,y) is empty, add the
                    full created by taking the union over all semis
                    between (x,y).
        */
        case CREATE_SEMI:
            nvc = VC::AndVCs(from, to, *i, vc, over);
            m_statistics.and_semi_attempts++;

            if (AddNewSemi(nvc)) 
            {
                count++;
                m_statistics.and_semi_successes++;
            }
            break;
            
        default:
            HexAssert(false);
        }
    }
    return count;
}

int Connections::doOr(const VC& vc, 
                      const VCList* semi_list, 
                      VCList* full_list, 
                      std::list<VC>& added, 
                      int max_ors)
{
    if (semi_list->empty())
        return 0;
    
    int count = 0;
    int index[16];
    bitset_t ors[16];
    bitset_t ands[16];
    bitset_t stones[16];

    HexAssert(max_ors < 16);

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
        return 0;

    // compute intersection of [i, size()] for each i
    std::vector<bitset_t> tail(semi.size());
    tail.back() = semi.back().carrier();
    for (int i=semi.size()-2; i>=0; --i) {
        tail[i] = semi[i].carrier() & tail[i+1];
    }

    ors[0] = vc.carrier();
    ands[0] = vc.carrier();
    //stones[0] = vc.stones();  // see note below. 
    index[1] = 0;
    
    max_ors--;
    int d = 1;
    int N = semi.size();
    while (true) {
        
        int i = index[d];

        // the current intersection (some subset from [0, i-1]) is not
        // disjoint with the intersection of [i, N), so stop.
        if ((i < N) && (ands[d-1] & tail[i]).any()) {
            i = N;
        }

        if (i == N) {
            if (d == 1) 
                break;
            
            ++index[--d];
            continue;
        }
        
        ands[d] = ands[d-1] & semi[i].carrier();
        ors[d]  =  ors[d-1] | semi[i].carrier();

        /** @note It seems that we cannot actually do this.  So we
            keep all stones[d] as the empty set. */
        //stones[d] = stones[d-1] & semi[i].stones();
        
        if (ands[d].none()) {  
            
            /** Create a new full since intersection is empty
                
                @note We do no use AddNewFull() because if add is
                successful, it checks for semi-supersets and adds the
                list to the queue.  Both of these checks are not
                needed here.
            */
            VC v(full_list->getX(), full_list->getY(), 
                 ors[d], stones[d], VC_RULE_OR);

            m_statistics.or_attempts++;
            if (full_list->add(v) != VCList::ADD_FAILED) {
                count++;
                m_statistics.or_successes++;
                added.push_back(v);
            }
        
            ++index[d];

        } else if (ands[d] == ands[d-1]) {
            
            // this connection does not shrink intersection so skip it

            ++index[d];

        } else {
            
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

int Connections::doPushRule(const VC& vc, const VCList* semi_list)
{
    if (m_brd.getColor(vc.x()) != EMPTY || m_brd.getColor(vc.y()) != EMPTY)
        return 0;
    
    // copy processed semis
    std::vector<VC> semi;
    int soft = semi_list->softlimit();
    VCList::const_iterator it = semi_list->begin();
    VCList::const_iterator end = semi_list->end();
    for (int count=0; count<soft && it!=end; ++count, ++it) {
        if (it->processed())
            semi.push_back(*it);
    }
    if (semi.empty()) return 0;

    // the endpoints will be the keys to any semi-connections we create
    std::vector<HexPoint> keys;
    keys.push_back(vc.x());
    keys.push_back(vc.y());
    
    // track if an SC has empty mustuse and get captains for vc's mustuse
    bitset_t mu[3];
    bool has_empty_mustuse0 = vc.stones().none();
    mu[0] = m_brd.CaptainizeBitset(vc.stones());

    int count = 0;
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
                if (VCUtils::ValidEdgeBridge(m_brd, I, k, e) 
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
	mu[1] = m_brd.CaptainizeBitset(vi.stones());
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
                    if (VCUtils::ValidEdgeBridge(m_brd, I, k, e) 
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
                    if (VCUtils::ValidEdgeBridge(m_brd, I, k, e) 
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
	    mu[2] = m_brd.CaptainizeBitset(vj.stones());
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
                                  bitset_t(), VC_RULE_PUSH));
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

                        count++;
                    }
                }
            }
        }
    }
    return count;
}

bool Connections::AddNewFull(const VC& vc)
{
    VCList::AddResult result = m_vc[VC::FULL][vc.x()][vc.y()]->add(vc);
    if (result != VCList::ADD_FAILED) 
    {
        // any semi that is a superset of a full is is useless, so
        // remove them.
        m_vc[VC::SEMI][vc.x()][vc.y()]->removeSuperSetsOf(vc);
                
        // add this list to the queue if inside the soft-limit
        if (result == VCList::ADDED_INSIDE_SOFT_LIMIT)
            m_queue.add(std::make_pair(vc.x(), vc.y()));

        return true;
    }
    return false;
}

bool Connections::AddNewSemi(const VC& vc)
{
    VCList* out_full = m_vc[VC::FULL][vc.x()][vc.y()];
    VCList* out_semi = m_vc[VC::SEMI][vc.x()][vc.y()];
    
    if (!out_full->isVCSuperSetOfAny(vc)) 
    {
        VCList::AddResult result = out_semi->add(vc);
        if (result != VCList::ADD_FAILED) 
        {
            if (out_semi->hardIntersection().none())
            {
                if (result == VCList::ADDED_INSIDE_SOFT_LIMIT) 
                {
                    m_queue.add(std::make_pair(vc.x(), vc.y()));
                } 
                else if (out_full->empty())
                {
                    bitset_t carrier = g_use_greedy_union 
                        ? out_semi->getGreedyUnion() 
                        : out_semi->getUnion();
                    
                    bitset_t stones;
                    VC v(out_full->getX(), out_full->getY(), 
                         carrier, stones, VC_RULE_ALL);

                    out_full->add(v);
                }
            }
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------

void Connections::RemoveAllContaining(const bitset_t& added)
{
    HexColorSet not_other = HexColorSetUtil::NotColor(!m_color);
    for (BoardIterator x(m_brd.Groups(not_other)); x; ++x) {
        for (BoardIterator y(m_brd.Groups(not_other)); *y != *x; ++y) {
            VCList* fulls = m_vc[VC::FULL][*x][*y];
            VCList* semis = m_vc[VC::SEMI][*x][*y];
            int cur0 = fulls->removeAllContaining(added);
            int cur1 = semis->removeAllContaining(added);

            m_statistics.killed0 += cur0; 
            m_statistics.killed1 += cur1;

            // process this list since we could have bumped some guys
            // under the soft limit. 
            if (cur0 || cur1) {
                m_queue.add(std::make_pair(*x, *y));
            }
        }
    }
}

//----------------------------------------------------------------------------

void Connections::VerifyIntegrity() const
{
    hex::log << hex::fine << "VerifyIntegrity(" << m_color << ")" << hex::endl;
    HexColorSet not_other = HexColorSetUtil::NotColor(!m_color);
    for (BoardIterator x(m_brd.Groups(not_other)); x; ++x) {
        for (BoardIterator y(m_brd.Groups(not_other)); *y != *x; ++y) {
            for (int type = VC::FULL; type != VC::NUM_TYPES; ++type) {
                const VCList* lst = m_vc[type][*x][*y];
                if ((lst->getUnion() & m_brd.getOccupied()).any()) {
                    hex::log << hex::warning;
                    hex::log << VCTypeUtil::toString((VC::Type)type) 
                             << "-connections hit stones!" << hex::endl;
                    hex::log << m_brd << hex::endl;
                    hex::log << m_brd.printBitset(lst->getUnion() & 
                                                  m_brd.getOccupied());
                    hex::log << hex::endl << lst->dump() << hex::endl;
                    HexAssert(false);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------

#define ADD_ONLY_UNIQUE 0

bool Connections::VCQueue::empty() const
{
    return m_queue.empty();
}

const HexPointPair& Connections::VCQueue::front() const
{
    return m_queue.front();
}

void Connections::VCQueue::clear()
{
#if ADD_ONLY_UNIQUE
    memset(m_seen, 0, sizeof(m_seen));
#endif

    m_queue = std::queue<HexPointPair>();
}

void Connections::VCQueue::pop()
{
    HexPointPair p = m_queue.front();
    m_queue.pop();

#if ADD_ONLY_UNIQUE
    m_seen[p.first][p.second] = false;
#endif

}

void Connections::VCQueue::add(const HexPointPair& p)
{
#if ADD_ONLY_UNIQUE

    if (!m_seen[p.first][p.second]) {
        m_seen[p.first][p.second] = true;
        m_queue.push(p);
    }

#else

    // try to not process the same list multiple times in a row
    if (m_queue.empty() || m_queue.back() != p)
        m_queue.push(p);

#endif
}

//----------------------------------------------------------------------------

bool Connections::operator==(const Connections& other) const
{
    HexColorSet not_other = HexColorSetUtil::NotColor(!m_color);
    for (BoardIterator x(m_brd.Groups(not_other)); x; ++x) {
        for (BoardIterator y(m_brd.Groups(not_other)); *y != *x; ++y) {
            const VCList& full1 = *m_vc[VC::FULL][*x][*y];
            const VCList& full2 = *other.m_vc[VC::FULL][*x][*y];
            if (full1 != full2) return false;

            const VCList& semi1 = *m_vc[VC::SEMI][*x][*y];
            const VCList& semi2 = *other.m_vc[VC::SEMI][*x][*y];
            if (semi1 != semi2) return false;
        }
    }
    return true;
}
 
bool Connections::operator!=(const Connections& other) const
{
    return !operator==(other);
}

//----------------------------------------------------------------------------
