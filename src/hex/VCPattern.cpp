//----------------------------------------------------------------------------
// $Id: VCPattern.cpp 1666 2008-09-16 20:38:55Z broderic $
//----------------------------------------------------------------------------

#include "BitsetIterator.hpp"
#include "VCPattern.hpp"
#include "StoneBoard.hpp"

//----------------------------------------------------------------------------

/** Tools for constructing patterns. */
namespace
{

/** The start/end of a ladder pattern. */
struct BuilderPattern
{
    bitset_t black, empty, badprobes;
    HexPoint endpoint;
    int height;
    
    BuilderPattern(const bitset_t& b, const bitset_t& em,
                   const bitset_t& p, HexPoint e, int h)
        : black(b), empty(em), badprobes(p), endpoint(e), height(h)
    { };
};

/** Shifts a BuilderPattern in the given direction. Returns true
    if pattern remains inside the board, false otherwise. */
bool ShiftBuilderPattern(BuilderPattern& pat, HexDirection dir,
                         const StoneBoard& brd)
{
    bitset_t black, empty, bad;
    HexPoint endpoint = brd.PointInDir(pat.endpoint, dir);
   
    if (!brd.ShiftBitset(pat.black, dir, black)) return false;
    if (!brd.ShiftBitset(pat.empty, dir, empty)) return false;
    if (!brd.ShiftBitset(pat.badprobes, dir, bad)) return false;
    
    pat = BuilderPattern(black, empty, bad, endpoint, pat.height);
    return true;
}

//---------------------------------------------------------------------------

/** Rotates pattern on given board. */
VCPattern RotatePattern(const VCPattern& pat, const StoneBoard& brd)
{
    HexPoint endpoint1 = brd.rotate(pat.Endpoint(0));
    HexPoint endpoint2 = brd.rotate(pat.Endpoint(1));
    bitset_t must = brd.rotate(pat.MustHave());
    bitset_t oppt = brd.rotate(pat.NotOpponent());
    bitset_t badp = brd.rotate(pat.BadProbes());
    return VCPattern(endpoint1, endpoint2, must, oppt, badp);
}

/** Mirrors pattern on given board. */
VCPattern MirrorPattern(const VCPattern& pat, const StoneBoard& brd)
{
    HexPoint endpoint1 = brd.mirror(pat.Endpoint(0));
    HexPoint endpoint2 = brd.mirror(pat.Endpoint(1));
    bitset_t must = brd.mirror(pat.MustHave());
    bitset_t oppt = brd.mirror(pat.NotOpponent());
    bitset_t badp = brd.mirror(pat.BadProbes());
    return VCPattern(endpoint1, endpoint2, must, oppt, badp);
}

/** Applies the reverse-mapping; used to reverse the direction of
    ladder vcs. Returns INVALID_POINT if this point would be reversed
    off the board. */
HexPoint ReversePoint(HexPoint point, const StoneBoard& brd)
{
    if (HexPointUtil::isEdge(point)) 
        return point;

    int x, y;
    HexPointUtil::pointToCoords(point, x, y);
    x = (brd.width() - 1 - x) + (brd.height() - 1 - y);
    if (x >= brd.width()) 
        return INVALID_POINT;

    return HexPointUtil::coordsToPoint(x, y);
}

/** Reverses bitset using ReversePoint(). Returns true if all points
    were reversed successfully, false otherwise. */
bool ReverseBitset(const bitset_t& bs, const StoneBoard& brd, bitset_t& out)
{
    out.reset();
    for (BitsetIterator p(bs); p; ++p) {
        HexPoint rp = ReversePoint(*p, brd);
        if (rp == INVALID_POINT) return false;
        out.set(rp);
    }
    return true;
}

/** Reverses a pattern situated in the bottom left corner. */
bool ReversePattern(VCPattern& pat, const StoneBoard& brd)
{
    do {
        bitset_t must, oppt, badp;
        if (ReverseBitset(pat.MustHave(), brd, must) 
            && ReverseBitset(pat.NotOpponent(), brd, oppt)
            && ReverseBitset(pat.BadProbes(), brd, badp))
        {
            HexPoint endpoint1 = ReversePoint(pat.Endpoint(0), brd);
            HexPoint endpoint2 = ReversePoint(pat.Endpoint(1), brd);
            pat = VCPattern(endpoint1, endpoint2, must, oppt, badp);
            return true;
        }
    } while (pat.ShiftPattern(DIR_EAST, brd));

    return false;
}

//---------------------------------------------------------------------------

/** Shifts pattern in given direction until it goes off the board. */
void ShiftAndAdd(const VCPattern& pat, HexDirection dir, 
                 StoneBoard& brd, std::vector<VCPattern>& out)
{
    VCPattern spat(pat);
    do {
        out.push_back(spat);
    } while (spat.ShiftPattern(dir, brd));
}

/** Shifts pattern in first direction, rotates, then shifts in second
    direction. */
void RotateAndShift(const VCPattern& pat, StoneBoard& brd,
                    HexDirection d1, HexDirection d2, 
                    std::vector<VCPattern>& out)
{
    ShiftAndAdd(pat, d1, brd, out);
    ShiftAndAdd(RotatePattern(pat, brd), d2, brd, out);
}

/** Calls RotateAndShift() on the original and mirrored versions of
    pattern, then again on the reversed and mirrored-reversed versions
    of the pattern. */
void ProcessPattern(const VCPattern& pat, StoneBoard& brd,
                    std::vector<VCPattern> out[BLACK_AND_WHITE])
{
    RotateAndShift(pat, brd, DIR_EAST, DIR_WEST, out[BLACK]);
    RotateAndShift(MirrorPattern(pat, brd), brd, DIR_SOUTH, 
                   DIR_NORTH, out[WHITE]);

    VCPattern rpat(pat);
    ReversePattern(rpat, brd);
    RotateAndShift(rpat, brd, DIR_WEST, DIR_EAST, out[BLACK]);
    RotateAndShift(MirrorPattern(rpat, brd), brd, DIR_NORTH, 
                   DIR_SOUTH, out[WHITE]);
}

} // annonymous namespace

