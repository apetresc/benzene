//----------------------------------------------------------------------------
// $Id: RingGodel.cpp 1798 2008-12-15 02:39:28Z broderic $
//----------------------------------------------------------------------------

#include "RingGodel.hpp"
#include "Pattern.hpp"

//----------------------------------------------------------------------------

RingGodel::GlobalData::GlobalData()
{
    //std::cout << "RingGodel::Initialize" << std::endl;
    
    // compute scores adjusted by slice
    for (int s=0; s<Pattern::NUM_SLICES; ++s) 
    {
        for (ColorIterator color; color; ++color) 
        {
            color_slice_score[*color].push_back
                (AdjustScoreBySlice(Score(*color), s));
        }
        mask_slice_score.push_back(AdjustScoreBySlice(SLICE_MASK, s));
    }

    // compute the empty ring godel
    empty = 0;
    for (int s=0; s<Pattern::NUM_SLICES; ++s)
        empty |= color_slice_score[EMPTY][s];
}

//----------------------------------------------------------------------------

RingGodel::RingGodel()
    : m_value(0)
{
}

RingGodel::RingGodel(int value)
    : m_value(value)
{
}

RingGodel::~RingGodel()
{
}

RingGodel::GlobalData& RingGodel::GetGlobalData()
{
    static GlobalData s_data;
    return s_data;
}

void RingGodel::AddColorToSlice(int slice, HexColor color)
{
    HexAssert(HexColorUtil::isBlackWhite(color));

    // add the color to the slice
    m_value |= GetGlobalData().color_slice_score[color][slice];

    // remove empty from the slice
    m_value &= ~GetGlobalData().color_slice_score[EMPTY][slice];
}

void RingGodel::SetSliceToColor(int slice, HexColor color)
{
    // zero the slice 
    m_value &=~GetGlobalData().mask_slice_score[slice];

    // set slice to color
    m_value |= GetGlobalData().color_slice_score[color][slice];
}

void RingGodel::SetEmpty()
{
    m_value = GetGlobalData().empty;
}

int RingGodel::Index() const
{
    return GetValidGodelData().godel_to_index[m_value];
}

//----------------------------------------------------------------------------

PatternRingGodel::PatternRingGodel()
    : RingGodel(),
      m_mask(0)
{
}

PatternRingGodel::~PatternRingGodel()
{
}

void PatternRingGodel::SetEmpty()
{
    RingGodel::SetEmpty();
    m_mask = 0;
}

void PatternRingGodel::AddSliceToMask(int slice)
{
    m_mask |= AdjustScoreBySlice(SLICE_MASK, slice);
}
    
bool PatternRingGodel::MatchesGodel(const RingGodel& other) const
{
    // check that on our mask the colors in other's slices are
    // supersets of our slices.
    int other_ring = other.Value() & m_mask;
    return (other_ring & m_value) == (m_value & m_mask);
}

//----------------------------------------------------------------------------

RingGodel::ValidGodelData& RingGodel::GetValidGodelData()
{
    static ValidGodelData s_valid_godels;
    return s_valid_godels;
}

const std::vector<RingGodel>& RingGodel::ValidGodels()
{
    return GetValidGodelData().valid_godel;
}

/** Computes the set of valid godels. This skips godels where a slice
    is empty and either black or white. Also computes the godel to
    index vector for fast lookups -- using a map is apparently too
    slow.
*/
RingGodel::ValidGodelData::ValidGodelData()
{
    //std::cout << "RingGodel::ComputeValid()" << std::endl;
    
    int num_possible_godels = 1<<(BITS_PER_SLICE*Pattern::NUM_SLICES);

    const GlobalData& data = GetGlobalData();
    for (int g=0; g<num_possible_godels; ++g) 
    {
        bool valid = true;
        for (int s=0; valid && s<Pattern::NUM_SLICES; ++s) 
        {
            if ((g & data.mask_slice_score[s]) == 0) 
            {
                valid = false;
            }
            else if (g & data.color_slice_score[EMPTY][s]) 
            {
                if ((g & data.color_slice_score[BLACK][s]) || 
                    (g & data.color_slice_score[WHITE][s]))
                {
                    valid = false;
                }
            }
        }

        if (valid) 
        {
            godel_to_index.push_back(valid_godel.size());
            valid_godel.push_back(g);
        } 
        else
        {
            godel_to_index.push_back(-1);
        }
    }

    //std::cout << "Possible godels: " << num_possible_godels << std::endl;
    //std::cout << "Valid godels: " << valid_godel.size() << std::endl;
}

//----------------------------------------------------------------------------
