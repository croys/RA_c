#pragma once

#include <vector>
//#include <string>
#include <string_view>
#include <tuple>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <ostream>
#include <sstream>
#include <initializer_list>

#include "base.h"

namespace rac
{

// Types of basic objects that can be stored in a relation

// FIXME: uint32_t, uint64_t, size_t, pointers

typedef enum {
    Void, Bool, Int, Float, Double, String, Date, Time,
    Object,
} ty_con_t;


// FIXME: extend to tycons with args
struct type_t
{
    ty_con_t ty_con;

};
constexpr bool operator==(const type_t& ta, const type_t& tb)
{
    return ta.ty_con == tb.ty_con;
}

constexpr auto operator<=>(const type_t& ta, const type_t& tb)
{
    return ta.ty_con <=> tb.ty_con;
}


std::ostream& ty_to_stream( std::ostream& os, const type_t& ty )
{
    switch (ty.ty_con) {
        case Void:      return os << "Void";
        case Bool:      return os << "Bool";
        case Int:       return os << "Int";
        case Float:     return os << "Float";
        case Double:    return os << "Double";
        case String:    return os << "String";
        case Date:      return os << "Date";
        case Time:      return os << "Time";
        case Object:    return os << "Object";
    }
    throw std::invalid_argument( "Unrecognised type" );
}

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


// columns are typed and have names and ordering
// used in relation_builder and table_view
typedef std::vector< std::pair< std::string, type_t > > col_tys_t;

std::ostream& col_tys_to_stream( std::ostream& os, const col_tys_t& col_tys )
{
    os << "{ ";
    for (auto it = col_tys.cbegin(); it != col_tys.cend(); ++it) {
        if (it != col_tys.cbegin())
            os << ", ";
        os  << std::get<0>(*it) << " : " << ty_to_string( std::get<1>(*it) );
    }
    os << " }";

    return os;
}

std::string col_tys_to_string( const col_tys_t& col_tys )
{
    std::ostringstream ss;
    col_tys_to_stream( ss, col_tys );
    return ss.str();
}

typedef std::string_view cstring_t;


// Relation type
//
// A relation type is built up of typed and named columns, but has no
// ordering
struct rel_ty_t
{
private:
    void construct()
    {
        // Normal/canonical form is just sorted
        std::sort( m_tys.begin(), m_tys.end() );

        // Check for repeated column names
        if ( m_tys.size() > 1 )
        {
            for ( size_t i=0; i < m_tys.size() - 1; ++i )
            {
                if ( m_tys[ i ].first == m_tys[ i+1 ].first ) {
                    throw_with< std::invalid_argument >(
                        std::ostringstream()
                        << "Column name '" << m_tys[ i ].first << "' repeated"
                    );
                }
            }
        }
    }
public:

    explicit rel_ty_t( const col_tys_t& col_tys )
        : m_tys( col_tys )
    {
        construct();
    }

    explicit rel_ty_t( col_tys_t&& col_tys )
        : m_tys( col_tys )
    {
        construct();
    }

    rel_ty_t( std::initializer_list< col_tys_t::value_type > init )
        : m_tys( init )
    {
        construct();
    }

    col_tys_t m_tys;
};


inline bool operator==(const rel_ty_t& ta, const rel_ty_t& tb)
{
    return ta.m_tys == tb.m_tys;
}

inline auto operator<=>(const rel_ty_t& ta, const rel_ty_t& tb)
{
    return ta.m_tys <=> tb.m_tys;
}


// Operations on relation types


// union
rel_ty_t rty_union( const rel_ty_t& a, const rel_ty_t& b )
{
    std::vector< std::pair< std::string, type_t > > res;
    std::map< cstring_t, type_t > b_names( b.m_tys.cbegin(), b.m_tys.cend() );

    for( auto a_it = a.m_tys.cbegin(); a_it != a.m_tys.cend(); ++a_it )
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
    return rel_ty_t( std::move( res ) );
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


// all_but




}