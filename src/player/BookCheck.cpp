//----------------------------------------------------------------------------
// $Id: BookCheck.cpp 1715 2008-10-28 22:13:37Z broderic $
//----------------------------------------------------------------------------

#include "BookCheck.hpp"
#include "BitsetIterator.hpp"
#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

BookCheck::BookCheck(UofAPlayer* player)
    : UofAPlayerFunctionality(player),
      m_openingBookLoaded(false)
{
}

BookCheck::~BookCheck()
{
}

HexPoint BookCheck::pre_search(HexBoard& brd, const Game& game_state,
			       HexColor color, bitset_t& consider,
                               double time_remaining, double& score)
{
    if (hex::settings.get_bool("player-use-book") &&
	hex::settings.get_int("player-book-max-depth") >
	(int) game_state.History().size()) {
	HexPoint response = BookResponse(brd, color);
        if (response != INVALID_POINT)
            return response;
    }
    return m_player->pre_search(brd, game_state, color, consider,
				time_remaining, score);
}

//----------------------------------------------------------------------------

void BookCheck::LoadOpeningBooks()
{
    // find and open the file with the names of all opening books
    std::string book_path = hex::settings.get("config-data-path");
    book_path.append("book/");
    std::string file = hex::settings.get("book-list-file");
    if (file[0] == '*') 
	file = book_path + file.substr(1);
    
    // open file if exists, else abort
    std::ifstream f;
    f.open(file.c_str());
    if (!f) {
        hex::log << hex::warning;
        hex::log << "Could not open file '" << file << "'!" << hex::endl;
        return;
    }
    
    // extract opening book file names/alpha values
    std::string line;
    while (getline(f, line)) {
	std::istringstream iss;
	iss.str(line);
	std::string name;
	iss >> name;
	m_openingBookFiles.push_back(book_path + name);
	double alpha;
	iss >> alpha;
	m_openingBookAlphas.push_back(alpha);
    }
    HexAssert(m_openingBookFiles.size() == m_openingBookAlphas.size());
    // note: file closes automatically
    
    m_openingBookLoaded = true;
}

HexPoint BookCheck::BookResponse(HexBoard& brd, HexColor color)
{
    if (!m_openingBookLoaded)
	LoadOpeningBooks();
    
    hex::log << hex::info << "BookCheck: Searching books for response"
	     << hex::endl;
    
    m_openingBookDepthAdjustment =
	hex::settings.get_double("player-book-depth-value-adjustment");
    m_openingBookMinDepth = hex::settings.get_int("player-book-min-depth");
    HexPoint bestMove = INVALID_POINT;
    float bestScore = -1e10;
    
    // compute rotation of this board so we can check that state as well
    StoneBoard rotatedBrd(brd.width(), brd.height());
    rotatedBrd.addColor(BLACK, brd.rotate(brd.getBlack()));
    rotatedBrd.addColor(WHITE, brd.rotate(brd.getWhite()));
    rotatedBrd.setPlayed(brd.rotate(brd.getPlayed()));
    
    
    // scan opening book files for the best move beyond min depth
    HexAssert(m_openingBookFiles.size() == m_openingBookAlphas.size());
    for (unsigned i=0; i<m_openingBookFiles.size(); i++) {
	OpeningBook book(brd.width(), brd.height(), m_openingBookAlphas[i],
			 m_openingBookFiles[i]);
	
	// see if can find a better-rated move in this opening book,
	// checking both the board and its rotation
	float score;
	HexPoint move;
	ComputeBestChild(brd, color, book, move, score);
	if (score > bestScore) {
	    bestScore = score;
	    bestMove = move;
	    hex::log << hex::info << "New best:"
		     << " " << m_openingBookFiles[i]
		     << " " << HexPointUtil::toString(bestMove)
		     << " " << bestScore
		     << hex::endl;
	}
	ComputeBestChild(rotatedBrd, color, book, move, score);
	if (score > bestScore) {
	    bestScore = score;
	    bestMove = brd.rotate(move);
	    hex::log << hex::info << "New best (from rotation):"
		     << " " << m_openingBookFiles[i]
		     << " " << HexPointUtil::toString(bestMove)
		     << " " << bestScore
		     << hex::endl;
	}
    }
    
    return bestMove;
}

void BookCheck::ComputeBestChild(StoneBoard& brd, HexColor color,
                                 const OpeningBook& book, HexPoint& move,
                                 float& score)
{
    score = -1e10;
    move = INVALID_POINT;
    
    // if book does not contain state with a minimum depth, just quit
    if (book.GetMainLineDepth(brd, color) <= m_openingBookMinDepth) return;
    
    for (BitsetIterator p(brd.getEmpty()); p; ++p) {
	brd.playMove(color, *p);
	
	// get value of this move, giving a bonus to deeper values
	int depth = book.GetMainLineDepth(brd, !color);
	if (depth >= m_openingBookMinDepth) {
	    float curScore = -book.GetNode(brd.Hash()).getPropValue();
	    /*hex::log << hex::info << "Value for "
		     << HexPointUtil::toString(*p) << ": " << curScore
		     << " at depth " << depth << hex::endl;
	    */
	    curScore += depth * m_openingBookDepthAdjustment;
	    /*hex::log << hex::info << "Depth adjusted value: " << curScore
		     << hex::endl;
	    */
	    
	    // store the best (adjusted) score and move
	    if (curScore > score) {
		score = curScore;
		move = *p;
	    }
	}
	
	brd.undoMove(*p);
    }
}

//----------------------------------------------------------------------------
