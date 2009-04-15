//----------------------------------------------------------------------------
// $Id: GroupBoard.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "BitsetIterator.hpp"
#include "GraphUtils.hpp"
#include "GroupBoard.hpp"

//----------------------------------------------------------------------------

GroupBoard::GroupBoard( unsigned size )
    : StoneBoard(size)
{
    init();
}

GroupBoard::GroupBoard( unsigned width, unsigned height )
    : StoneBoard(width, height)
{
    init();
}

GroupBoard::GroupBoard(const StoneBoard& brd)
    : StoneBoard(brd)
{
    init();
}

void GroupBoard::init()
{
    hex::log << hex::fine << "--- GroupBoard" << hex::endl;
    hex::log << "sizeof(GroupBoard) = " << sizeof(GroupBoard) << hex::endl;
    clearAbsorb();
}

GroupBoard::~GroupBoard()
{
}

//----------------------------------------------------------------------------

void GroupBoard::modified()
{
    StoneBoard::modified();

    m_captains_computed = false;
    m_members_computed = false;
    m_nbs_computed = false;
}

//----------------------------------------------------------------------------

const BoardIterator& GroupBoard::Groups(HexColorSet colorset) const
{
    if (!m_captains_computed) {

        for (int i=0; i<NUM_COLOR_SETS; ++i) 
            m_captain_list[i].clear();
    
        for (BoardIterator p(const_locations()); p; ++p) {
            if (!isCaptain(*p)) continue;

            HexColor color = getColor(*p);
            for (int i=0; i<NUM_COLOR_SETS; ++i) {
                if (HexColorSetUtil::InSet(color, (HexColorSet)i))
                    m_captain_list[i].push_back(*p);
            }
        }

        for (int i=0; i<NUM_COLOR_SETS; ++i) {
            m_captain_list[i].push_back(INVALID_POINT);
            m_captains[i] = BoardIterator(m_captain_list[i]);
        }

        m_captains_computed = true;
    }

    return m_captains[colorset];
}

int GroupBoard::NumGroups(HexColorSet colorset) const
{
    // force computation of groups if not computed already 
    if (!m_captains_computed) {
        Groups(colorset);  
    }

    // subtract 1 because of the INVALID_POINT appended at the end
    return m_captain_list[colorset].size()-1;
}

int GroupBoard::GroupIndex(HexColorSet colorset, HexPoint group) const
{
    // force computation of groups if not computed already 
    if (!m_captains_computed) {
        Groups(colorset);  
    }

    /** find group in the list
        @todo cache this? */
    int n = m_captain_list[colorset].size()-1;
    for (int i=0; i<n; ++i) {
        if (m_captain_list[colorset][i] == group)
            return i;
    }
    HexAssert(false);
    return 0;
}

bitset_t GroupBoard::GroupMembers(HexPoint cell) const
{
    if (!m_members_computed) {
        m_members.clear();

        for (BoardIterator p(const_locations()); p; ++p)
            m_members[getCaptain(*p)].set(*p);
        
        m_members_computed = true;
    }
    return m_members[getCaptain(cell)];
}

bitset_t GroupBoard::CaptainizeBitset(bitset_t locations) const
{
    HexAssert(isLocation(locations));
    bitset_t captains;
    for (BitsetIterator i(locations); i; ++i) {
        captains.set(getCaptain(*i));
    }
    return captains;
}

//----------------------------------------------------------------------------

bitset_t GroupBoard::Nbs(HexPoint group, HexColor nb_color) const
{
    if (!m_nbs_computed) {
        
        memset(m_nbs, 0, sizeof(m_nbs));
        for (BoardIterator p(const_locations()); p; ++p) {
            HexPoint pcap = getCaptain(*p);
            HexColor pcolor = getColor(*p);
            for (BoardIterator nb(const_nbs(*p)); nb; ++nb) {
                HexPoint ncap = getCaptain(*nb);
                HexColor ncolor = getColor(*nb);
                if (ncap != pcap) {
                    m_nbs[ncolor][pcap].set(ncap);
                    m_nbs[pcolor][ncap].set(pcap);
                }
            }
        }

        m_nbs_computed = true;
    }

    return m_nbs[nb_color][getCaptain(group)];
}

bitset_t GroupBoard::Nbs(HexPoint group, HexColorSet colorset) const
{
    bitset_t ret;
    for (ColorIterator color; color; ++color) 
        if (HexColorSetUtil::InSet(*color, colorset))
            ret |= Nbs(group, *color);
    return ret;
}

bitset_t GroupBoard::Nbs(HexPoint group) const
{
    return Nbs(group, ALL_COLORS);
}

//----------------------------------------------------------------------------

void GroupBoard::ComputeDigraph(HexColor color, PointToBitset& nbs) const
{
    nbs.clear();

    // Copy adjacent nbs
    HexColorSet not_other = HexColorSetUtil::ColorOrEmpty(color);
    for (BoardIterator g(Groups(not_other)); g; ++g) {
        nbs[*g] = Nbs(*g, EMPTY);
    }
   
    // Go through groups of color
    for (BitsetIterator p(getEmpty()); p; ++p) {
        for (BoardIterator nb(const_nbs(*p)); nb; ++nb) {
            if (getColor(*nb) == color) {
                nbs[*p] |= nbs[getCaptain(*nb)];
                nbs[*p].reset(*p);  // ensure p doesn't point to p
            }
        }
    }
}

//----------------------------------------------------------------------------

void GroupBoard::clearAbsorb()
{
    m_groups.clear();
}

void GroupBoard::absorb(HexPoint cell)
{
    HexAssert(isValid(cell));
    
    HexColor color = getColor(cell);
    HexAssert(color != EMPTY);            /// @todo just return here instead?
    
    for (BoardIterator i(const_nbs(cell)); i; ++i) {
        if (getColor(*i) == color)
            m_groups.unionGroups(cell, *i);
    }
        
    HexAssert(HexPointUtil::isEdge(getCaptain(NORTH)));
    HexAssert(HexPointUtil::isEdge(getCaptain(SOUTH)));
    HexAssert(HexPointUtil::isEdge(getCaptain(EAST)));
    HexAssert(HexPointUtil::isEdge(getCaptain(WEST)));
}

void GroupBoard::absorb(const bitset_t& changed)
{
    for (BitsetIterator p(changed); p; ++p) {
        absorb(*p);
    }
}

void GroupBoard::absorb()
{
    clearAbsorb();
    for (BitsetIterator p(getBlack() | getWhite()); p; ++p) {
        absorb(*p);
    }
}

HexColor GroupBoard::getWinner() const
{
    for (BWIterator it; it; ++it) {
    if (getCaptain(HexPointUtil::colorEdge1(*it)) == 
        getCaptain(HexPointUtil::colorEdge2(*it)))
        return *it;
    }
    return EMPTY;
}

bool GroupBoard::isGameOver() const
{
    return (getWinner() != EMPTY);
}

//----------------------------------------------------------------------
// protected members
//----------------------------------------------------------------------

void GroupBoard::clear()
{
    StoneBoard::clear();
    clearAbsorb();
}

//----------------------------------------------------------------------------
