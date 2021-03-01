#pragma once

#include <memory_resource>
#include <vector>
#include <algorithm>
#include <execution>
#include <memory>
#include <ostream>
#include <sstream>

#include "base.h"
#include "types.h"

namespace rac
{

struct value_t{};


// Note: we implement as much of the stdlib iteratos as makes sense
// As we are abstracting over variable sized storage of unknown type
// we cannot provide anything where the type leaks
// - no dereferencing, so std::copy and std::move cannot work with
// these iterators
struct const_value_iterator
{
    //typedef const value_t*  value_type;
    typedef int64_t         difference_type;
    //typedef const value_t*  reference_type;

    const_value_iterator( const value_t* ptr, size_t size )
        : m_ptr( ptr ), m_size( size )
    {}


    // Iterator

    // operator* purposefully left out
    const_value_iterator& operator++() noexcept
    {
        m_ptr += m_size;
        return *this;
    }

    // ForwardIterator
    const_value_iterator operator++(int) noexcept // post increment
    {
        return const_value_iterator( m_ptr + m_size, m_size );
    }

    // BidirectionalIterator

    const_value_iterator& operator--() noexcept
    {
        m_ptr -= m_size;
        return *this;
    }

    const_value_iterator operator--(int) noexcept // post-decrement
    {
        return const_value_iterator( m_ptr - m_size, m_size );
    }

    // RandomAccessIterator
#if 0
    constexpr reference_type operator[](size_t idx) noexcept
    {
        return m_ptr + ( idx * m_size );
    }
#endif

    constexpr const_value_iterator& operator+=(size_t n) noexcept
    {
        m_ptr += n * m_size;
        return *this;
    }

    constexpr const_value_iterator& operator-=(size_t n) noexcept
    {
        m_ptr -= n * m_size;
        return *this;
    }

    const value_t* get() const noexcept
    {
        return m_ptr;
    }

    const value_t*  m_ptr;
    size_t          m_size;
};


const_value_iterator operator+(
     const const_value_iterator&            it
    ,const_value_iterator::difference_type  d
) noexcept {
    return const_value_iterator( it.m_ptr + d * it.m_size, it.m_size );
}

const_value_iterator operator+(
     const_value_iterator::difference_type  d
    ,const const_value_iterator&            it
) noexcept {
    return const_value_iterator( it.m_ptr + d * it.m_size, it.m_size );
}

const_value_iterator::difference_type operator-(
     const const_value_iterator& a
    ,const const_value_iterator& b
) noexcept {
    return ( a.m_ptr - b.m_ptr ) / a.m_size;
}


constexpr bool operator==(const const_value_iterator& a, const const_value_iterator& b)
{
    return a.m_ptr == b.m_ptr;
}

constexpr auto operator<=>(const const_value_iterator& a, const const_value_iterator& b)
{
    return a.m_ptr <=> b.m_ptr;
}



struct value_iterator
{
    typedef int64_t         difference_type;

    value_iterator( value_t* ptr, size_t size )
        : m_ptr( ptr ), m_size( size )
    {}

    // Iterator

    // operator* purposefully left out
    value_iterator& operator++() noexcept
    {
        m_ptr += m_size;
        return *this;
    }

    // ForwardIterator
    value_iterator operator++(int) noexcept // post increment
    {
        return value_iterator( m_ptr + m_size, m_size );
    }

    // BidirectionalIterator

    value_iterator& operator--() noexcept
    {
        m_ptr -= m_size;
        return *this;
    }

    value_iterator operator--(int) noexcept // post-decrement
    {
        return value_iterator( m_ptr - m_size, m_size );
    }

    // RandomAccessIterator

    // operator[] purposefully left out

    constexpr value_iterator& operator+=( size_t n ) noexcept
    {
        m_ptr += n * m_size;
        return *this;
    }

    constexpr value_iterator& operator-=( size_t n ) noexcept
    {
        m_ptr -= n * m_size;
        return *this;
    }

    value_t* get() const noexcept
    {
        return m_ptr;
    }

