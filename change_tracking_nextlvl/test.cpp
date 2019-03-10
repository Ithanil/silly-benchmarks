#include "OnewayBitfield.hpp"
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

using TestSizeT = size_t;
using TestAllocT = uint8_t;
using TestBF = OnewayBitfield<TestSizeT, TestAllocT>;

void reportField(const TestBF &field, const std::string &label, const bool printStatics = true, const bool printBits = true)
{
    using namespace std;
    cout << endl << "Report for " << label << ":" << endl;
    cout << "-----------------------------------" << endl;
    if (printStatics) {
        cout << "static values:" << endl;
        cout << "sizeof(this)  " << to_string(sizeof(field)) << endl;
        cout << "blocksize     " << to_string(field.blocksize) << endl;
        cout << "alloct_one    " << to_string(field.alloct_one) << endl;
        cout << "alloct_zero   " << to_string(field.alloct_zero) << endl;
        cout << "alloct_all    " << to_string(field.alloct_all) << endl;
        cout << endl;
    }
    cout << "const values:" << endl;
    cout << "nbits         " << to_string(field.nbits) << endl;
    cout << "nblocks       " << to_string(field.nblocks) << endl;
    cout << "nrest         " << to_string(field.nrest) << endl;
    cout << endl;

    cout << "other values:" << endl;
    if (printBits) {
        cout << "bits:" << endl;
        for (TestSizeT i=0; i<field.nbits; ++i) {
            cout << field.get(i) << " ";
        }
        cout << endl;
    }
    cout << "count()       " << to_string(field.count()) << endl;
    cout << "-----------------------------------" << endl;
}

void assertBitsEqual(const TestSizeT nbits, const bool bits1[], const bool bits2[])
{
    for (TestSizeT i=0; i<nbits; ++i) {
        assert(bits1[i] == bits2[i]);
    }
}

void checkFieldElementWise(const TestBF &field, const bool refBits[])
{
    for (TestSizeT i=0; i<field.nbits; ++i) {
        assert(field.get(i) == refBits[i]);
    }
}

void checkFieldArrayWise(const TestBF &field, const bool refBits[])
{
    bool testBits[field.nbits];
    field.getAll(testBits);
    assertBitsEqual(field.nbits, testBits, refBits);
}

void checkField(const TestBF &field, const bool refBits[])
{
    checkFieldElementWise(field, refBits);
    checkFieldArrayWise(field, refBits);
}


// --- Main program ---

int main () {
    using namespace std;

    const TestSizeT nbits = 17;
    const TestSizeT testIndex1 = 7;
    const TestSizeT testIndex2 = 16;

    TestBF testfield1(nbits);
    TestBF testfield2(nbits);
    TestBF testfield3(nbits);
    bool refBits[nbits] {0};

    // check proper init of testfield1 (2 is identical)
    reportField(testfield1, "testfield1");
    assert(testfield1.alloct_one == 1);
    assert(testfield1.alloct_zero == 0);
    assert(testfield1.nbits == nbits);
    assert(testfield1.count() == 0);
    assert(testfield1.none());
    assert(!testfield1.any());
    assert(!testfield1.all());
    checkField(testfield1, refBits);

    // set bit testIndex1 of testfield2
    testfield2.set(testIndex1);
    refBits[testIndex1] = true;
    reportField(testfield2, "testfield2", false);
    assert(testfield2.count() == 1);
    assert(!testfield2.none());
    assert(testfield2.any());
    assert(!testfield2.all());
    checkField(testfield2, refBits);

    // set bit testIndex2 of testfield2
    testfield2.set(testIndex2);
    refBits[testIndex2] = true;
    reportField(testfield2, "testfield2", false);
    assert(testfield2.count() == 2);
    checkField(testfield2, refBits);

    // merge testfield2 into testfield1
    testfield1.merge(testfield2);
    reportField(testfield1, "testfield1", false);
    checkField(testfield1, refBits);

    // merge testfield3 (all 0) into testfield1
    // should not change anything
    testfield1.merge(testfield3);
    reportField(testfield1, "testfield1", false);
    checkField(testfield1, refBits);

    // merge a copy of testfield2 into testfield3,
    // to also test rvalue-ref overload of merge
    testfield3.merge(TestBF(testfield2));
    reportField(testfield3, "testfield3", false);
    checkField(testfield3, refBits);

    // finally set all bits 1 on testfield3
    testfield3.setAll();
    std::fill(refBits, refBits+nbits, true);
    reportField(testfield3, "testfield3", false);
    assert(testfield3.count() == nbits);
    assert(testfield3.all());
    checkField(testfield3, refBits);

    // and reset testfield3
    testfield3.reset();
    std::fill(refBits, refBits+nbits, false);
    reportField(testfield3, "testfield3", false);
    assert(testfield3.count() == 0);
    assert(testfield3.none());
    assert(!testfield3.any());
    assert(!testfield3.all());
    checkField(testfield3, refBits);

    cout << endl << "Creating two fields of 20 GBit size..." << endl;
    const TestSizeT hugeNumber = 20000000000UL;
    TestBF testfield4(hugeNumber);
    TestBF testfield5(hugeNumber);
    cout << "Done." << endl;

    reportField(testfield4, "testfield4", false, false);

    cout << "Now we perform some operations on one or both fields:" << endl << endl;

    cout << "Setting every third bit of first field, element-wise..." << endl;
    for (TestSizeT i=0; i<testfield4.nbits; i+=3) {
        testfield4.set(i);
    }
    cout << "Done." << endl << endl;

    cout << "Counting first field..." << endl;
    cout << "count = " << testfield4.count() << endl << endl;

    cout << "Merging first field into second..." << endl;
    testfield5.merge(testfield4);
    cout << "Done." << endl << endl;

    cout << "Resetting both..." << endl;
    testfield4.reset();
    testfield5.reset();
    cout << "Done." << endl << endl;

    cout << "Setting some bits, element-wise, on both fields..." << endl;
    for (TestSizeT i=500; i<testfield4.nbits; i+=997) { // set some bits on both fields
        testfield4.set(i-500);
        testfield4.set(i-250);
        testfield5.set(i-250);
        testfield5.set(i);
    }
    cout << "Done." << endl << endl;

    cout << "Counting..." << endl;
    cout << "count1 = " << testfield4.count() << endl;
    cout << "count2 = " << testfield5.count() << endl << endl;

    cout << "Merging second into first..." << endl;
    testfield4.merge(testfield5);
    cout << "count1 = " << testfield4.count() << endl << endl;

    return 0;
}
