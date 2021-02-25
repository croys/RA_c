#pragma once

#include <memory_resource>
#include <vector>
#include <algorithm>
#include <execution>
#include <memory>

#include "base.h"
#include "types.h"

namespace rac
{

struct value_t{};

// untyped access to aligned, contiguous storage of monotyped values
//
// tempting to split out interfaces into pure and mutable, but all
// implementations will implement both, so we can just stick
// with const methods and use IStorage/const IStorage as appropriate
struct IStorage
{
    // immutable access
    virtual const value_t*  at( size_t idx ) const = 0;
    virtual size_t          size() const noexcept = 0;
    virtual const value_t*  cbegin() const noexcept = 0;
    virtual const value_t*  cend() const noexcept = 0;

    // mutable access
    virtual value_t*    at( size_t idx ) = 0;
    virtual value_t*    begin() noexcept = 0;
    virtual value_t*    end() noexcept = 0;

    // reserve
    virtual void reserve( size_t sz ) = 0;

    // resize
    virtual void resize( size_t sz ) = 0;

    // push_back
    virtual void push_back( const value_t* v ) = 0;

    // insert - limit to extend?


    // copy
    virtual void copy(   const value_t* fromb
                        ,const value_t* frome
                        ,value_t*       to
                        ) = 0;

    // move
    virtual void move(   value_t*   fromb
                        ,value_t*   frome
                        ,value_t*   to
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

    const value_t* cbegin() const noexcept override
    {
        // NOTE: can't reinterpret_cast iterator to T*
        return cv( m_storage->data() );
    }

    const value_t* cend() const noexcept override
    {
        // NOTE: can't reinterpret_cast iterator to T*
        return cv( m_storage->data() + m_storage->size() );
    }

    value_t* at( size_t idx ) override
    {
        return v( &( m_storage->at( idx ) ) );
    }

    value_t* begin() noexcept override
    {
        return v( m_storage->data() );
    }

    value_t* end() noexcept override
    {
        return v( m_storage->data() + m_storage->size() );
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

    void copy(   const value_t* fromb
                ,const value_t* frome
                ,value_t*       to
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

    void move(   value_t*   fromb
                ,value_t*   frome
                ,value_t*   to
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

    // really belongs in IColumn...
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

public:
    // IValue
    using typename IValue::storage_ptr_t;

    constexpr type_t type() const noexcept override
    {
        return val_t::type();
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


template<typename T, typename... Ts>
struct tys_folds
{
    static void collect_tys( std::vector<type_t>& tys )
    {
        tys.push_back( value_ops<T>::type() );
        tys_folds<Ts...>::collect_tys(tys);
    }

    static void collect_ops( std::vector<IValue*>& ops )
    {
        ops.push_back( untyped_value_ops<T>::ops() );
        tys_folds<Ts...>::collect_ops(ops);
    }

};

template<typename T>
struct tys_folds<T>
{
    static void collect_tys( std::vector<type_t>& tys )
    {
        tys.push_back( value_ops<T>::type() );
    }

    static void collect_ops( std::vector<IValue*>& ops )
    {
        ops.push_back( untyped_value_ops<T>::ops() );
    }
};


template<typename... Ts>
void collect_tys( std::vector< type_t >& tys )
{
    tys_folds<Ts...>::collect_tys( tys );
}


template<typename... Ts>
void collect_ops( std::vector<IValue*>& ops )
{
    tys_folds<Ts...>::collect_ops( ops );
}



struct relation_storage_t
{
    rel_ty_t m_ty;

    typedef std::shared_ptr<IStorage> storage_ptr_t;

    std::vector<storage_ptr_t> m_cols;
};


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
    explicit relation_builder(std::pmr::memory_resource* rsrc, Iter nb, Iter ne)
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
            m_rel_ty.push_back( std::make_pair(*ni, (*oi)->type() ) );
        }

        for (auto oi = m_ops.cbegin(); oi != m_ops.cend(); ++oi ) {
            // we could use memory directly allocated from monotonic_buffer_resource
            // instead of a std::vector
            //resource_ptr_t r = std::make_shared<std::pmr::monotonic_buffer_resource>( rsrc );
            resource_ptr_t r = std::make_shared<std::pmr::unsynchronized_pool_resource>( rsrc );
            m_resources.push_back( r );
            m_cols.push_back( (*oi)->make_storage( r.get() ) );
        }
    }

    constexpr const rel_ty_t& type() const noexcept
    {
        return m_rel_ty;
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


private:
    template<typename T, typename... Ts>
    struct at_helper
    {
        static constexpr std::tuple<T, Ts...> _at(
             const std::vector<IValue::storage_ptr_t>&  cols
            ,size_t                                     col
            ,size_t                                     idx
        )
        {
            return std::tuple_cat(
                std::tuple<T>( *(reinterpret_cast<const T*>( cols[col]->at(idx) ) ) ),
                at_helper<Ts...>::_at( cols, col+1, idx )
            );
        }
    };

    template<typename T>
    struct at_helper<T>
    {
        static constexpr std::tuple<T> _at(
             const std::vector<IValue::storage_ptr_t>&  cols
            ,size_t                                     col
            ,size_t                                     idx
        )
        {
            return std::tuple<T>( *(reinterpret_cast<const T*>( cols[col]->at(idx) ) ) );
        }
    };

public:
    constexpr std::tuple<Types...> at( size_t idx ) const
    {
        return at_helper<Types...>::_at( m_cols, 0, idx );
    }

    rel_ty_t                            m_rel_ty;
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



// relations themselves
// want iteration over rows and std::get<> to allow
// interop with std contaiers

// tableviews


// relation operations


}