    value_t*    m_ptr;
    size_t      m_size;
};


value_iterator operator+(
     const value_iterator&              it
    ,value_iterator::difference_type    d
) noexcept {
    return value_iterator( it.m_ptr + d * it.m_size, it.m_size );
}

value_iterator operator+(
     value_iterator::difference_type    d
    ,const value_iterator&              it
) noexcept {
    return value_iterator( it.m_ptr + d * it.m_size, it.m_size );
}

value_iterator::difference_type operator-(
     const value_iterator& a
    ,const value_iterator& b
) noexcept {
    return ( a.m_ptr - b.m_ptr ) / a.m_size;
}


constexpr bool operator==(const value_iterator& a, const value_iterator& b)
{
    return a.m_ptr == b.m_ptr;
}

constexpr auto operator<=>(const value_iterator& a, const value_iterator& b)
{
    return a.m_ptr <=> b.m_ptr;
}




// untyped access to aligned, contiguous storage of monotyped values
//
// tempting to split out interfaces into pure and mutable, but all
// implementations will implement both, so we can just stick
// with const methods and use IStorage/const IStorage as appropriate
struct IStorage
{
    typedef const_value_iterator    const_iterator;
    typedef value_iterator          iterator;

    // immutable access
    virtual const value_t*  at( size_t idx ) const = 0;
    virtual size_t          size() const noexcept = 0;
    virtual const_iterator  cbegin() const noexcept = 0;
    virtual const_iterator  cend() const noexcept = 0;

    // mutable access
    virtual value_t*        at( size_t idx ) = 0;
    virtual iterator        begin() noexcept = 0;
    virtual iterator        end() noexcept = 0;

    // reserve
    virtual void reserve( size_t sz ) = 0;

    // resize
    virtual void resize( size_t sz ) = 0;

    // push_back
    virtual void push_back( const value_t* v ) = 0;

    // insert - limit to extend?


    // copy
    virtual void copy(   const const_iterator&  fromb
                        ,const const_iterator&  frome
                        ,iterator               to
                        ) = 0;

    // move
    virtual void move(   iterator  fromb
                        ,iterator  frome
                        ,iterator  to
                        ) = 0;

};

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

template<typename T>
struct column_storage_base
{
    // types

    typedef std::pmr::vector<T> vec_t;

    typedef vec_t::value_type       value_type;
    typedef vec_t::size_type        size_type;
    typedef vec_t::const_reference  const_reference;
    typedef vec_t::reference        reference;
    typedef vec_t::const_iterator   const_iterator;
    typedef vec_t::iterator         iterator;

    //typedef std::shared_ptr<std::pmr::memory_resource> resource_ptr_t;
    typedef std::pmr::memory_resource* resource_ptr_t;

    explicit column_storage_base( resource_ptr_t rsrc )
        : m_rsrc( rsrc ), m_vec( rsrc ) {
        assert( rsrc );
    }

    // immutable deconstruction

    constexpr bool empty() const noexcept
    {
        return this->m_vec.empty();
    }

    constexpr size_type size() const noexcept
    {
        return this->m_vec.size();
    }

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

    constexpr const_reference operator[]( size_type i ) const
    {
        return this->m_vec[i];
    }

    constexpr const T* data() const noexcept
    {
        return this->m_vec.data();
    }

    // mutation

    constexpr iterator begin() noexcept
    {
        return this->m_vec.begin();
    }

    constexpr iterator end() noexcept
    {
        return this->m_vec.end();
    }

    constexpr reference at( size_type i )
    {
        return this->m_vec.at( i );
    }

    constexpr reference operator[]( size_type i )
    {
        return this->m_vec[i];
    }

    constexpr T* data() noexcept
    {
        return this->m_vec.data();
    }

    constexpr void reserve( size_t sz )
    {
        this->m_vec.reserve( sz );
    }

    constexpr void resize( size_type sz )
    {
         this->m_vec.resize( sz );
    }

    constexpr void push_back( const T& v )
    {
        this->m_vec.push_back( v );
    }

    constexpr void push_back( const T&& v )
    {
        this->m_vec.push_back( v );
    }


protected:
    resource_ptr_t  m_rsrc;
    vec_t           m_vec;
};


template<typename T>
struct column_storage : public column_storage_base< T >
{
    using typename column_storage_base< T >::resource_ptr_t;

    explicit column_storage( resource_ptr_t rsrc )
        : column_storage_base< T >( rsrc )
    {
    }
};


template<typename T>
struct untyped_column_storage : public IStorage
{
    typedef std::shared_ptr< column_storage< T > > storage_ptr_t;
    //typedef column_storage< T >* storage_ptr_t;

    explicit untyped_column_storage( storage_ptr_t storage ) :
        m_storage( storage )
    {
    }

private:

