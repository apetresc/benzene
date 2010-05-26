//----------------------------------------------------------------------------
/** @file BookBuilderCommands.hpp
 */
//----------------------------------------------------------------------------

#ifndef BOOKBUILDERCOMMANDS_HPP
#define BOOKBUILDERCOMMANDS_HPP

#include "BookBuilder.hpp"
#include "BookCommands.hpp"
#include "HexHtpEngine.hpp"

_BEGIN_BENZENE_NAMESPACE_

//----------------------------------------------------------------------------

/** Commands for building opening books. */
template<class PLAYER>
class BookBuilderCommands : public BookCommands
{
public:
    BookBuilderCommands(Game& game, HexEnvironment& env, 
                        boost::scoped_ptr<Book>& book,
                        BookCheck& bookCheck, PLAYER& player);

    ~BookBuilderCommands();

    void Register(GtpEngine& engine);

private:

    BookBuilder<PLAYER> m_bookBuilder;
    
    typedef BookBuilderCommands<PLAYER> BookBuilderType;
    void Register(GtpEngine& engine, const std::string& command,
                  typename GtpCallback<BookBuilderType>::Method method);

    void CmdBookPriorities(HtpCommand& cmd);
    void CmdBookExpand(HtpCommand& cmd);
    void CmdBookRefresh(HtpCommand& cmd);
    void CmdBookIncreaseWidth(HtpCommand& cmd);
    void CmdParamBookBuilder(HtpCommand& cmd);
};

//----------------------------------------------------------------------------

template<class PLAYER>
BookBuilderCommands<PLAYER>::BookBuilderCommands(Game& game, 
                                                 HexEnvironment& env,
                                                 boost::scoped_ptr<Book>& book,
                                                 BookCheck& bookCheck, 
                                                 PLAYER& player)
    : BookCommands(game, env, book, bookCheck),
      m_bookBuilder(player)
{
}

template<class PLAYER>
BookBuilderCommands<PLAYER>::~BookBuilderCommands()
{
}

template<class PLAYER>
void BookBuilderCommands<PLAYER>::Register(GtpEngine& e)
{
    BookCommands::Register(e);
    Register(e, "book-expand", &BookBuilderCommands<PLAYER>::CmdBookExpand);
    Register(e, "book-priorities", 
             &BookBuilderCommands<PLAYER>::CmdBookPriorities);
    Register(e, "book-refresh", &BookBuilderCommands<PLAYER>::CmdBookRefresh);
    Register(e, "book-increase-width", 
             &BookBuilderCommands<PLAYER>::CmdBookIncreaseWidth);
    Register(e, "param_book_builder", 
             &BookBuilderCommands<PLAYER>::CmdParamBookBuilder);
}

template<class PLAYER>
void BookBuilderCommands<PLAYER>::Register(GtpEngine& engine, 
                                           const std::string& command,
                typename GtpCallback<BookBuilderType>::Method method)
{
    engine.Register(command, new GtpCallback<BookBuilderType>(this, method));
}

//----------------------------------------------------------------------------

template<class PLAYER>
void BookBuilderCommands<PLAYER>::CmdParamBookBuilder(HtpCommand& cmd)
{
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "[bool] use_widening " 
            << m_bookBuilder.UseWidening() << '\n'
            << "[bool] use_ice "
            << m_bookBuilder.UseICE() << '\n'
            << "[string] alpha "
            << m_bookBuilder.Alpha() << '\n'
            << "[string] expand_width "
            << m_bookBuilder.ExpandWidth() << '\n'
            << "[string] expand_threshold " 
            << m_bookBuilder.ExpandThreshold() << '\n'
            << "[string] num_threads " 
            << m_bookBuilder.NumThreads() << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "alpha")
            m_bookBuilder.SetAlpha(cmd.FloatArg(1));
        else if (name == "expand_width")
            m_bookBuilder.SetExpandWidth(cmd.SizeTypeArg(1, 1));
        else if (name == "expand_threshold")
            m_bookBuilder.SetExpandThreshold(cmd.SizeTypeArg(1, 1));
        else if (name == "num_threads")
            m_bookBuilder.SetNumThreads(cmd.SizeTypeArg(1));
        else if (name == "use_ice")
            m_bookBuilder.SetUseICE(cmd.BoolArg(1));
        else if (name == "use_widening")
            m_bookBuilder.SetUseWidening(cmd.BoolArg(1));
        else
            throw HtpFailure() << "unknown parameter: " << name;
    }
}

/** Expands the current node in the current opening book.
    "book-expand [iterations]"
*/
template<class PLAYER>
void BookBuilderCommands<PLAYER>::CmdBookExpand(HtpCommand& cmd)
{
    if (m_book.get() == 0) 
        throw HtpFailure() << "No open book.";
    cmd.CheckNuArg(1);
    int iterations = cmd.IntArg(0, 1);
    HexState state(m_game.Board(), m_game.Board().WhoseTurn());
    HexBoard& brd = m_env.SyncBoard(m_game.Board());
    m_bookBuilder.SetState(state);
    m_bookBuilder.SetWorkBoard(brd);
    m_bookBuilder.Expand(*m_book, iterations);
}

/** Refreshes the current book. See BookBuilder::Refresh(). */
template<class PLAYER>
void BookBuilderCommands<PLAYER>::CmdBookRefresh(HtpCommand& cmd)
{
    UNUSED(cmd);
    if (m_book.get() == 0) 
        throw HtpFailure() << "No open book.";
    HexState state(m_game.Board(), m_game.Board().WhoseTurn());
    HexBoard& brd = m_env.SyncBoard(m_game.Board());
    m_bookBuilder.SetState(state);
    m_bookBuilder.SetWorkBoard(brd);
    m_bookBuilder.Refresh(*m_book);
}

/** Increases the width of all internal nodes that need to be
    increased. See BookBuilder::IncreaseWidth().  */
template<class PLAYER>
void BookBuilderCommands<PLAYER>::CmdBookIncreaseWidth(HtpCommand& cmd)
{
    UNUSED(cmd);
    if (m_book.get() == 0) 
        throw HtpFailure() << "No open book.";
    HexState state(m_game.Board(), m_game.Board().WhoseTurn());
    HexBoard& brd = m_env.SyncBoard(m_game.Board());
    m_bookBuilder.SetState(state);
    m_bookBuilder.SetWorkBoard(brd);
    m_bookBuilder.IncreaseWidth(*m_book);
}

template<class PLAYER>
void BookBuilderCommands<PLAYER>::CmdBookPriorities(HtpCommand& cmd)
{
    if (m_book.get() == 0) 
        throw HtpFailure() << "No open book.";
    HexState state(m_game.Board(), m_game.Board().WhoseTurn());
    BookNode parent;
    if (!m_book->Get(state, parent))
        return;
    for (BitsetIterator p(state.Position().GetEmpty()); p; ++p) 
    {
        state.PlayMove(*p);
        BookNode succ;
        if (m_book->Get(state, succ))
        {
            cmd << " " << *p;
            float priority = m_bookBuilder.ComputePriority(parent, 
                                                           succ.m_value,
                                                           succ.m_priority);
            float value = BookUtil::InverseEval(succ.m_value);
            if (HexEvalUtil::IsWin(value))
                cmd << " W";
            else if (HexEvalUtil::IsLoss(value))
                cmd << " L";
            else
                cmd << " " << std::fixed << std::setprecision(1) << priority;
        }
        state.UndoMove(*p);
    }
}

//----------------------------------------------------------------------------

_END_BENZENE_NAMESPACE_

#endif // BOOKBUILDERCOMMANDS_HPP
