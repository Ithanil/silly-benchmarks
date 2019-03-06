#include "../common/Timer.hpp"
#include "../common/benchtools.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <functional>


// Benchmark of 3 different ways to access and sum up data stored in an object
//
// Type 1: Use element-wise getData(i) + loop
// Type 2: Use const ptr getData() + loop
// Type 3: Use const ptr getData() + std::accumulate
//
// Also every type has two versions: One which extracts all constants explicitly
// and one which relies on the compiler to figure out what is const
//
// Result (GCC 8: g++ -O3 -flto -march=native):
// Even with only default optimization (-O2) all 6 versions yield exactly the same
// runtime, and quite probably also about exactly the same machine code. The only
// way to see a difference between types&versions is by suppressing optimization entirely (-O0).
// Quite remarkably, with -O0 the accumulate version is the fastest, with consts or not.
//
// Conclusion:
// I consider this a nice demonstration of how manual micro optimization is mostly useless, with modern compilers.
// Also, it shows how there is not a single case (even with O0!!) where a loop is better than accumulate.
// This finding fits the general concensus that the STL functions rarely carry any performance penalty,
// but rather just yield better readability and sometimes even better performance.


// --- Class for testing ---

class ObjectWithData
{
private:
    int _ndim = 0;
    double * _data = nullptr;

public:
    ~ObjectWithData(){ delete [] _data; }

    int getNDim(){ return _ndim; }
    double getData(const int i){ return _data[i]; } // element-wise access
    const double * getData(){ return _data; } // read-only pointer access

    void generateData(const int ndim)
    {
        delete [] _data;
        _data = new double[ndim];
        _ndim = ndim;
        for (int i=0; i<ndim; ++i) {
            _data[i] = rand()*(1.0 / RAND_MAX);
        }
    }
};

// element-wise access loop
double sumElementLoop(ObjectWithData * testobj) {
    double obs = 0.;
    for (int i=0; i<testobj->getNDim(); ++i) {
        obs += testobj->getData(i);
    }
    return obs;
}

double sumElementLoopConsts(ObjectWithData * testobj) {
    double obs = 0.;
    const int ndim = testobj->getNDim(); // to be sure that compiler knows this is a const
    for (int i=0; i<ndim; ++i) {
        obs += testobj->getData(i);
    }
    return obs;
}

// ptr-based access + loop
double sumPtrLoop(ObjectWithData * testobj) {
    double obs = 0.;
    for (int i=0; i<testobj->getNDim(); ++i) {
        obs += testobj->getData()[i];
    }
    return obs;
}

double sumPtrLoopConsts(ObjectWithData * testobj) {
    double obs = 0.;
    const int ndim = testobj->getNDim(); // to be sure that compiler knows this is a const
    const double * const dataptr = testobj->getData();
    for (int i=0; i<ndim; ++i) {
        obs += dataptr[i];
    }
    return obs;
}

// ptr-based access + accumulate
double sumPtrAccumulate(ObjectWithData * testobj) {
    return std::accumulate(testobj->getData(), testobj->getData()+testobj->getNDim(), 0.);
}

double sumPtrAccumulateConsts(ObjectWithData * testobj) {
    const int ndim = testobj->getNDim(); // to be sure that compiler knows this is a const
    const double * const dataptr = testobj->getData();
    return std::accumulate(dataptr, dataptr+ndim, 0.);
}


// --- Benchmark execution ---

double benchmark_objdata(const int accessType /* 1 element-loop, 2 ptr-loop, 3 ptr-accumulate */,
                                 const bool useConsts /* use versions with explicit constants */, const int ndim) {
    Timer timer(1.);
    double obs = 0.;

    ObjectWithData testobj;
    srand(1337);
    testobj.generateData(ndim);

    timer.reset();
    switch (accessType) {
    case 1:
        obs = useConsts ? sumElementLoopConsts(&testobj) : sumElementLoop(&testobj);
        break;
    case 2:
        obs = useConsts ? sumPtrLoopConsts(&testobj) : sumPtrLoop(&testobj);
        break;
    case 3:
        obs = useConsts ? sumPtrAccumulateConsts(&testobj) : sumPtrAccumulate(&testobj);
        break;
    default:
        std::cout << "Invalid accessType (must be 1, 2 or 3)." << std::endl;
        return 0.;
    }
    const double time = timer.elapsed();

    // to make sure obs is used:
    std::cout << obs/ndim;
    return time;
}


void run_single_benchmark(const std::string &label, const int nruns, const int accessType, const bool useConsts, const int ndim) {
    std::pair<double, double> result;
    const double time_scale = 1000000000.; //nanoseconds

    result = sample_benchmark([=] { return benchmark_objdata(accessType, useConsts, ndim); }, nruns);
    std::cout << std::endl << std::endl;
    std::cout << label << ":" << std::setw(std::max(1, 20-static_cast<int>(label.length()))) << std::setfill(' ') << " " << result.first/ndim*time_scale << " +- " << result.second/ndim*time_scale << " nanoseconds" << std::endl << std::endl;
}


// --- Main program ---

int main () {
    // benchmark settings
    const int nruns = 100;
    const int ndim = 10000000;
    const bool useConsts[2] = {false, true};

    std::cout << "=========================================================================================" << std::endl << std::endl;
    std::cout << "Benchmark results (time per element):" << std::endl;

    // tracking benchmark
    for (int accessType = 1; accessType < 4; ++accessType) {
        for (auto & flag_const : useConsts) {
            run_single_benchmark("t/element ( type " + std::to_string(accessType) + ", useConsts " + std::to_string(flag_const) + " )", nruns, accessType, flag_const, ndim);
        }
    }
    std::cout << "=========================================================================================" << std::endl << std::endl << std::endl;

    return 0;
}