    // Convenience
    static constexpr value_t* v( T* x ) noexcept
    {
        return reinterpret_cast<value_t*>(x);
    }

    static constexpr const value_t* cv( const T* x ) noexcept
    {
        return reinterpret_cast<const value_t*>(x);
    }

    static constexpr T* t( value_t * x ) noexcept
    {
        return reinterpret_cast<T*>(x);
    }

    static constexpr const T* ct( const value_t  * x ) noexcept
    {
        return reinterpret_cast<const T*>(x);
    }

public:

    // IStorage interface

    const value_t* at( size_t idx ) const override
    {
        return cv( &( m_storage->at( idx ) ) );
    }

    size_t size() const noexcept override
    {
        return m_storage->size();
    }

    const_iterator cbegin() const noexcept override
    {
        return const_value_iterator( cv( m_storage->data() ), sizeof( T ) ); // FIXME: sizeof?
    }

    const_iterator cend() const noexcept override
    {
        return const_value_iterator( cv( m_storage->data() + m_storage->size() ), sizeof( T ) ); // FIXME: sizeof?
    }

    value_t* at( size_t idx ) override
    {
        return v( &( m_storage->at( idx ) ) );
    }


    iterator begin() noexcept override
    {
        return value_iterator( v( m_storage->data() ), sizeof( T ) ); // FIXME: sizeof?
    }

    iterator end() noexcept override
    {
        return value_iterator( v( m_storage->data() + m_storage->size() ), sizeof( T ) ); // FIXME: sizeof
    }

    void reserve( size_t sz ) override
    {
        m_storage->reserve( sz );
    }

    void resize( size_t sz ) override
    {
        m_storage->resize( sz );
    }

    void push_back( const value_t* v ) override
    {
        const T* v_ = ct( v );
        m_storage->push_back( *v_ );
    }

    void copy(   const const_iterator&  fromb
                ,const const_iterator&  frome
                ,iterator               to
    ) override
    {
        const T* fb = ct( fromb.get() );
        const T* fe = ct( frome.get() );
        T* to_      = t( to.get() );
        // abstract out execution policy?
        // select based on size?
        if (false) {
            std::copy( std::execution::par_unseq, fb, fe, to_ );
        } else {
            std::copy( fb, fe, to_ );
        }
    }

    void move(   iterator   fromb
                ,iterator   frome
                ,iterator   to
    ) override
    {
        T* fb   = t( fromb.get() );
        T* fe   = t( frome.get() );
        T* to_  = t( to.get() );

        if (false) {
            std::move( std::execution::par_unseq, fb, fe, to_ );
        } else {
            //std::move( std::execution::unseq, fb, fe, to_ );
            std::move( fb, fe, to_ );
        }
    }

private:
    storage_ptr_t m_storage;
};



template<typename T>
struct value_ops_base
{
    typedef std::shared_ptr<IStorage> storage_ptr_t;

    static constexpr storage_ptr_t make_storage( std::pmr::memory_resource* rsrc )
    {
        // FIXME: use another memory resource for the objects themselves
        auto s = std::make_shared< column_storage< T > >( rsrc );
        auto us =  std::make_shared< untyped_column_storage< T > >( s );
        return us;
    }

};


template<typename T>
struct value_ops : public value_ops_base< T >
{
};

// FIXME: std::vector<bool> is specialised, so this needs to be too...
template<>
struct value_ops<bool> : public value_ops_base<bool>
{
    static constexpr const type_t type() noexcept {
        return type_t( { Bool } );
    }
};

template<>
struct value_ops<int> : public value_ops_base<int>
{
    static constexpr const type_t type() noexcept {
        return type_t( { Int } );
    }

};

template<>
struct value_ops<float> : public value_ops_base<float>
{
    static constexpr const type_t type() noexcept {
        return type_t( { Float } );
    }

};

template<>
struct value_ops<double> : public value_ops_base<double>
{
    static constexpr const type_t type() noexcept {
        return type_t( { Double } );
    }

};






// IValue - Dynamically/monotyped value operations
// Arguably could be merged with IStorage, though later we will have
// comparison operations, etc...
struct IValue
{
    virtual constexpr type_t type() const noexcept = 0;

    virtual std::ostream& to_stream( const value_t* v, std::ostream& os ) const = 0;

    // really belongs in IColumn/IColumnBuilder/etc...
    typedef std::shared_ptr<IStorage> storage_ptr_t;

