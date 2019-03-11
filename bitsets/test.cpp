#include "OnewayBitset.hpp"
#include "../change_tracking/tracking.hpp"
#include "../common/Timer.hpp"
#include "../common/benchtools.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <functional>
#include <cassert>

using TestSizeT = uint64_t;
using TestAllocT = uint8_t;
using TestBF = OnewayBitset<TestSizeT, TestAllocT>;

void reportBitset(const TestBF &bitset, const std::string &label, const bool printStatics = true, const bool printBits = true)
{
    using namespace std;
    cout << endl << "Report for " << label << ":" << endl;
    cout << "-----------------------------------" << endl;
    if (printStatics) {
        cout << "static values:" << endl;
        cout << "sizeof(this)  " << to_string(sizeof(bitset)) << endl;
        cout << "blocksize     " << to_string(bitset.blocksize) << endl;
        cout << "alloct_one    " << to_string(bitset.alloct_one) << endl;
        cout << "alloct_zero   " << to_string(bitset.alloct_zero) << endl;
        cout << "alloct_all    " << to_string(bitset.alloct_all) << endl;
        cout << endl;
    }
    cout << "const values:" << endl;
    cout << "nbits         " << to_string(bitset.getNBits()) << endl;
    cout << "nblocks       " << to_string(bitset.getNBlocks()) << endl;
    cout << "padblk        " << to_string(bitset.getPadBlock()) << endl;
    cout << endl;

    cout << "other values:" << endl;
    if (printBits) {
        cout << "bits:" << endl;
        for (TestSizeT i=0; i<bitset.getNBits(); ++i) {
            cout << bitset.get(i) << " ";
        }
        cout << endl;
    }
    cout << "count()       " << to_string(bitset.count()) << endl;
    cout << "-----------------------------------" << endl;
}

void assertBitsEqual(const TestSizeT nbits, const bool bits1[], const bool bits2[])
{
    for (TestSizeT i=0; i<nbits; ++i) {
        assert(bits1[i] == bits2[i]);
    }
}

void checkBitsetElementWise(const TestBF &bitset, const bool refBits[])
{
    for (TestSizeT i=0; i<bitset.getNBits(); ++i) {
        assert(bitset.get(i) == refBits[i]);
    }
}

void checkBitsetArrayWise(const TestBF &bitset, const bool refBits[])
{
    bool testBits[bitset.getNBits()];
    bitset.getAll(testBits);
    assertBitsEqual(bitset.getNBits(), testBits, refBits);
}

void checkBitset(const TestBF &bitset, const bool refBits[])
{
    checkBitsetElementWise(bitset, refBits);
    checkBitsetArrayWise(bitset, refBits);
}


// --- Main program ---

int main () {
    using namespace std;

    const TestSizeT nbits = 17;
    const TestSizeT testIndex1 = 7;
    const TestSizeT testIndex2 = 16;

    TestBF testset1(nbits);
    TestBF testset2(nbits);
    TestBF testset3(nbits);
    bool refBits[nbits] {0};

    // check proper init of testset1 (2 is identical)
    reportBitset(testset1, "testset1");
    assert(testset1.alloct_one == 1);
    assert(testset1.alloct_zero == 0);
    assert(testset1.getNBits() == nbits);
    assert(testset1.count() == 0);
    assert(testset1.none());
    assert(!testset1.any());
    assert(!testset1.all());
    checkBitset(testset1, refBits);
    assert(testset1 == testset2);

    // set bit testIndex1 of testset2
    testset2.set(testIndex1);
    refBits[testIndex1] = true;
    reportBitset(testset2, "testset2", false);
    assert(testset2.count() == 1);
    assert(!testset2.none());
    assert(testset2.any());
    assert(!testset2.all());
    checkBitset(testset2, refBits);
    assert(testset1 != testset2);

    // set bit testIndex2 of testset2
    testset2.set(testIndex2);
    refBits[testIndex2] = true;
    reportBitset(testset2, "testset2", false);
    assert(testset2.count() == 2);
    checkBitset(testset2, refBits);

    // merge testset2 into testset1
    testset1.merge(testset2);
    reportBitset(testset1, "testset1", false);
    checkBitset(testset1, refBits);
    assert(testset1 == testset2);

    // merge testset3 (all 0) into testset1
    // should not change anything
    testset1+=testset3; // using compound assignment overload
    reportBitset(testset1, "testset1", false);
    checkBitset(testset1, refBits);

    // test copy/move/assign: 
    // make a copy of testset1, merge testset2,
    // and testset3 to result
    testset3 = TestBF(testset1) + testset2;
    reportBitset(testset3, "testset3", false);
    checkBitset(testset3, refBits);

    // finally set all bits 1 on testset3
    testset3.setAll();
    std::fill(refBits, refBits+nbits, true);
    reportBitset(testset3, "testset3", false);
    assert(testset3.count() == nbits);
    assert(testset3.all());
    checkBitset(testset3, refBits);

    // and reset testset3
    testset3.reset();
    std::fill(refBits, refBits+nbits, false);
    reportBitset(testset3, "testset3", false);
    assert(testset3.count() == 0);
    assert(testset3.none());
    assert(!testset3.any());
    assert(!testset3.all());
    checkBitset(testset3, refBits);

    cout << endl << "Creating two bitsets of 20 GBit size..." << endl;
    const TestSizeT hugeNumber = 20000000000UL;
    TestBF testset4(hugeNumber);
    TestBF testset5(hugeNumber);
    cout << "Done." << endl;

    reportBitset(testset4, "testset4", false, false);

    cout << "Now we perform some operations on one or both bitsets:" << endl << endl;

    cout << "Setting every third bit of first bitset, element-wise..." << endl;
    for (TestSizeT i=0; i<testset4.getNBits(); i+=3) {
        testset4.set(i);
    }
    cout << "Done." << endl << endl;

    cout << "Counting first bitset..." << endl;
    cout << "count = " << testset4.count() << endl << endl;

    cout << "Merging first bitset into second..." << endl;
    testset5.merge(testset4);
    cout << "Done." << endl << endl;

    cout << "Resetting both..." << endl;
    testset4.reset();
    testset5.reset();
    cout << "Done." << endl << endl;

    cout << "Setting some bits, element-wise, on both bitsets..." << endl;
    for (TestSizeT i=500; i<testset4.getNBits(); i+=997) { // set some bits on both bitsets
        testset4.set(i-500);
        testset4.set(i-250);
        testset5.set(i-250);
        testset5.set(i);
    }
    cout << "Done." << endl << endl;

    cout << "Counting..." << endl;
    cout << "count1 = " << testset4.count() << endl;
    cout << "count2 = " << testset5.count() << endl << endl;

    cout << "Merging second into first..." << endl;
    testset4.merge(testset5);
    cout << "count1 = " << testset4.count() << endl << endl;

    return 0;
}
