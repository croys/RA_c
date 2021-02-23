#pragma once

#include <vector>
//#include <string>
#include <string_view>
#include <tuple>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <sstream>

#include "base.h"

namespace rac
{

// Types of basic objects that can be stored in a relation

typedef enum {
    Void, Bool, Int, Float, Double, String, Date, Time,
    Object,
} ty_con_t;


// FIXME: extend to tycons with args
struct type_t
{
    ty_con_t ty_con;

};

inline bool operator==(const type_t& ta, const type_t& tb)
{
    return ta.ty_con == tb.ty_con;
}

inline bool operator!=(const type_t& ta, const type_t& tb)
{
    return ta.ty_con == tb.ty_con;
}


inline bool operator<(const type_t& ta, const type_t& tb)
{
    return ta.ty_con < tb.ty_con;
}



// FIXME: traits of C++ types

// FIXME: uint32_t, uint64_t, size_t, pointers
std::string ty_to_string( const type_t& ty )
{
    switch (ty.ty_con) {
        case Void:      return "Void";
        case Bool:      return "Bool";
        case Int:       return "Int";
        case Float:     return "Float";
        case Double:    return "Double";
        case String:    return "String";
        case Date:      return "Date";
        case Time:      return "Time";
        case Object:    return "Object";
    }
    throw std::invalid_argument( "Unrecognised type" );
}

//typedef std::u8string_view cstring_t;
typedef std::string_view cstring_t;

// Relation type
#if 0
typedef
struct
{
public:
    // construction

    //relation_ty()

    // deconstruction  - iterators

//private:
    std::vector< std::tuple< cstring_t, type_t > > m_tys;
} rel_ty_t;

#endif


//typedef std::vector< std::tuple< cstring_t, type_t > > rel_ty_t;
//typedef std::vector< std::pair< cstring_t, type_t > > rel_ty_t;
typedef std::vector< std::pair< std::string, type_t > > rel_ty_t;

std::string rel_ty_to_string( const rel_ty_t& rty )
{
    std::ostringstream ss;
    ss << "{ ";
    for (auto it = rty.cbegin(); it != rty.cend(); ++it) {
        if (it != rty.cbegin())
            ss << ", ";
        ss  << std::get<0>(*it) << " : " << ty_to_string( std::get<1>(*it) );
    }
    ss << " }";
    return ss.str();
}


// Operations on relation types


// union
rel_ty_t rty_union( const rel_ty_t& a, const rel_ty_t& b )
{
    std::vector< std::pair< std::string, type_t > > res;
    std::map< cstring_t, type_t > b_names( b.cbegin(), b.cend() );

    for( auto a_it = a.cbegin(); a_it != a.cend(); ++a_it )
    {
        auto b_it = b_names.find( std::get<0>( *a_it ) );
        if ( b_it != b_names.cend() )
        {
            if ( std::get<1>( *a_it ) != std::get<1>( *b_it ) ) {
                throw_with< std::invalid_argument >(
                    std::ostringstream()
                    << "Types for '" << std::get<0>(*a_it).data() << " do not match: "
                    << ty_to_string( std::get<1>(*a_it) )
                    << " and "
                    << ty_to_string( std::get<1>(*b_it) )
                );
            }
            res.emplace_back( *a_it );
        }
    }
    return res;
}

// project
template <typename Iterator>
rel_ty_t rty_project( const rel_ty_t& a, Iterator begin, Iterator end )
{
    throw not_implemented();
}

// minus

rel_ty_t rty_minus( const rel_ty_t& a, const rel_ty_t& b )
{
    throw not_implemented();
}


// intersect

rel_ty_t rty_intersect( const rel_ty_t& a, const rel_ty_t& b )
{
    throw not_implemented();
}


// allBut




}