    virtual constexpr storage_ptr_t make_storage(
        std::pmr::memory_resource* rsrc
    ) const = 0;

};



template<typename T>
struct untyped_value_ops : public IValue
{
private:
    typedef value_ops<T> val_t;

    // Convenience
    static constexpr value_t* v( T* x ) noexcept
    {
        return reinterpret_cast<value_t*>(x);
    }

    static constexpr const value_t* cv( const T* x ) noexcept
    {
        return reinterpret_cast<const value_t*>(x);
    }

    static constexpr T* t( value_t * x ) noexcept
    {
        return reinterpret_cast<T*>(x);
    }

    static constexpr const T* ct( const value_t  * x ) noexcept
    {
        return reinterpret_cast<const T*>(x);
    }


public:
    // IValue
    using typename IValue::storage_ptr_t;

    constexpr type_t type() const noexcept override
    {
        return val_t::type();
    }

    //
    std::ostream& to_stream( const value_t* v, std::ostream& os ) const override
    {
        return os << *ct( v );
    }

    constexpr storage_ptr_t make_storage(
        std::pmr::memory_resource* rsrc
    ) const override
    {
        return val_t::make_storage( rsrc );
    }

    // Note that multiple instantiations will result in multiple statics.
    // We don't care, as this class has no storage, and all instances are canonical.
    // Also, use of this function should be limited.
    static IValue* ops() noexcept
    {
        static untyped_value_ops<T> ops;
        return &ops;
    }

};

// FIXME:
struct column_storage_t
{
    rel_ty_t m_ty;

    typedef std::shared_ptr<IStorage> storage_ptr_t;

    std::vector<storage_ptr_t> m_cols;
};

// Typed operations over multiple columns
// FIXME: these are just folds
template<typename T, typename... Ts>
struct col_helper
{
    static void collect_ops( std::vector<IValue*>& ops )
    {
        ops.push_back( untyped_value_ops<T>::ops() );
        col_helper<Ts...>::collect_ops(ops);
    }

    static constexpr std::tuple<T, Ts...> row(
         const std::vector<IValue::storage_ptr_t>&  cols
        ,size_t                                     col
        ,size_t                                     row
    )
    {
        return std::tuple_cat(
            std::tuple<T>( *(reinterpret_cast<const T*>( cols[ col ]->at( row ) ) ) ),
            col_helper<Ts...>::row( cols, col + 1, row )
        );
    }
};


template<typename T>
struct col_helper<T>
{
    static void collect_ops( std::vector<IValue*>& ops )
    {
        ops.push_back( untyped_value_ops<T>::ops() );
    }

