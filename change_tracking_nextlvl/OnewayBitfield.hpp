/* struct OnewayBitfield
   Author: Jan Kessler (2019)

   Some inspiration came from:
   1) https://www.hackerearth.com/practice/notes/bit-manipulation/
   2) https://stackoverflow.com/a/47990
   3) https://stackoverflow.com/a/26230537
*/

#ifndef ONEWAY_BITFIELD_HPP
#define ONEWAY_BITFIELD_HPP

#include <type_traits>
#include <climits>
#include <limits>
#include <algorithm>

template <
    typename SizeT, /* integral index type (e.g. int or size_t) */
    typename AllocT /* integral memory type (e.g. char) */
    >
struct OnewayBitfield
// A minimalistic bitfield class, which starts with all bits = 0 and then
// accumulates changes of bits to 1, until the field gets evaluated
// and reset to 0 again.
// This is meant to be used to remember a subset of flagged indices out of
// a range (0, 1, ..., nflags-1), in a setting where the the flags are set
// iteratively in one part of your program and multiple other parts want to
// accumulate true flags (in their own bitfield instances) until the flags
// are used to control (optimize) execution of recurring, expensive computations.
// The present implementation should be very time&space efficient for this use case,
// especially due to the fast merge() method.
{
public:
    // --- Compile-Time Statics
    static constexpr AllocT blocksize = static_cast<AllocT>( sizeof(AllocT)*CHAR_BIT ); // safe, every integer is capable of holding own size in bits
    static constexpr AllocT alloct_one = 1; // block of alloc type, least bit 1
    static constexpr AllocT alloct_zero = 0; // block of alloc type, all bits 0

    // --- Runtime consts

    const SizeT nflags;
    const SizeT nblocks;
    AllocT * const blocks; // ptr to memory allocation storing the bitfield


    // --- Constructors/Destructor

    explicit OnewayBitfield(const SizeT nflags):
        nflags(nflags), nblocks((nflags-1)/blocksize + 1), blocks(new AllocT[nblocks])
    {
        this->reset();
    }

    OnewayBitfield(const OnewayBitfield &other):
        nflags(other.nflags), nblocks(other.nblocks), blocks(new AllocT[nblocks])
    {
        std::copy(other.blocks, other.blocks+nblocks, blocks);
    }

    ~OnewayBitfield(){ delete [] blocks; }


    // --- Methods

    void reset() { // reset to 0/false
        std::fill(blocks, blocks+nblocks, alloct_zero);
    }

    void set(SizeT index) // set the single bit corresponding to index (to 1)
    {
        // let's hope these two become one instruction
        const SizeT blockIndex = index / blocksize;
        const AllocT bitIndex = static_cast<AllocT>(index % blocksize);

        // set single bit
        blocks[blockIndex] |= (alloct_one << bitIndex);
    }

    void setAll() { // fast way to set all bits 1, but why would you want to do that, right?
        std::fill(blocks, blocks+nblocks, ~(alloct_zero));
    }

    void merge(const OnewayBitfield &other) // merge ref
    {
        for (SizeT i=0; i<nblocks; ++i) {
            blocks[i] |= other.blocks[i];
        }
    }

    void merge(const OnewayBitfield &&other) // merge rvalue
    {
        for (SizeT i=0; i<nblocks; ++i) {
            blocks[i] |= other.blocks[i];
        }
    }

    bool get(SizeT index) const // element-wise get (a bit slow)
    {
        // let's hope these two become one instruction
        const SizeT blockIndex = index / blocksize;
        const SizeT bitIndex = static_cast<AllocT>(index % blocksize);

        // get single bit
        return ( (blocks[blockIndex] >> bitIndex) & alloct_one );
        //return ( blocks[blockIndex] & (alloct_one << bitIndex) );
    }

    void getAll(bool out[] /*bool out[nflags]*/) const // get all (fast)
    {
        SizeT blkidx=0;
        AllocT bitidx=0;
        AllocT blkval = blocks[0];
        for (SizeT flagidx=0; flagidx<nflags; ++flagidx, ++bitidx, blkval >>= alloct_one) {
            if (bitidx>=blocksize) {
                bitidx = 0;
                ++blkidx;
                blkval = blocks[blkidx];
            }
            out[flagidx] = blkval & alloct_one;
        }
    }

    SizeT count() const // returns number of true bits in worst-case log(N) time
    {
        SizeT count = 0;
        for (SizeT blkidx=0; blkidx<nblocks; ++blkidx) {
            AllocT blkval = blocks[blkidx];
            while( blkval && count<nflags ) {
                blkval = blkval & (blkval-alloct_one);
                ++count;
            }
        }
        return count;
    }
};


#endif
