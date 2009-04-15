//----------------------------------------------------------------------------
// $Id: FakeInteger.hpp 1168 2008-02-22 00:09:30Z broderic $
//----------------------------------------------------------------------------

#ifndef FAKEINTEGER_HPP
#define FAKEINTEGER_HPP

/** Class that mimics the 'int' type. */
class FakeInteger
{
public:
    FakeInteger() : m_v(0) { }
    FakeInteger(int a) : m_v(a) { }

    operator int() const;

    FakeInteger& operator++();
    FakeInteger operator++(int);
    FakeInteger& operator--();
    FakeInteger operator--(int);
    
    FakeInteger& operator+=(FakeInteger& o) { m_v+=o.m_v; return *this; }
    FakeInteger& operator-=(FakeInteger o) { m_v-=o.m_v; return *this; }
    FakeInteger& operator/=(FakeInteger o) { m_v/=o.m_v; return *this; }
    FakeInteger& operator*=(FakeInteger o) { m_v*=o.m_v; return *this; }
    FakeInteger& operator%=(FakeInteger o) { m_v%=o.m_v; return *this; }

    FakeInteger& operator&=(FakeInteger o) { m_v&=o.m_v; return *this; }
    FakeInteger& operator|=(FakeInteger o) { m_v|=o.m_v; return *this; }
    FakeInteger& operator^=(FakeInteger o) { m_v^=o.m_v; return *this; }

    FakeInteger& operator<<=(FakeInteger o) { m_v<<=o.m_v; return *this; }
    FakeInteger& operator>>=(FakeInteger o) { m_v>>=o.m_v; return *this; }
    
    FakeInteger operator=(FakeInteger o) { m_v = o.m_v; return *this; }

private:
    int m_v;
};

inline FakeInteger::operator int() const 
{ 
    return m_v; 
}

inline FakeInteger& FakeInteger::operator++() 
{
    ++m_v; 
    return *this; 
}

inline FakeInteger FakeInteger::operator++(int) 
{ 
    FakeInteger ret = *this; 
    ++(*this); 
    return ret; 
}

inline FakeInteger& FakeInteger::operator--() 
{ 
    --m_v; 
    return *this; 
}

inline FakeInteger FakeInteger::operator--(int) 
{ 
    FakeInteger ret = *this; 
    --(*this); 
    return ret; 
}

//----------------------------------------------------------------------------

#endif // FAKEINTEGER_HPP