    static constexpr std::tuple<T> row(
         const std::vector<IValue::storage_ptr_t>&  cols
        ,size_t                                     col
        ,size_t                                     row
    )
    {
        return std::tuple<T>( *(reinterpret_cast<const T*>( cols[ col ]->at( row ) ) ) );
    }
};

template<typename... Ts>
void collect_ops( std::vector<IValue*>& ops )
{
    col_helper<Ts...>::collect_ops( ops );
}



std::ostream& cols_to_stream(
     std::ostream&                              os
    ,const col_tys_t&                           col_tys
    ,const std::vector<IValue*>&                ops
    ,const std::vector<IValue::storage_ptr_t>&  cols
)
{
    const size_t n_cols = cols.size();
    const size_t n_rows = cols[ 0 ]->size();

    // Work out column widths
    std::vector<size_t> col_sizes( n_cols );
    for ( size_t c = 0; c < n_cols; ++c )
    {
        col_sizes[ c ] = col_tys[ c ].first.size();
    }

    std::ostringstream ss;
    size_t total_width = 0;
    for ( size_t c = 0; c < n_cols; ++c )
    {
        size_t m = col_sizes[ c ];
        for (auto it = cols[ c ]->cbegin(); it != cols[ c ]->cend(); ++it )
        {
            ss.str("");
            ops[ c ]->to_stream( it.get(), ss );
            m = std::max( size_t( ss.tellp() ), m );
        }
        col_sizes[ c ] = m;

        if (c > 0) {
            os << "  ";
            total_width += 2;
        }
        ss.str("");
        ss.width( m );
        ss << col_tys[ c ].first;
        ss.width( 0 );
        os << ss.str();
        total_width += m;
    }
    os << "\n";

    for ( size_t i=0; i < total_width; ++i )
    {
        os << "-";
    }
    os << "\n";

    // values

    for ( size_t r = 0; r < n_rows; ++r )
    {
        for( size_t c = 0; c < n_cols; ++c )
        {
            if (c > 0) {
                os << "  ";
            }
            ss.str("");
            ss.width( col_sizes[ c ] );
            ops[ c ]->to_stream( cols[ c ]->at( r ), ss );
            ss.width(0);
            os << ss.str();
        }
        os << "\n";
    }

    return os;
}



// relation builder
//
// Design considerations - the only reason we are using dynamically typed
// storage here is so we can pass ownership to a relation later.
// We could keep this fully statically typed and only create
// IStorage instances when the relation is created....
//
// Abstract over resource type?
template< typename... Types >
struct relation_builder
{
private:
    typedef std::shared_ptr<std::pmr::memory_resource> resource_ptr_t;

public:
    // FIXME: we probably should take ownership of the resource
    template<typename Iter>
    explicit relation_builder( std::pmr::memory_resource* rsrc, Iter nb, Iter ne )
    {
        collect_ops<Types...>(m_ops);

        // FIXME: refactor - extract out
        if ( ne - nb != m_ops.size() ) {
            throw_with< std::invalid_argument >(
                std::ostringstream()
                << "Size of column names and types do not match, "
                << "names: " << ne-nb << ", types: " << m_ops.size()
            );
        }
        Iter ni = nb;
        for (auto oi = m_ops.cbegin(); ni < ne; ++ni, ++oi) {
            m_col_tys.push_back( std::make_pair(*ni, (*oi)->type() ) );
        }

        for (const auto& op : m_ops ) {
            // we could use memory directly allocated from monotonic_buffer_resource
            // instead of a std::vector
            //resource_ptr_t r = std::make_shared<std::pmr::monotonic_buffer_resource>( rsrc );
            resource_ptr_t r  =
                std::make_shared<std::pmr::unsynchronized_pool_resource>( rsrc );
            m_cols.emplace_back( op->make_storage( r.get() ) );
            m_resources.emplace_back( std::move( r ) );
        }
    }

    constexpr const col_tys_t& type() const noexcept
    {
        return m_col_tys;
    }

    size_t constexpr size() const noexcept
    {
        return m_cols.size() > 0 ? m_cols[0]->size() : 0;
    }


    // push_back
private:
    template<typename T>
    constexpr void _push_back( size_t col, const T& v )
    {
        this->m_cols[col]->push_back( reinterpret_cast<const value_t*>( &v ) );
    }

    template<typename T, typename... Ts>
    constexpr void _push_back( size_t col, const T& v, Ts... vs )
    {
        this->m_cols[col]->push_back( reinterpret_cast<const value_t*>( &v ) );
        this->_push_back( col + 1, vs...);
    }
public:
    constexpr void push_back( Types... vs )
    {
        this->_push_back( 0, vs... );
    }

#if 0
    constexpr void push_back( const std::tuple<Types...>& v )
    {
    }
#endif
#if 0
    constexpr void push_back( const std::tuple<Types...>&& v )
    {
    }
#endif

    // FIXME: emplace_back


public:
    constexpr std::tuple<Types...> at( size_t idx ) const
    {
        return col_helper<Types...>::row( m_cols, 0, idx );
    }

    std::ostream& dump( std::ostream& os ) const
    {
        // dump type

        os << "relation_builder ";
        col_tys_to_stream( os, m_col_tys );
        os << "\n\n";

        return cols_to_stream( os, m_col_tys, m_ops, m_cols );
    }

    col_tys_t                           m_col_tys;
    // FIXME: combine following into tuple?
    std::vector<IValue*>                m_ops;
    std::vector<resource_ptr_t>         m_resources;
    std::vector<IValue::storage_ptr_t>  m_cols;

    // FIXME: move this over to being fully statically typed
    // use a tuple of shared_ptr to each column_storage class
    // can then construct IValue for each and pass off to relation
    // if we just add a typed_storage() method to untyped_column_storage,
    // then we can just keep untype_column_storage instances, and
    // use these to build the relation...
};


// relations - monotyped
struct relation
{

};
// - relation columns are unordered
// - do we want to guarantee ordering of rows on first key?
// - do keys belong in relation type, or just to relations?


// relation operations

// table views

// table_view - monotyped view onto a relation
// In table_view columns and rows have ordering

// table_view_t - statically typed view onto a relation
// In table_view_t columns and rows have ordering

// want iteration over rows and std::get<> to allow
// interop with std contaiers


}