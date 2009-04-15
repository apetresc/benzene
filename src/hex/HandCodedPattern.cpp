//----------------------------------------------------------------------------
// $Id: HandCodedPattern.cpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#include "HandCodedPattern.hpp"

//----------------------------------------------------------------------------

HandCodedPattern::HandCodedPattern()
    : m_dominatee(INVALID_POINT),
      m_dominator(INVALID_POINT)
{
}

HandCodedPattern::HandCodedPattern(HexPoint dominatee, HexPoint dominator)
    : m_dominatee(dominatee),
      m_dominator(dominator)
{
}

HandCodedPattern::~HandCodedPattern()
{
}
    
//----------------------------------------------------------------------------

void HandCodedPattern::set(HexColor color, const bitset_t& bs)
{
    HexAssert(HexColorUtil::isBlackWhite(color));
    m_color[color] = bs;
}

void HandCodedPattern::rotate(const ConstBoard& brd)
{
    m_dominatee = brd.rotate(m_dominatee);
    m_dominator = brd.rotate(m_dominator);
    m_mask = brd.rotate(m_mask);
    for (BWIterator it; it; ++it)
        m_color[*it] = brd.rotate(m_color[*it]);
}

void HandCodedPattern::mirror(const ConstBoard& brd)
{
    m_dominatee = brd.mirror(m_dominatee);
    m_dominator = brd.mirror(m_dominator);
    m_mask = brd.mirror(m_mask);
    for (BWIterator it; it; ++it)
        m_color[*it] = brd.mirror(m_color[*it]);
}

void HandCodedPattern::flipColors()
{
    std::swap(m_color[BLACK], m_color[WHITE]);
}

bool HandCodedPattern::check(const StoneBoard& brd)
{
    for (BWIterator it; it; ++it) {
        if (m_color[*it] != (brd.getColor(*it) & m_mask))
            return false;
   }
    return true;
}    

//----------------------------------------------------------------------------

void HandCodedPattern::CreatePatterns(std::vector<HandCodedPattern>& out)
{
    bitset_t bs;
    HandCodedPattern pat;

    //
    //
    // B3 dominates A3:
    //
    //   A B C D
    //   ----------
    // 1 \ . . . .
    //  2 \ . . .
    //   3 \ * !
    //
    HandCodedPattern pat1(HexPointUtil::fromString("a3"),
                          HexPointUtil::fromString("b3"));
    bs.reset();
    bs.set(HexPointUtil::fromString("a1"));
    bs.set(HexPointUtil::fromString("b1"));
    bs.set(HexPointUtil::fromString("c1"));
    bs.set(HexPointUtil::fromString("d1"));
    bs.set(HexPointUtil::fromString("a2"));
    bs.set(HexPointUtil::fromString("b2"));
    bs.set(HexPointUtil::fromString("c2"));
    bs.set(HexPointUtil::fromString("a3"));
    bs.set(HexPointUtil::fromString("b3"));
    pat1.setMask(bs);
    out.push_back(pat1);

    //
    //
    // With white C1, b3 dominates b2 for black.
    //
    //   A B C
    //   ----------
    // 1 \ . . W 
    //  2 \ . * .
    //   3 \ . !
    //    4 \ .
    //
    HandCodedPattern pat2(HexPointUtil::fromString("b2"),
                          HexPointUtil::fromString("b3"));
    bs.reset();
    bs.set(HexPointUtil::fromString("a1"));
    bs.set(HexPointUtil::fromString("b1"));
    bs.set(HexPointUtil::fromString("c1"));
    bs.set(HexPointUtil::fromString("a2"));
    bs.set(HexPointUtil::fromString("b2"));
    bs.set(HexPointUtil::fromString("c2"));
    bs.set(HexPointUtil::fromString("a3"));
    bs.set(HexPointUtil::fromString("b3"));
    bs.set(HexPointUtil::fromString("a4"));
    pat2.setMask(bs);
    bs.reset();
    bs.set(HexPointUtil::fromString("c1"));
    pat2.set(WHITE, bs);
    out.push_back(pat2);

    //
    //
    // With white C2, b3 dominates b2, a3, and a4.
    //
    //   A B C
    //   ----------
    // 1 \ . . . 
    //  2 \ . * W
    //   3 \ . !
    //    4 \ .
    //
    HandCodedPattern pat3(HexPointUtil::fromString("b2"),
                          HexPointUtil::fromString("b3"));
    bs.reset();
    bs.set(HexPointUtil::fromString("a1"));
    bs.set(HexPointUtil::fromString("b1"));
    bs.set(HexPointUtil::fromString("c1"));
    bs.set(HexPointUtil::fromString("a2"));
    bs.set(HexPointUtil::fromString("b2"));
    bs.set(HexPointUtil::fromString("c2"));
    bs.set(HexPointUtil::fromString("a3"));
    bs.set(HexPointUtil::fromString("b3"));
    bs.set(HexPointUtil::fromString("a4"));
    pat3.setMask(bs);
    bs.reset();
    bs.set(HexPointUtil::fromString("c2"));
    pat3.set(WHITE, bs);

    pat3.setDominatee(HexPointUtil::fromString("b2"));
    out.push_back(pat3);

    pat3.setDominatee(HexPointUtil::fromString("a3"));
    out.push_back(pat3);

    pat3.setDominatee(HexPointUtil::fromString("a4"));
    out.push_back(pat3);
}

//----------------------------------------------------------------------------
