#pragma once

#include <memory_resource>
#include <vector>
#include <algorithm>
#include <execution>

#include "base.h"
#include "types.h"

namespace rac
{

template< typename Container, typename... Values>
auto make_container( auto* resource, Values&&... values) {
    Container result{resource};
    result.reserve( sizeof...(values) );
    (result.emplace_back(std::forward<Values>(values)), ...);
    return result;
}

struct value{};

// We use one monotonic_buffer_resource per column
// to get good locality without fragmentation

// Make this a base class, so we can specialise, e.g. std::string -> std::pmr::string?
// Or just have a bunch of helper functions for std::string, etc..
// and always use std::pmr::string?
// deconstruction should be string_view...
// - have a floor size - do reserve on first alloc/store
// - might want two resources for strings
//   - one for string
//   - another for data and/or pooled strings
// - separate resource for each of strings and data
// - one more for string_views into pool
// might want to share pool across the whole relation, or even across relations
// => need pool to use shared pointers

// FIXME: need abstract base class to act as an interface for providing
// void * access & value operations - copy, move, (emplace?)
// also want bulk operations
// Should be abc rather than component as we want to be able to
// extract the typed version


// untyped access to aligned, contiguous storage of monotyped values
//
// tempting to split out interfaces into pure and mutable, but all
// implementations will implement both, so we can just stick
// with const methods and use IStorage/const IStorage as appropriate
struct IStorage
{
    // immutable access
    virtual const value*    at( size_t idx ) const = 0;
    virtual size_t          size() const noexcept = 0;
    virtual const value*    cbegin() const noexcept = 0;
    virtual const value*    cend() const noexcept = 0;

    // mutable access
    virtual value*  at( size_t idx ) = 0;
    virtual value*  begin() noexcept = 0;
    virtual value*  end() noexcept = 0;

    // reserve?

    // resize? 
    virtual void resize( size_t sz ) = 0;

    // push_back

    // insert - limit to extend?


    // copy
    virtual void copy(   const value*   fromb
                        ,const value*   frome
                        ,value*         to
                        ) = 0;

    // move
    virtual void move(   value* fromb
                        ,value* frome
                        ,value* to
                        ) = 0;


};


// FIXME: separate storage for builder and relation?

// column_storage base class to allows different high level
// views onto same storage
template<typename T>
struct column_storage_base
{
    // types

    typedef std::pmr::vector<T> vec_t;

    // FIXME: should probably take a shared pointer, no?
    column_storage_base( std::pmr::memory_resource* rsrc )
        : m_rsrc( rsrc ), m_vec( rsrc ) {
        assert( rsrc );
        //m_vec = make_container< std::pmr::vector< T > >( rsrc );
    }

    std::pmr::memory_resource*  m_rsrc;
    vec_t                       m_vec;
};

template<typename T>
struct untyped_column_storage : public IStorage
{
    untyped_column_storage( column_storage_base< T >* storage ) :
        m_storage( storage )
    {
    }

    // Convenience 
    static inline constexpr value* v( T* x ) noexcept
    {
        return reinterpret_cast<value*>(x);
    }

    static inline constexpr const value *cv( const T* x ) noexcept
    {
        return reinterpret_cast<const value*>(x);
    }

    static inline constexpr T* t( value * x ) noexcept
    {
        return reinterpret_cast<T*>(x);
    }

    static inline constexpr const T* ct( const value * x ) noexcept
    {
        return reinterpret_cast<const T*>(x);
    }


    // IStorage interface

    // FIXME: probably want to push all of these down to
    // column_storage_base to allow storage decisions at that
    // point

    const value* at( size_t idx ) const override
    {
        return cv( &( m_storage->m_vec.at( idx ) ) );
    }

    size_t size() const noexcept override
    {
        return m_storage->m_vec.size();
    }

    const value* cbegin() const noexcept override
    {
        return cv( m_storage->m_vec.data() );
    }

    const value* cend() const noexcept override
    {
        return cv( m_storage->m_vec.data() + m_storage->m_vec.size() );
    }


    value* at( size_t idx ) override
    {
        return v( &( m_storage->m_vec.at( idx ) ) );
    }

    value* begin() noexcept override
    {
        return v( m_storage->m_vec.data() );
    }

    value* end() noexcept  override
    {
        return v( m_storage->m_vec.data() + m_storage->m_vec.size() );
    }

    void resize( size_t sz ) override
    {
        m_storage->m_vec.resize( sz );
    }

    void copy(   const value *fromb
                ,const value *frome
                ,value       *to
    ) override
    {
        const T* fb = ct( fromb );
        const T* fe = ct( frome );
        T* to_      = t( to );
        // abstract out execution policy?
        // select based on size?
        if (false) {
            std::copy( std::execution::par_unseq, fb, fe, to_ );
        } else {
            std::copy( fb, fe, to_ );
        }
    }

    void move(   value  *fromb
                ,value  *frome
                ,value  *to
    ) override
    {
        T* fb   = t( fromb );
        T* fe   = t( frome );
        T* to_  = t( to );

        if (false) {
            std::move( std::execution::par_unseq, fb, fe, to_ );
        } else {
            //std::move( std::execution::unseq, fb, fe, to_ );
            std::move( fb, fe, to_ );
        }
    }

private:
    column_storage_base< T >* m_storage;
};


template<typename T>
struct column_storage : public column_storage_base<T>
{
    // types

    using typename column_storage_base< T >::vec_t;

    typedef vec_t::value_type       value_type;
    typedef vec_t::size_type        size_type;
    typedef vec_t::const_reference  const_reference;
    typedef vec_t::reference        reference;
    typedef vec_t::const_iterator   const_iterator;
    typedef vec_t::iterator         iterator;

    column_storage( std::pmr::memory_resource* rsrc ) :
        column_storage_base<T>( rsrc )
    {
    }


    // access elements

    constexpr const_iterator cbegin() const noexcept
    {
        return this->m_vec.cbegin();
    }

    constexpr const_iterator cend() const noexcept
    {
        return this->m_vec.cend();
    }

    constexpr const_reference at( size_type i ) const
    {
        return this->m_vec.at( i );
    }

    constexpr size_type size() const noexcept
    {
        return this->m_vec.size();
    }

    constexpr iterator begin() noexcept
    {
        return this->m_vec.begin();
    }

    constexpr iterator end() noexcept
    {
        return this->m_vec.end();
    }

    // add element(s)

};




struct relation_storage_t
{
    rel_ty_t m_ty;

    std::vector< std::pmr::monotonic_buffer_resource > m_resources;

};



// relation builder
struct relation_builder
{

};

// build up column or row-wise

// templated convenience functions for adding rows

#if 0
template<typename NI, typename TI>
relation_t create(
         NI nstart, NI nend // range of column names
        ,TI tstart, TI tend // range of column types
        ,

)
{
    throw not_implemented();
}
#endif


}