//----------------------------------------------------------------------------

VCPattern::VCPatternSetMap& VCPattern::GetConstructed(HexColor color)
{
    static GlobalData data;
    return data.constructed[color];
}

const VCPatternSet& 
VCPattern::GetPatterns(int width, int height, HexColor color)
{
    /** @bug The "vc-pattern-file" option is not checked
        here. This means that creating a VCPatternSet for board size
        (x,y), changing "vc-pattern-file", and then asking for a
        pattern set for (x,y) will not cause a re-build.  No one will
        want to do this anyway (right?), so it doesn't matter.  If you
        really want to do this, add the filename as part of the key
        for VCPatternSetMap.
    */
    std::pair<int, int> key = std::make_pair(width, height);
    VCPatternSetMap::iterator it = GetConstructed(color).find(key);
    if (it == GetConstructed(color).end()) 
    {
        CreatePatterns(width, height);
    }

    return GetConstructed(color)[key];
}

void VCPattern::CreatePatterns(int width, int height)
{
    hex::log << hex::info << "VCPattern::CreatePatterns(" 
             << width << ", " << height << ")" << hex::endl;
    
    std::string file = hex::settings.get("vc-pattern-file");
    if (file[0] == '*') 
    {
        file = hex::settings.get("config-data-path") 
            + "vc-patterns/" + file.substr(1);
    }
    hex::log << hex::info << "'" << file << "'" << hex::endl;

    std::ifstream fin(file.c_str());
    if (!fin)
    {
        hex::log << hex::warning << "Could not open pattern file!" 
                 << hex::endl;
        HexAssert(false);
    }

    std::vector<VCPattern> out[BLACK_AND_WHITE];

    std::vector<BuilderPattern> start, end;
    std::vector<VCPattern> complete;

    int numConstructed = 0;
    int numComplete = 0;

    std::string line;
    while (true) {
        int patternHeight;
        std::string tmp, name, type;
        std::vector<std::string> carrier;
        
        std::istringstream is;
        if (!getline(fin, line)) break;
        if (line == "") break;
        
        is.str(line);
        is >> name;
        is >> name;
        is.clear();
        
        getline(fin, line);
        is.str(line);
        is >> type;
        is >> type;
        is.clear();

        getline(fin, line);
        is.str(line);
        is >> tmp;
        is >> patternHeight;
        is.clear();

        while (true) {
            getline(fin, line);
            if (line == "") break;
            carrier.push_back(line);
        }
        HexAssert(!carrier.empty());
        HexAssert(carrier.size() <= 11);
        HexAssert(patternHeight == -1 || patternHeight <= (int)carrier.size());

        // abort if pattern too large for board
        if ((int)carrier.size() > height) continue;
        
        int row = height-1;
        int numcol = -1;
        HexPoint endpoint = SOUTH;
        bitset_t black, empty, badprobes;
        for (int i=carrier.size()-1; i>=0; --i) {
            int col = 0; 
            is.clear();
            is.str(carrier[i]);

            std::string sym;
            while (is >> sym) {
                HexPoint p = HexPointUtil::coordsToPoint(col, row);
                switch(sym[0]) {
                case '*': empty.set(p); break;
                case 'X': badprobes.set(p); empty.set(p); break;
                case 'E': endpoint = p; empty.set(p); break;
                case 'B': black.set(p); break;
                case '.': break;
                default: HexAssert(false);
                }
                col++;
            }

            if (numcol == -1) numcol = col;
            else if (numcol != col) {
                hex::log << hex::severe 
                         << "Number of columns is not the same!"
                         << numcol << " != " << col
                         << hex::endl;
                HexAssert(false);
            }

            row--;
        }

        // abort if pattern too large for board 
        if (numcol > width) continue;

        StoneBoard sb(width, height);
        sb.startNewGame();

        if (type == "complete") {
            numComplete++;
            VCPattern pat(endpoint, SOUTH, black, empty, badprobes);
            complete.push_back(pat);
        } else if (type == "start") {
            start.push_back(BuilderPattern(black, empty, badprobes, 
                                           endpoint, patternHeight));
        } else if (type == "end") {
            end.push_back(BuilderPattern(black, empty, badprobes, 
                                           endpoint, patternHeight));
        }
    }
    fin.close();

    // build ladder patterns by combining start and end patterns
    hex::log << hex::info << "Combining start(" << start.size()
             << ") and end(" << end.size() << ")..." << hex::endl;
    StoneBoard sb(width, height);
    for (std::size_t s=0; s<start.size(); ++s) {
        const BuilderPattern& st = start[s];

        for (std::size_t e=0; e<end.size(); ++e) {
            const BuilderPattern& en = end[e];
            if (en.height < st.height) continue;

            // shift end pattern until it does not hit start pattern
            sb.startNewGame();
            BuilderPattern bp(en);
            int col=0;
            bool onBoard = true;
            while (onBoard) {
		if (((bp.empty|bp.black) & (st.empty|st.black)).none()) break;
                onBoard = ShiftBuilderPattern(bp, DIR_EAST, sb);
                col++;
            }
            
            // patterns are too big combined to fit on board 
            if (!onBoard) continue;

            int startCol = col;
            while (onBoard) {
                bitset_t empty = st.empty | bp.empty;
                bitset_t black = st.black | bp.black;
                bitset_t badprobes = st.badprobes | bp.badprobes;
                
                for (int i=startCol; i<col; ++i) {
                    for (int j=0; j<st.height; ++j) {
                        HexPoint p = HexPointUtil::coordsToPoint(i, height-1-j);
                        empty.set(p);
                    }
                }
                
                VCPattern pat(st.endpoint, SOUTH, black, empty, badprobes);    
                complete.push_back(pat);
                numConstructed++;

                onBoard = ShiftBuilderPattern(bp, DIR_EAST, sb);
                col++;
            }
        }
    }

    hex::log << hex::info << "Constructed " << numConstructed << "." 
             << hex::endl << "Parsed " << numComplete << " complete." 
             << hex::endl;

    // translate, rotate, and mirror all completed patterns
    hex::log << "Translating, rotating, mirroring..." << hex::endl;
    for (std::size_t i=0; i<complete.size(); ++i) {
        ProcessPattern(complete[i], sb, out);
    }

    hex::log << hex::info << out[BLACK].size() << " total patterns" 
             << hex::endl;

    GetConstructed(BLACK)[std::make_pair(width, height)] = out[BLACK];
    GetConstructed(WHITE)[std::make_pair(width, height)] = out[WHITE];

    hex::log << "Done." << hex::endl;
}

