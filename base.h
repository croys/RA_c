#pragma once

#include <stdexcept>
#include <sstream>
#include <iostream>

namespace rac
{

struct not_implemented : public std::logic_error
{
    not_implemented() : std::logic_error( "Not yet implemented" ) { }
};

}

template<typename T>
class thrower : public std::ostringstream
{
public:
// This can't work as write is not virtual.
//    basic_ostream& write( const char_type* s, std::streamsize count ) override
// relying on ends is fragile anyway.
    basic_ostream& write( const char_type* s, std::streamsize count )
    {
        if ( (count == 1) && (*s == '\0') ) {
            throw T( std::ostringstream::str() );
        } else {
            std::ostringstream::write(s, count);
            return *this;
        }
    }
};


template<typename T, typename U>
void throw_with( U& s )
{
    std::ostringstream* ss = dynamic_cast<std::ostringstream*>(&s);
    if (ss) {
        throw T( ss->str() );
    } else {
        throw std::logic_error( "Guru meditation: ostringstream expected!" );
    }
}

template< typename T >
void throw_lambda( auto l )
{
    std::ostringstream ss;
    l(ss);
    throw T( ss.str() );
}
