//----------------------------------------------------------------------------
// $Id: HexException.cpp 627 2007-08-23 00:04:48Z broderic $
//----------------------------------------------------------------------------

#include "HexException.hpp"

//----------------------------------------------------------------------------

HexException::HexException()
    : m_message("")
{ }

HexException::HexException(const std::string& message)
    : m_message(message)
{ }

HexException::~HexException() throw()
{ }


const char* HexException::what() const throw()
{
    return m_message.c_str();
}

//----------------------------------------------------------------------------
