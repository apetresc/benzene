//----------------------------------------------------------------------------
// $Id: HandBookCheck.cpp 1877 2009-01-29 00:57:27Z broderic $
//----------------------------------------------------------------------------

#include "HandBookCheck.hpp"
#include "BenzenePlayer.hpp"

#include <boost/filesystem/path.hpp>

//----------------------------------------------------------------------------

HandBookCheck::HandBookCheck(BenzenePlayer* player)
    : BenzenePlayerFunctionality(player),
      m_enabled(false),
      m_handBookLoaded(false)
{
}

HandBookCheck::~HandBookCheck()
{
}

HexPoint HandBookCheck::pre_search(HexBoard& brd, const Game& game_state,
				   HexColor color, bitset_t& consider,
				   double time_remaining, double& score)
{
    if (m_enabled) 
    {
	HexPoint response = HandBookResponse(brd, color);
        if (response != INVALID_POINT)
            return response;
	//StoneBoard b(brd);
	//b.rotateBoard();
	//response = HandBookResponse(b, color);
	//if (response != INVALID_POINT)
	//  return b.rotate(response);
	
    }
    return m_player->pre_search(brd, game_state, color, consider,
				time_remaining, score);
}

//----------------------------------------------------------------------------

void HandBookCheck::LoadHandBook()
{
    hex::log << hex::info << "Loading hand book" << hex::endl;
    
    // Find hand book file
    using namespace boost::filesystem;
    path p = path(ABS_TOP_SRCDIR) / "share" / "hand-book.txt";
    p.normalize();
    std::string file = p.native_file_string();
    
    // Open file if exists, else abort
    std::ifstream f;
    f.open(file.c_str());
    if (!f) {
        hex::log << hex::warning;
        hex::log << "Could not open file '" << file << "'!" << hex::endl;
        return;
    }
    
    // Extract board ID/response pairs from hand book
    std::string line;
    while (getline(f, line)) {
	std::istringstream iss;
	iss.str(line);
	std::string brdID;
	iss >> brdID;
	// Comment lines should be ignored
	if (brdID[0] != '#') {
	    m_id.push_back(brdID);
	    std::string response;
	    iss >> response;
	    HexPoint p = HexPointUtil::fromString(response);
	    HexAssert(p != INVALID_POINT);
	    m_response.push_back(p);
	}
    }
    HexAssert(m_id.size() == m_response.size());
    // note: file closes automatically
    
    m_handBookLoaded = true;
}

HexPoint HandBookCheck::HandBookResponse(const StoneBoard& brd, 
                                         HexColor UNUSED(color))
{
    if (!m_handBookLoaded)
	LoadHandBook();
    
    hex::log << hex::info << "HandBookCheck: Seeking response" << hex::endl;
    hex::log << "Board ID: " << brd.GetBoardIDString() << hex::endl;
    
    for (uint i=0; i<m_id.size(); ++i) {
	if (brd.GetBoardIDString() == m_id[i]) {
	    hex::log << hex::info << "Found hand book response!" << hex::endl;
	    HexAssert(m_response[i] != INVALID_POINT);
	    HexAssert(brd.isEmpty(m_response[i]));
	    return m_response[i];
	}
    }
    
    hex::log << hex::info << "HandBookCheck: No response found." << hex::endl;
    return INVALID_POINT;
}

//----------------------------------------------------------------------------
