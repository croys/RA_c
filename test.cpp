#include <stdexcept>
#include <sstream>
#include <vector>
#include <array>
#include <iostream>
#include <memory_resource>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "relation.h"

using namespace rac;

TEST_CASE( "base basics" , "[base]" ) {
    {
        bool thrown = false;
        std::string msg;
        try {
            throw_with< std::runtime_error >(
                std::ostringstream() << "Test"
            );
        } catch (const std::exception& e) {
            thrown = true;
            msg = e.what();
        }
        REQUIRE( thrown == true );
        REQUIRE( msg == "Test" );
    }

    {
        bool thrown = false;
        std::string msg;
        try {
            throw_lambda< std::invalid_argument >(
                [=]( std::ostringstream& ss ) { ss
                    << "Lambda throw";
                } );
        } catch (const std::exception& e) {
            thrown = true;
            msg = e.what();
        }
        REQUIRE( thrown == true );
        REQUIRE( msg == "Lambda throw" );
    }
}

TEST_CASE( "type_t basics", "[type_t]" ) {
    std::vector<type_t> tys( {
         { Void }, { Bool }, { Int }, { Float }, { Double }, { String },
         { Date }, { Time }, { Object }
        } );
    std::vector<std::string> expected(
        {
             "Void", "Bool", "Int", "Float", "Double", "String"
            ,"Date", "Time", "Object"
    } );
    std::vector<std::string> res;
    for( auto it = tys.begin(); it != tys.end(); ++it ) {
        res.push_back( ty_to_string(*it) );
    }
    REQUIRE( res == expected );
}

TEST_CASE( "rel_ty_t basics", "[rel_ty_t]") {
    std::vector< std::pair< std::string, type_t > > col_tys( {
         { "A", { Void } }, { "B", { Bool } }, { "C", { Int } }
        ,{ "D", { Float } }, { "E", { Double } }, { "F", { String } }
        ,{ "G", { Date } }, { "H", { Time } }, { "I", { Object } }
        } );

    std::string expected =
        "{ A : Void, B : Bool, C : Int, D : Float, E : Double, F : String, "
        "G : Date, H : Time, I : Object }";

    rel_ty_t rel_ty( col_tys );
    REQUIRE( rel_ty_to_string( rel_ty ) == expected );

}

TEST_CASE( "column_storage basics", "[column_storage] [untyped_column_storage]") {
    std::array< std::uint8_t, 32768 > buffer{};
    std::pmr::monotonic_buffer_resource rsrc( buffer.data(), buffer.size() );

    column_storage< int > cs_int( &rsrc );
    untyped_column_storage< int > ucs_int( &cs_int );
    IStorage* is = static_cast<IStorage*>( &ucs_int );
    const IStorage* cis = static_cast<const IStorage*>( &ucs_int );

    REQUIRE( cs_int.empty() );

    REQUIRE( cs_int.cend() - cs_int.cbegin() == 0 );
    REQUIRE( cis->cend() - cis->cbegin()  == 0 );

    REQUIRE( cs_int.size() == 0 );
    REQUIRE( ucs_int.size() == 0 );
    REQUIRE( is->size() == 0 );
    REQUIRE( cis->size() == 0 );

    const size_t n = 100;
    is->resize( n );
    REQUIRE( cs_int.size() == n );
    REQUIRE( ucs_int.size() == n );
    REQUIRE( is->size() == n );
    REQUIRE( cis->size() == n );
    REQUIRE( cs_int.end() - cs_int.cbegin() == cs_int.size() );
    REQUIRE( cis->cend() - cis->cbegin() == cis->size()  * sizeof(int) );


    column_storage<int>::iterator it;
    size_t i = 0;
    for ( it = cs_int.begin(); it != cs_int.end(); ++it )
    {
        *it = i++;
    }

    auto as_cint = []( const value* v )
    {
        return reinterpret_cast<const int *>(v);
    };

    REQUIRE( cs_int.at(0) == 0 );
    REQUIRE( cs_int.at(99) == 99 );
    REQUIRE( *as_cint( cis->at(0) ) == 0 );
    REQUIRE( *as_cint( cis->at(99) ) == 99 );


    column_storage< int > cs_int2( &rsrc );
    untyped_column_storage< int > ucs_int2( &cs_int2 );
    IStorage* is2 = static_cast<IStorage*>( &ucs_int2 );
    const IStorage* cis2 = static_cast<const IStorage*>( &ucs_int2 );

    is2->resize( n );

    REQUIRE( cs_int2.size() == n );
    REQUIRE( ucs_int2.size() == n );
    REQUIRE( is2->size() == n );
    REQUIRE( cis2->size() == n );

    is2->move( is->begin(), is->end(), is2->begin() );
    REQUIRE( *as_cint( cis2->at( 0 ) ) == 0 );
    REQUIRE( *as_cint( cis2->at( 99 ) ) == 99 );

    for ( it = cs_int.begin(); it != cs_int.end(); ++it ) {
        *it = 0;
    }

    REQUIRE( *as_cint( cis->at( 0 ) ) == 0 );
    REQUIRE( *as_cint( cis->at( 99 ) ) == 0 );

    is->copy( cis2->cbegin(), cis2->cend(), is->begin() );

    REQUIRE( *as_cint( cis->at( 0 ) ) == 0 );
    REQUIRE( *as_cint( cis->at( 99 ) ) == 99 );

    for (i = 0; i<n; ++i) {
        cs_int2.at( i ) = n-i;
    }
    REQUIRE( cs_int2.at( 0 ) == n );
    REQUIRE( cs_int2.at( 99 ) == n - 99 );

    for (i = 0; i<n; ++i) {
        cs_int2[i] = i;
    }

    REQUIRE( cs_int2[ 0 ] == 0 );
    REQUIRE( cs_int2.at( 0 ) == 0 );
    REQUIRE( cs_int2[ 99 ] == 99 );
    REQUIRE( cs_int2.at( 99 ) == 99 );

}
