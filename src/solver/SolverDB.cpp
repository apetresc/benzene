//----------------------------------------------------------------------------
// $Id: SolverDB.cpp 1786 2008-12-14 01:55:45Z broderic $
//----------------------------------------------------------------------------

#include <cstring>
#include <cstdio>

#include "Time.hpp"
#include "Hex.hpp"
#include "BoardUtils.hpp"
#include "BitsetIterator.hpp"
#include "SolverDB.hpp"
#include "SortedSequence.hpp"

//----------------------------------------------------------------------------

/** Dumps some debug output. */
#define PRINT_OUTPUT 0

//----------------------------------------------------------------------------

SolverDB::SolverDB()
{
}

SolverDB::~SolverDB()
{
}

//----------------------------------------------------------------------------


bool SolverDB::open(int width, int height, int maxstones, int transtones,
                    const std::string& filename)
{
    hex::log << hex::info;
    hex::log << "SolverDB: attempting to open DB..." << hex::endl;

    m_settings = Settings(width, height, transtones, maxstones);

    bool db_opened_successfully = m_db.Open(filename);
    HexAssert(db_opened_successfully);

    // Load settings from database and ensure they match the current
    // settings.  
    char key[] = "settings";
    Settings temp;
    if (m_db.Get(key, strlen(key)+1, &temp, sizeof(temp))) {
        hex::log << hex::info << "Database exists." << hex::endl;
        if (m_settings != temp) {
            hex::log << "Settings do not match!" << hex::endl;
            hex::log << "DB: " << temp.toString() << hex::endl;
            hex::log << "Current: " << m_settings.toString() << hex::endl;
            return false;
        } 
    } else {
        // Read failed: this is a new database.
        // Store the settings.
        hex::log << hex::info << "New database!" << hex::endl;
        bool settings_stored_successfully = 
            m_db.Put(key, strlen(key)+1, &m_settings, sizeof(m_settings));
        HexAssert(settings_stored_successfully);
    }

    hex::log << "Settings: " << m_settings.toString() << hex::endl;
    return true;
}

bool SolverDB::open(int width, int height, const std::string& filename)
{
    hex::log << hex::info;
    hex::log << "SolverDB: attempting to open DB..." << hex::endl;

    bool db_opened_successfully = m_db.Open(filename);
    HexAssert(db_opened_successfully);

    // Load settings from database
    char key[] = "settings";
    if (m_db.Get(key, strlen(key)+1, &m_settings, sizeof(m_settings))) {
        hex::log << "Settings: " << m_settings.toString() << hex::endl;

        if (m_settings.width == width && m_settings.height == height)
            return true;
        
        hex::log << hex::warning << "Dimensions do not match!" << hex::endl;
        return false;
    } 


    // read failed; return false
    return false;
}

void SolverDB::close()
{
    m_db.Close();
}

//----------------------------------------------------------------------------

bool SolverDB::get(const StoneBoard& brd, SolvedState& state)
{
    int count = brd.numStones();
    if (0 < count && count <= m_settings.maxstones) {

        // check if exact boardstate exists in db
        if (m_db.Get(brd.Hash(), state)) {
            m_stats.gets++;
            m_stats.saved += state.numstates;
            
            state.hash = brd.Hash();
            state.numstones = brd.numStones();

            return true;
        }

        // check if rotated boardstate exists in db
        StoneBoard rotated_brd(brd);
        rotated_brd.rotateBoard();
        if (m_db.Get(rotated_brd.Hash(), state)) {

            // rotate proof so it matches the given board
            state.proof = BoardUtils::Rotate(brd.Const(), state.proof);
            state.winners_stones = BoardUtils::Rotate(brd.Const(), 
                                                      state.winners_stones);

            state.hash = brd.Hash();
            state.numstones = brd.numStones();

            m_stats.gets++;
            m_stats.saved += state.numstates;
            return true;
        }
    }
    return false;
}

bool SolverDB::check(const StoneBoard& brd)
{
    int count = brd.numStones();
    if (0 < count && count <= m_settings.maxstones) {
        if (m_db.Exists(brd.Hash()))
            return true;

        StoneBoard rotated_brd(brd);
        rotated_brd.rotateBoard();
        if (m_db.Exists(rotated_brd.Hash()))
            return true;
    }
    return false;
}

