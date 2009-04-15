//----------------------------------------------------------------------------
// $Id$
//----------------------------------------------------------------------------

#include "HandBookCheck.hpp"
#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

HandBookCheck::HandBookCheck(UofAPlayer* player)
    : UofAPlayerFunctionality(player),
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
    if (hex::settings.get_bool("player-use-hand-book")) {
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
    std::string book_path = hex::settings.get("config-data-path");
    book_path.append("book/");
    std::string file = hex::settings.get("hand-book-file");
    if (file[0] == '*') 
	file = book_path + file.substr(1);
    
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

HexPoint HandBookCheck::HandBookResponse(const StoneBoard& brd, HexColor color)
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
