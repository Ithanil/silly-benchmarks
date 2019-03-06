#include "../common/Timer.hpp"
#include "../common/benchtools.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <functional>


// Benchmark large nested vs flat arrays
//
// Here we consider large nested arrays with a larger first and a smaller second dimension,
// stored as jagged array (double **) in one case and as flat array in the other.
// We measure the time needed to sum up all array elements, with different loop/sum constructs
// and 3 different combinations of array dimensions.
//
// Result (GCC 8: g++ -O3 -flto -march=native):
// As expected, the jagged array yields significantly worse performance when the second dimension
// is small. This is easily explained by bad cache/memory efficiency due to only the sub-arrays
// of second dimension are contiguous. This means that the CPU has to wait a lot for new data.
// The flat array however yields about the same runtime for any combination of dimensions, at least
// when a single loop/accumulate is used to obtain the sum. A nested loop construct instead carries
// a small performance penalty when second dimension is small, even when the array is flat.
// Finally, again we find that accumulate and loop are virtually identical in every aspect,
// besides code style / readability.

// Conclusion:
// Don't use jagged arrays when the second dimension is the same for each element. It leads to ugly
// new/delete code, it requires nested loops to work with (you can't simply access all elements in a row with a single loop)
// and it is never beneficial to performance. The only upside is the multi-index access, which is
// really not much of a reason as soon as you got used to the index calculus for flat multidim arrays,
// which isn't even needed whenever you want to do the same operation with all elements.


// --- Functions to generate the data ---

// fill flat data with random numbers
void generateDataFlat(const int ntotaldim, double data[]) {
    for (int i=0; i<ntotaldim; ++i) {
        data[i] = rand()*(1.0 / RAND_MAX);
    }
}

// fill nested data with random numbers
void generateDataJagged(const int nsteps, const int ndim, double ** data) {
    for (int i=0; i<nsteps; ++i) {
        for (int j=0; j<ndim; ++j) {
            data[i][j] = rand()*(1.0 / RAND_MAX);
        }
    }
}


// --- Functions to sum up all data ---

// flat loop, flat array
double flatLoopArrayFlat(const int ntotaldim, double data[]) {
    double obs = 0.;
    for (int i=0; i<ntotaldim; ++i) {
        obs += data[i];
    }
    return obs;
}

// nested loop, flat array
double nestedLoopArrayFlat(const int nsteps, const int ndim, double data[]) {
    double obs = 0.;
    for (int i=0; i<nsteps; ++i) {
        for (int j=0; j<ndim; ++j) {
            obs += data[i*ndim + j];
        }
    }
    return obs;
}

// flat accumulate, flat array
double flatAccuArrayFlat(const int ntotaldim, double data[]) {
    return std::accumulate(data, data+ntotaldim, 0.);
}

// nested accumulate, flat array
double nestedAccuArrayFlat(const int nsteps, const int ndim, double data[]) {
    double obs = 0.;
    for (int i=0; i<nsteps; ++i) {
        obs = std::accumulate(data+i*ndim, data+(i+1)*ndim, obs);
    }
    return obs;
}


// nested loop, nested array
double nestedLoopArrayNested(const int nsteps, const int ndim, double ** data) {
    double obs = 0.;
    for (int i=0; i<nsteps; ++i) {
        for (int j=0; j<ndim; ++j) {
            obs += data[i][j];
        }
    }
    return obs;
}

// nested accumulate, nested array
double nestedAccuArrayNested(const int nsteps, const int ndim, double ** data) {
    double obs = 0.;
    for (int i=0; i<nsteps; ++i) {
        obs = std::accumulate(data[i], data[i]+ndim, obs);
    }
    return obs;
}



// --- Benchmark execution ---

double benchmark_jagged(const bool useJagged, const bool useNestedLoop, const bool useAccumulate, const int nsteps, const int ndim) {
    Timer timer(1.);
    double time = 0.;
    double obs = 0.;

    srand(1337);
    if (useJagged) {
        if (!useNestedLoop) {
            std::cout << "Jagged array requires nested loop!" << std::endl;
            return 0.;
        } else {
            double ** dataJagged = new double*[nsteps];
            for (int i=0; i<nsteps; ++i) { dataJagged[i] = new double[ndim]; }
            generateDataJagged(nsteps, ndim, dataJagged);

            timer.reset();
            obs = useAccumulate ? nestedAccuArrayNested(nsteps, ndim, dataJagged) : nestedLoopArrayNested(nsteps, ndim, dataJagged);
            time = timer.elapsed();

            for (int i=0; i<nsteps; ++i) { delete [] dataJagged[i]; }
            delete [] dataJagged;
        }
    } else { // use flat array
        const int ntotaldim = nsteps*ndim; // for convenience
        double * dataFlat = new double[ntotaldim];
        generateDataFlat(ntotaldim, dataFlat);

        timer.reset();
        if (useNestedLoop) {
            obs = useAccumulate ? nestedAccuArrayFlat(nsteps, ndim, dataFlat) : nestedLoopArrayFlat(nsteps, ndim, dataFlat);
        } else {
            obs = useAccumulate ? flatAccuArrayFlat(ntotaldim, dataFlat) : flatLoopArrayFlat(ntotaldim, dataFlat);
        }
        time = timer.elapsed();

        delete [] dataFlat;
    }

    // to make sure obs is used:
    const double normf = (1./nsteps)/ndim;
    std::cout << obs*normf;

    return time;
}

void run_single_benchmark(const std::string &label, const int nruns, const bool useJagged, const bool useNestedLoop, const bool useAccumulate, const int nsteps, const int ndim) {
    std::pair<double, double> result;
    const double time_scale = 1000000000.; //nanoseconds
    const double normf = (1./nsteps)/ndim;

    result = sample_benchmark([=] { return benchmark_jagged(useJagged, useNestedLoop, useAccumulate, nsteps, ndim); }, nruns);

    std::cout << std::endl << std::endl;
    std::cout << label << ":" << std::setw(std::max(1, 20-static_cast<int>(label.length()))) << std::setfill(' ') << " " << result.first*normf*time_scale << " +- " << result.second*normf*time_scale << " nanoseconds" << std::endl << std::endl;
}


// --- Main program ---

int main () {
    // benchmark settings
    const int nruns = 10;

    std::vector< std::array<int, 2> > dimensions;
    dimensions.push_back({10000000, 2}); // worst case for jagged
    dimensions.push_back({2000000, 10}); // still bad
    dimensions.push_back({200000, 100}); // not much difference

    std::vector< std::array<bool, 3> > settings; // each sub-vector holds 3 config bools: useJagged, useNestedLoop, useAccumulate
    settings.push_back({true, true, true});
    settings.push_back({true, true, false});
    settings.push_back({false, true, true});
    settings.push_back({false, true, false});
    settings.push_back({false, false, true});
    settings.push_back({false, false, false});

    std::cout << "=========================================================================================" << std::endl << std::endl;
    std::cout << "Benchmark results (time per element):" << std::endl;

    // benchmark arrays
    for (auto & dimension : dimensions) {
        std::cout << std::endl << "Array dimensions: " << dimension[0] << " x " << dimension[1] << std::endl << std::endl;
        for (auto & setting : settings) {
            run_single_benchmark("t/element ( jaggedArray " + std::to_string(setting[0]) +
                                 ", nestedLoop " + std::to_string(setting[1]) +
                                 ", useAccumulate " + std::to_string(setting[2]) + " )",
                                 nruns, setting[0], setting[1], setting[2], dimension[0], dimension[1]);
        }
    }
    std::cout << "=========================================================================================" << std::endl << std::endl << std::endl;

    return 0;
}