int SolverDB::write(const StoneBoard& brd, const SolvedState& state)
{
    int count = brd.numStones();
    if (0 < count && count <= m_settings.maxstones) {
        
        SolvedState old_state;
        bool old_exists = get(brd, old_state);

        if (old_exists && old_state.win != state.win) {
            hex::log << hex::severe
                     << "old win = " << old_state.win << hex::endl
                     << "new win = " << state.win << hex::endl
                     << "old_proof = " 
                     << brd.printBitset(old_state.proof & brd.getEmpty())
                     << hex::endl
                     << "new_proof = " 
                     << brd.printBitset(state.proof & brd.getEmpty()) 
                     << hex::endl;
            HexAssert(false);
        }

        // do not overwrite a proof unless the new one is smaller
        if (old_exists && old_state.proof.count() <= state.proof.count())
            return 0;

        // track the shrinkage
        if (old_exists) {
            m_stats.shrunk++;
            m_stats.shrinkage += 
                old_state.proof.count() - state.proof.count();
        }
        
        if (m_db.Put(brd.Hash(), state)) {
            m_stats.writes++;
            return 1;
        }
    }
    return 0;
}

int SolverDB::put(const StoneBoard& brd, const SolvedState& state)
{
    int count = brd.numStones();
    if (0 < count && count <= m_settings.maxstones) {

        int wrote = write(brd, state);
        if (count <= m_settings.trans_stones) {
            wrote += SolverDBUtil::StoreTranspositions(*this, brd, state);
            wrote += SolverDBUtil::StoreFlippedStates(*this, brd, state);
        }
	
        if (wrote) {
            m_stats.puts++;
        }
        return wrote;
    }
    return 0;
}

//----------------------------------------------------------------------------

int SolverDBUtil::StoreTranspositions(SolverDB& db, 
                                      const StoneBoard& brd, 
                                      const SolvedState& state)
{
    int numstones = brd.numStones();
    int numblack = (numstones+1) / 2;
    int numwhite = numstones / 2;
    HexAssert(numblack + numwhite == numstones);

    // Find color of losing/winning players
    HexColor toplay = brd.WhoseTurn();
    HexColor other = !toplay;
    HexColor loser = (state.win) ? other : toplay;
    HexColor winner = (state.win) ? toplay : other;

    // Loser can use his stones as well as all those outside the proof
    bitset_t outside = (~state.proof & brd.getEmpty()) | 
                       (brd.getColor(loser) & brd.getCells());

    // Winner can use his stones
    // @todo fix this so only relevant stones to the proof are used.
    bitset_t winners = brd.getColor(winner) & brd.getCells();

    // store the players' stones as lists of sorted indices  
    std::vector<HexPoint> black, white;
    std::vector<HexPoint>& lose_list = (loser == BLACK) ? black : white;
    std::vector<HexPoint>& winn_list = (loser == BLACK) ? white : black;
    BitsetUtil::BitsetToVector(outside, lose_list);
    BitsetUtil::BitsetToVector(winners, winn_list);

    HexAssert(black.size() >= (unsigned)numblack);
    HexAssert(white.size() >= (unsigned)numwhite);

#if 0
    hex::log << hex::info;
    hex::log << "[" << numstones << "]" 
             << " StoreTranspositions" << hex::endl;
    hex::log << brd.printBitset(state.proof & brd.getEmpty()) << hex::endl;
    hex::log << "Winner: " << winner << hex::endl;
    hex::log << "Black positions: " << black.size() << hex::endl;
    hex::log << HexPointUtil::ToPointListString(black)<< hex::endl;
    hex::log << "White positions: " << white.size() << hex::endl;
    hex::log << HexPointUtil::ToPointListString(white)<< hex::endl;
    hex::log << "outside proof:" << hex::endl;
    hex::log << brd.printBitset(outside) << hex::endl;
#endif

    // write each transposition 
    int count=0;
    StoneBoard board(brd.width(), brd.height());
    SortedSequence bseq(black.size(), numblack);
    while (!bseq.finished()) {
        SortedSequence wseq(white.size(), numwhite);
        while (!wseq.finished()) {

            // convert the indices into cells
            board.startNewGame();
            for (int i=0; i<numblack; ++i) {
                board.playMove(BLACK, black[bseq[i]]);
            }
            for (int i=0; i<numwhite; ++i) {
                board.playMove(WHITE, white[wseq[i]]);
            }

            // mark state as transposition if the current one is not
            // the original.
            SolvedState ss(state);
            if (board.Hash() != brd.Hash())
                ss.flags |= SolvedState::FLAG_TRANSPOSITION;
            
            // do the write; this handles replacing only larger
            // proofs, etc.
            count += db.write(board, ss);
            
            ++wseq;
        }
        ++bseq;
    }
    return count;
}


