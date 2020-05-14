
#ifndef __RP_RECYCLE_H__
#define __RP_RECYCLE_H__

#include <assert.h>
#include <vector>

namespace rp {

template <typename T> class Recycle;

/**
 * Iterator<T>
 **/
template <typename T>
struct Iterator {
    typedef Iterator<T>    IteratorType;
    typedef Recycle<T>     HostType;

public:
    Iterator() : host(nullptr), index(0) {}
    Iterator( Iterator& other ) : 
        host(other.host), index(other.index) {}

public:
    bool Equal( const IteratorType& other ) const {
        return host == other.host && index == other.index;
    }
    bool operator ==( const IteratorType& other ) const { return Equal( other ); }
    bool operator !=( const IteratorType& other ) const { return !Equal( other ); }

public:
    bool Eof() const;
    void Next();

public:
    T& operator*();

public:
    void operator++(int) { index++; }

public:
    HostType* host;
    std::size_t index;
};

/**
 * Recycle<T>
 **/
template <typename T>
class Recycle {
public:
    typedef std::vector<T>  HolderType;
    typedef Iterator<T>     IteratorType;

public:
    explicit Recycle( const std::size_t& size ) : start_(0), end_(0) {
        holder_.resize( size );
    }

public:
    void FromBegin( IteratorType* it ) {
        assert( it != nullptr );

        it->host = this;
        it->index = start_;
    }

public:
    Error EraseUntil(const IteratorType& it) {
        start_ = it.index;
        return Error::OK;
    }

    Error Push( const T& data ) {
        std::size_t newEnd = (end_ + 1) % holder_.size();
        if ( newEnd == start_ ) {
            return Error::Full;
        }

        holder_[end_] = data;
        end_ = newEnd;
        return Error::OK;
    }

    Error Pop( T* data ) {
        if ( start_ == end_ ) {
            return Error::Empty;
        }

        *data = holder_[start_];
        start_ = (start_ + 1) % holder_.size();
        return Error::OK;
    }

public:
    T& Get( const std::size_t& index ) {
        return holder_[index];
    }
    const T& Get( const std::size_t& index ) const {
        return holder_[index];
    }

public:
    bool Empty() const {
        return start_ == end_;
    }
    bool Full() const {
        std::size_t newEnd = (end_ + 1) % holder_.size();
        return newEnd == start_;
    }

private:
    friend class Iterator<T>;

    HolderType  holder_;
    std::size_t start_, end_;
};

template <typename T>
T& Iterator<T>::operator*() {
    assert( host != nullptr );
    return host->Get(index);
}

template <typename T>
bool Iterator<T>::Eof() const {
    if ( host == nullptr ) { return true; }
    return index == host->end_;
}

template <typename T>
void Iterator<T>::Next() {
    assert( host != nullptr );
    index = (index + 1) % host->holder_.size();
}

}

#endif
