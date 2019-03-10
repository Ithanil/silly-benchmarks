/* class OnewayBitfield
   Author: Jan Kessler (2019)

   Some inspiration came from:
   1) https://www.hackerearth.com/practice/notes/bit-manipulation/
   2) https://stackoverflow.com/a/47990
   3) https://stackoverflow.com/a/26230537
*/

#ifndef ONEWAY_BITFIELD_HPP
#define ONEWAY_BITFIELD_HPP

#include <climits>
#include <algorithm>

template <
    typename SizeT, /* integral index type (e.g. int or size_t) */
    typename AllocT /* integral memory type (e.g. char) */
    >
struct OnewayBitfield
// A minimalistic (runtime-)const-sized bitfield class, which starts with all
// bits = 0 and then accumulates changes of bits to 1, until the field gets
// evaluated and reset to 0 again. Therefore we can answer any() directly
// via a flag. Also it reduces the number of required methods and may be just as
// much as is needed for some use cases.
// Also note that except for the bit values and the any flag, this class has only
// public constant members. This prevents copy-assignment, but again reduces
// complexity.
// After all, when compared to the std::vector<bool> specialization (a dynamic
// and resizable bitfield), in my tests I get similar or slightly better
// performance (with -O3 -flto -march=native) and as a bonus, the any method comes
// without cost in this bitfield.
// The implementation is unit-tested and is stable, handling bitfields as large as
// you memory allows (and your chosen SizeT). But notice that in general there are
// no bounds checks on indices, so please make sure that you don't get/set beyond
// the size nbits and that you don't merge/compare bitfields of different size.
{
public:
    // --- Compile-Time Statics
    static constexpr AllocT blocksize = static_cast<AllocT>( sizeof(AllocT)*CHAR_BIT ); // safe, every integer is capable of representing own size in bits
    static constexpr AllocT alloct_one = 1; // block of alloc type, least bit 1
    static constexpr AllocT alloct_zero = 0; // block of alloc type, all bits 0
    static constexpr AllocT alloct_all = ~(alloct_zero); // block of alloc type, all bits 1

    // --- Runtime consts
    const SizeT nbits; // number of bits (without padding)
    const SizeT nblocks; // number of memory blocks of sizeof(AllocT) byte
    const AllocT nrest; // number of (relevant) bits in the padded block (0 if no padded block)
    const AllocT padblk; // padblk is all bits 1 except padding bits for last block

private:
    AllocT * const _blocks; // ptr to first block of the bitfield
    bool _flag_zero;

public:
    // --- Constructors/Destructor

    explicit OnewayBitfield(const SizeT nbits):
        nbits(nbits), nblocks((nbits-1)/blocksize + 1), nrest(nbits%blocksize),
        padblk(~(alloct_all << nrest)), _blocks(new AllocT[nblocks]), _flag_zero(true)
    {
        this->reset();
    }

    OnewayBitfield(const OnewayBitfield &other):
        nbits(other.nbits), nblocks(other.nblocks), nrest(other.nrest),
        padblk(other.padblk), _blocks(new AllocT[nblocks]), _flag_zero(other._flag_zero)
    {
        std::copy(other._blocks, other._blocks+nblocks, _blocks);
    }

    ~OnewayBitfield(){ delete [] _blocks; }


    // --- Methods involving this bitfield

    void reset() { // reset to 0/false
        std::fill(_blocks, _blocks+nblocks, alloct_zero);
        _flag_zero = true;
    }

    void set(SizeT index) // set the single bit corresponding to index (to 1)
    {
        const SizeT blockIndex = index / blocksize;
        const AllocT bitIndex = index % blocksize;
        // set single bit
        _blocks[blockIndex] |= (alloct_one << bitIndex);
        _flag_zero = false;
    }

    void setAll() { // fast way to set all bits 1
        std::fill(_blocks, _blocks+nblocks-1, alloct_all);
        _blocks[nblocks-1] = padblk; // to make sure the padding bits are 0
        _flag_zero = false;
    }

    bool get(SizeT index) const // element-wise get
    {
        if (_flag_zero) return false;
        const SizeT blockIndex = index / blocksize;
        const AllocT bitIndex = index % blocksize;
        // get single bit
        return ( (_blocks[blockIndex] >> bitIndex) & alloct_one );
    }

    void getAll(bool out[] /*out[nbits]*/) const // get all
    {
        std::fill(out, out+nbits, false); // this should be fast and worth it in many cases
        if (_flag_zero) return;

        SizeT blkoff = 0;
        for (SizeT blkidx=0; blkidx<nblocks; ++blkidx, blkoff+=blocksize) {
            AllocT blkval = _blocks[blkidx];
            SizeT bitidx = blkoff;
            while ( blkval ) { // because then we can stop early here
                out[bitidx] = blkval & alloct_one;
                blkval >>= alloct_one;
                ++bitidx;
            }
        }
    }

    bool any() const { return !_flag_zero; }

    bool none() const { return _flag_zero; }

    bool all() const
    {
        if (_flag_zero) { return false; }
        SizeT blkidx;
        for (blkidx=0; blkidx<nblocks-1; ++blkidx) {
            if (_blocks[blkidx] != alloct_all) {
                return false;
            }
        }
        return ( _blocks[blkidx] == padblk );
    }

    SizeT count() const // returns number of true bits in worst-case log(N) time
    {
        if (_flag_zero) { return 0; }
        SizeT count = 0;
        for (SizeT blkidx=0; blkidx<nblocks; ++blkidx) {
            AllocT blkval = _blocks[blkidx];
            while( blkval ) {
                blkval = blkval & (blkval-alloct_one);
                ++count;
            }
        }
        return count;
    }

    // Methods involving two bitfields

    void merge(const OnewayBitfield &other) // set this = this | other (ref)
    {
        for (SizeT i=0; i<nblocks; ++i) {
            _blocks[i] |= other._blocks[i];
        }
        _flag_zero = (_flag_zero && other._flag_zero);
    }

    void merge(const OnewayBitfield &&other) // set this = this | other (rvalue)
    {
        for (SizeT i=0; i<nblocks; ++i) {
            _blocks[i] |= other._blocks[i];
        }
        _flag_zero = (_flag_zero && other._flag_zero);
    }

    bool equals(const OnewayBitfield &other) const // return this == other (ref)
    {
        return std::equal(_blocks, _blocks+nblocks, other._blocks);
    }

    bool equals(const OnewayBitfield &&other) const // return this == other (rvalue)
    {
        return std::equal(_blocks, _blocks+nblocks, other._blocks);
    }
};


#endif