int SolverDBUtil::StoreFlippedStates(SolverDB& db, 
                                     const StoneBoard& brd,
                                     const SolvedState& state)
{
    // Start by computing the flipped board position.
    // This involves mirroring the stones and *flipping their colour*.
    bitset_t flippedBlack = BoardUtils::Mirror(brd.Const(), 
                            brd.getWhite() & brd.getPlayed() & brd.getCells());
    bitset_t flippedWhite = BoardUtils::Mirror(brd.Const(),
                            brd.getBlack() & brd.getPlayed() & brd.getCells());
    StoneBoard flippedBrd(brd.width(), brd.height());
    flippedBrd.startNewGame();
    flippedBrd.addColor(BLACK, flippedBlack);
    flippedBrd.addColor(WHITE, flippedWhite);
    flippedBrd.setPlayed(flippedBlack | flippedWhite);
#if PRINT_OUTPUT
    hex::log << hex::info << "Original Board:" << brd << hex::endl
	     << "Flipped Board:" << flippedBrd << hex::endl;
#endif
    
    // Find color of winning player in *flipped state*
    HexColor toPlay = brd.WhoseTurn();
    HexColor flippedWinner = (state.win) ? !toPlay : toPlay;
#if PRINT_OUTPUT
    hex::log << hex::info << "Normal winner: "
	     << (state.win ? toPlay : !toPlay) << hex::endl;
    hex::log << hex::info << "Flipped winner: "
	     << flippedWinner << hex::endl;
#endif
    
    // Find empty cells outside the flipped proof, if any
    bitset_t flippedProof = BoardUtils::Mirror(brd.Const(), state.proof);
    bitset_t flippedOutside = (~flippedProof & flippedBrd.getEmpty());
#if PRINT_OUTPUT
    hex::log << hex::info << "Flipped proof:"
	     << flippedBrd.printBitset(flippedProof) << hex::endl;
#endif
    
    // We need to determine what stones we can add or remove.
    bool canAddFlippedBlack = false;
    bitset_t flippedBlackToAdd;
    bool canRemoveFlippedWhite = false;
    bitset_t flippedWhiteToRemove;
    // To switch player to move (while keeping parity valid), we must either
    // add 1 stone to flippedBlack or else delete 1 stone from flippedWhite.
    // Note that we can always add winner stones or delete loser stones
    // without changing the value, and we can add loser stones if the proof
    // set does not cover all empty cells.
    if (flippedWinner == BLACK) {
	canAddFlippedBlack = true;
	flippedBlackToAdd = flippedBrd.getEmpty();
	canRemoveFlippedWhite = true;
	flippedWhiteToRemove = flippedWhite;
    } else {
	HexAssert(flippedWinner == WHITE);
	canAddFlippedBlack = flippedOutside.any();
	flippedBlackToAdd = flippedOutside;
    }
    HexAssert(canAddFlippedBlack != flippedBlackToAdd.none());
    HexAssert(BitsetUtil::IsSubsetOf(flippedBlackToAdd,flippedBrd.getEmpty()));
    HexAssert(canRemoveFlippedWhite != flippedWhiteToRemove.none());
    HexAssert(BitsetUtil::IsSubsetOf(flippedWhiteToRemove,flippedWhite));
    
    // Now we can create and store the desired flipped states.
    // Note that numstates and nummoves are approximations.
    SolvedState ss;
    ss.win = state.win;
    ss.flags = state.flags 
        | SolvedState::FLAG_TRANSPOSITION 
        | SolvedState::FLAG_MIRROR_TRANSPOSITION;
    ss.numstates = state.numstates;
    ss.nummoves = state.nummoves;
    ss.proof = flippedProof;
    ss.winners_stones = (flippedWinner == BLACK) ? flippedBlack : flippedWhite;
    
    int count = 0;
    if (canAddFlippedBlack) {
#if PRINT_OUTPUT
	hex::log << hex::info << "Add-Black Flips:" << hex::endl;
#endif
	for (BitsetIterator i(flippedBlackToAdd); i; ++i) {
	    flippedBrd.playMove(BLACK, *i);
	    HexAssert(!toPlay == flippedBrd.WhoseTurn());
	    HexAssert(!ss.winners_stones.test(*i));
	    if (flippedWinner == BLACK) {
		ss.winners_stones.set(*i);
		ss.proof.set(*i);
	    }
#if PRINT_OUTPUT
	    hex::log << hex::info << flippedBrd << hex::endl;
#endif
	    count += db.write(flippedBrd, ss);
	    ss.proof = flippedProof;
	    ss.winners_stones.reset(*i);
	    flippedBrd.undoMove(*i);
	}
    }
    if (canRemoveFlippedWhite) {
#if PRINT_OUTPUT
	hex::log << hex::info << "Remove-White Flips:" << hex::endl;
#endif
	for (BitsetIterator i(flippedWhiteToRemove); i; ++i) {
	    flippedBrd.undoMove(*i);
	    HexAssert(!toPlay == flippedBrd.WhoseTurn());
#if PRINT_OUTPUT
	    hex::log << hex::info << flippedBrd << hex::endl;
#endif
	    count += db.write(flippedBrd, ss);
	    flippedBrd.playMove(WHITE, *i);
	}
    }
    return count;
}

//----------------------------------------------------------------------------
