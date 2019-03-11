/* class OnewayBitset
   Author: Jan Kessler (2019)

   Some inspiration came from:
   1) https://www.hackerearth.com/practice/notes/bit-manipulation/
   2) https://stackoverflow.com/a/47990
   3) https://stackoverflow.com/a/26230537
*/

#ifndef ONEWAY_BITSET_HPP
#define ONEWAY_BITSET_HPP

#include <type_traits>
#include <limits>
#include <climits>
#include <algorithm>
#include <iostream>

template <
    typename SizeT, /* integral index type (e.g. int or size_t) */
    typename AllocT /* integral memory type, defines blocksize (e.g. uint8_t or uint64_t) */
    >
struct OnewayBitset
// A runtime-sized bitset class, specialized for the case that you never want
// to set single bits back 0 again, but instead want to set single bits to 1 in
// an iterative fashion and then periodically evaluate and reset the entire bitset.
// This restriction allows for slightly simpler/less code and allows storing two
// flag to answer "is any bit set?", with virtually zero overhead for keeping the
// flag up-to-date. The practical usefulness of this is debatable though ;-)
//
// This work was mostly done for my own educational purpose, but the result appears
// to be something that could be useful in practice, when the use case is very fitting.
// The implementation is reasonably tested, stable and fast, handling bitfields as large
// as your memory allows (and your chosen SizeT). But notice that in general there are
// no bounds checks on indices, so please make sure that you don't get/set beyond the
// size nbits. Merging bitfields of different size remains without effect. That said,
// the class should be relatively self-documenting and easy to use.
//
// Performance:
// Compared to the std::vector<bool> specialization (a dynamic and resizable bitfield),
// in my tests I get typically equal or slightly better performance, but the differences
// vary with all relevant variables (system, compiler+options, chosen types, benchmark etc.)
// in a somewhat unpredictable way. Anyway, I'm pleased with the results.
{
    // --- Static Asserts
    static_assert(std::is_integral<SizeT>::value, "SizeT must be integral type.");
    static_assert(std::is_integral<AllocT>::value && std::is_unsigned<AllocT>::value, "AllocT must be unsigned integral type.");
    static_assert(std::numeric_limits<SizeT>::max() >= sizeof(AllocT)*CHAR_BIT, "SizeT must be able to represent number of bits of AllocT."); // I don't think this will ever fail ;-)

    // --- Compile-Time Statics
    static constexpr AllocT blocksize = static_cast<AllocT>( sizeof(AllocT)*CHAR_BIT ); // safe, every integer is capable of representing own size in bits
    static constexpr AllocT alloct_one = 1; // block of alloc type, least bit 1
    static constexpr AllocT alloct_zero = 0; // block of alloc type, all bits 0
    static constexpr AllocT alloct_all = ~(alloct_zero); // block of alloc type, all bits 1

private:
    // these are const unless you use assignment operators
    SizeT _nbits; // number of bits (without padding)
    SizeT _nblocks; // number of memory blocks of sizeof(AllocT) byte
    AllocT _padblk; // _padblk has all bits 1, except for the padded bits of the last block

    // variables
    AllocT * _blocks; // ptr to first block of the bitfield
    bool _flag_zero;

public:
    // --- Constructors/Destructor

    explicit OnewayBitset(const SizeT n_bits = 0): // we do sanity checks only here (to set proper empty state)
        _nbits(n_bits > 0 ? n_bits : 0), _nblocks(_nbits > 0 ? (_nbits-1)/blocksize + 1 : 0),
        _padblk(~( alloct_all << static_cast<AllocT>(_nbits%blocksize) )),
        _blocks(_nbits > 0 ? new AllocT[_nblocks] : nullptr), _flag_zero(true)
    {
        reset();
    }

    OnewayBitset(const OnewayBitset &other):
        _nbits(other._nbits), _nblocks(other._nblocks), _padblk(other._padblk),
        _blocks(new AllocT[_nblocks]), _flag_zero(other._flag_zero)
    {
        std::copy(other._blocks, other._blocks+_nblocks, _blocks);
    }

    ~OnewayBitset(){ delete [] _blocks; }


    // --- Canonical copy / move assignment

    OnewayBitset& operator=(const OnewayBitset &other) // copy assignment
    {
        if (this != &other) { // self-assignment check
            if (_nbits != other._nbits) { // we need to change constants
                if (_nblocks != other._nblocks) { // we need to reallocate
                    delete[] _blocks;
                    _blocks = new AllocT[other._nblocks];
                    _nblocks = other._nblocks;
                }
                // copy constants
                _nbits = other._nbits;
                _padblk = other._padblk;
            }
            // copy data
            std::copy(other._blocks, other._blocks+_nblocks, _blocks);
            _flag_zero = other._flag_zero;
        }
        return *this;
    }

    OnewayBitset& operator=(OnewayBitset &&other) noexcept // move assignment
    {
        if(this != &other) { // self-move-assignment check
            // move from other (rvalue) and set other to empty state
            _nbits = std::exchange(other._nbits, 0);
            _nblocks = std::exchange(other._nblocks, 0);
            _padblk = std::exchange(other._padblk, 0);

            delete[] _blocks;
            _blocks = std::exchange(other._blocks, nullptr);
            _flag_zero = std::exchange(other._flag_zero, true);
        }
        return *this;
    }


    // --- Overloaded Operators (using methods from below)

    OnewayBitset& operator+=(const OnewayBitset &other) // plus assignment (merge)
    {
        this->merge(other);
        return *this;
    }

    friend OnewayBitset operator+(OnewayBitset lhs, const OnewayBitset &rhs) // plus (merged copy)
    {
        lhs += rhs; // use merge on lhs copy
        return lhs;
    }

    friend bool operator==(const OnewayBitset& lhs, const OnewayBitset& rhs){ return lhs.equals(rhs); }
    friend bool operator!=(const OnewayBitset& lhs, const OnewayBitset& rhs){ return !(lhs.equals(rhs)); }

    // --- Getters

    SizeT getNBits() const { return _nbits; }
    SizeT getNBlocks() const { return _nblocks; }
    AllocT getPadBlock() const { return _padblk; }
    const AllocT * const getBlocks() const { return _blocks; }


    // --- Methods involving this bitfield

    void reset() { // reset to 0/false
        std::fill(_blocks, _blocks+_nblocks, alloct_zero);
        _flag_zero = true;
    }

    void set(SizeT index) // set the single bit via scalar index
    {   // pass 0<=index<_nbits
        const SizeT blockIndex = index / blocksize;
        const SizeT bitIndex = index % blocksize;
        _blocks[blockIndex] |= (alloct_one << bitIndex);
        _flag_zero = false;
    }

    void set(SizeT blockIndex, SizeT bitIndex) // set the single bit via to tuple index
    {    // pass 0<=blkidx<_nblocks, 0<=bitidx<blocksize
        _blocks[blockIndex] |= (alloct_one << static_cast<AllocT>(bitIndex));
        _flag_zero = false;
    }

    void setAll() { // fast way to set all bits 1
        std::fill(_blocks, _blocks+_nblocks-1, alloct_all);
        _blocks[_nblocks-1] = _padblk; // to make sure the padding bits are 0
        _flag_zero = false;
    }

    bool get(SizeT index) const // get bit via scalar index
    {   // pass 0<=index<_nbits
        const SizeT blockIndex = index / blocksize;
        const AllocT bitIndex = index % blocksize;
        return ( (_blocks[blockIndex] >> bitIndex) & alloct_one );
    }

    bool get(SizeT blockIndex, SizeT bitIndex) const // get bit via tuple index
    {   // pass 0<=blkidx<_nblocks, 0<=bitidx<blocksize
        return ( (_blocks[blockIndex] >> static_cast<AllocT>(bitIndex)) & alloct_one );
    }

    void getAll(bool out[] /*out[_nbits]*/) const // get all, to fill an ordinary bool array
    {
        std::fill(out, out+_nbits, false); // this should be fast and usually worth it ..
        if (_flag_zero) return;

        SizeT blkoff = 0;
        for (SizeT blkidx=0; blkidx<_nblocks; ++blkidx, blkoff+=blocksize) {
            AllocT blkval = _blocks[blkidx];
            SizeT bitidx = blkoff;
            while ( blkval ) { // .. because then we can stop early here
                out[bitidx] = blkval & alloct_one;
                blkval >>= alloct_one;
                ++bitidx;
            }
        }
    }

    bool empty() const { return (_nbits == 0); }

    bool any() const { return !_flag_zero; }

    bool none() const { return _flag_zero; }

    bool all() const
    {
        if (_flag_zero) { return false; }
        SizeT blkidx;
        for (blkidx=0; blkidx<_nblocks-1; ++blkidx) {
            if (_blocks[blkidx] != alloct_all) {
                return false;
            }
        }
        return ( _blocks[blkidx] == _padblk );
    }

    SizeT count() const // returns number of true bits in worst-case log(N) time
    {
        if (_flag_zero) { return 0; }
        SizeT count = 0;
        for (SizeT blkidx=0; blkidx<_nblocks; ++blkidx) {
            AllocT blkval = _blocks[blkidx];
            while( blkval ) {
                blkval = blkval & (blkval-alloct_one);
                ++count;
            }
        }
        return count;
    }

    // Methods involving this and other bitfield

    void merge(const OnewayBitset &other) // set this = this | other
    {
        if (_nbits!=other._nbits) { return; }
        for (SizeT i=0; i<_nblocks; ++i) {
            _blocks[i] |= other._blocks[i];
        }
        _flag_zero = (_flag_zero && other._flag_zero);
    }

    bool equals(const OnewayBitset &other) const // return this == other
    {
        if (_nbits!=other._nbits) { return false; }
        return std::equal(_blocks, _blocks+_nblocks, other._blocks);
    }
};


#endif