//----------------------------------------------------------------------------

VCPattern::VCPattern(HexPoint end1, HexPoint end2, const bitset_t& must_have, 
                     const bitset_t& not_oppt, const bitset_t& bad_probes)
    : m_must_have(must_have), m_not_oppt(not_oppt), m_bad_probes(bad_probes),
      m_end1(end1), m_end2(end2)
{
}

VCPattern::~VCPattern()
{
}

//----------------------------------------------------------------------------

bool VCPattern::Matches(HexColor color, const StoneBoard& brd) const
{
    bitset_t my_color = brd.getColor(color) & brd.getCells();
    bitset_t op_color = brd.getColor(!color) & brd.getCells();

    return ((m_not_oppt & op_color).none() 
            && BitsetUtil::IsSubsetOf(m_must_have, my_color));
}

//----------------------------------------------------------------------------

bool VCPattern::ShiftPattern(HexDirection dir, const StoneBoard& brd)
{
    bitset_t must, oppt, bad;
    if (!brd.ShiftBitset(MustHave(), dir, must)) return false;
    if (!brd.ShiftBitset(NotOpponent(), dir, oppt)) return false;
    if (!brd.ShiftBitset(BadProbes(), dir, bad)) return false;
    HexPoint endpoint1 = brd.PointInDir(Endpoint(0), dir);
    HexPoint endpoint2 = brd.PointInDir(Endpoint(1), dir);

    m_end1 = endpoint1;
    m_end2 = endpoint2;
    m_must_have = must;
    m_not_oppt = oppt;
    m_bad_probes = bad;
    return true;
}

//----------------------------------------------------------------------------
