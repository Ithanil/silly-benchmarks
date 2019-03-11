#include "tracking_nextlvl.hpp"
#include "../change_tracking/tracking.hpp"
#include "../bitsets/OnewayBitset.hpp"
#include "../common/Timer.hpp"
#include "../common/benchtools.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <functional>


// Next level of change tracking benchmark (see ../change_tracking)
// This time I went a step further and implemented a one-way bitset
// in OnewayBitset.hpp, which is a runtime-sized bitset, specialized
// to support accumulating/merging only positive bit flips.
// I wanted to know how it performs in this scenario here, compared to either
// raw boolean array and std::vector<bool> bitset.
//
// We compare the following approaches (for code see tracking(_nextlvl).hpp):
// Approach 1 (NoTrack): Just calculate everything on every step
// Approach 2 (Track): Let the xupdate method note down which x it changes (in a raw boolean array).
// Approach 3 (Bitset (int8)): Like approach 2, but using a OnewayBitset with blocks of 1 byte.
// Approach 4 (Bitset (int64)): Like approach 3, but using blocks of 8 byte.
// Approach 5:(Bitvector): Like approach 2, but using a std::vector<bool>.
//
// The following settings are configured:
// 10 runs per benchmark, 10000 steps per run.
// Using walker with 500 dimensions and change
// thresholds of: 1./ndim, 5./ndim, 1/2 and 1.
//
// Result (GCC 8: g++ -O3 -flto -march=native):
// On my systems, with the given compilation flags and benchmark settings,
// approach 3 / 4 (1 / 8 byte blocks) manage to slightly outperform or match
// approach 5, the std::vector<bool>, in all cases. Still, the raw bool array
// remains to be similar or faster than the bitsets.
//
// Note: gcc with -Os or -O2 can produce faster code in some situations,
// with O2 appearing to potentially be the best choice overall. That is if
// we exclude gcc -Ofast, which definitely produces the fastest running code.
//
// Note 2: With all that said, this here is not a good benchmark to compare
// the general performance of bitsets. It is very application specific and
// the measured time contains the time spent on random number generation and
// the expensive observable, not the bitsets. For a more direct bitset
// benchmark, see ../bitsets .

// --- Benchmark execution ---

double benchmark_tracking_nextlvl(const int trackingType /* 1 notrack 2 track 3 bitset track */, const int nsteps, const int ndim, const double changeThreshold) {
    Timer timer(1.);
    double obs;

    srand(1337);
    timer.reset();
    if (trackingType == 1) {
        obs = sampleNoTrack(nsteps, ndim, changeThreshold);
    } else if (trackingType == 2) {
        obs = sampleTrack(nsteps, ndim, changeThreshold);
    } else if (trackingType == 3) {
        obs = sampleBitsetTrack<int, uint8_t>(nsteps, ndim, changeThreshold);
    } else if (trackingType == 4) {
        obs = sampleBitsetTrack<int, uint64_t>(nsteps, ndim, changeThreshold);
    } else {
        obs = sampleBoolvecTrack(nsteps, ndim, changeThreshold);
    }
    const double time = timer.elapsed();

    std::cout << obs;
    return time;
}


void run_single_benchmark(const std::string &label, const int trackingType, const int nruns, const int nsteps, const int ndim, const double changeThreshold) {
    std::pair<double, double> result;
    const double time_scale = 1000000.; //microseconds

    result = sample_benchmark([=] { return benchmark_tracking_nextlvl(trackingType, nsteps, ndim, changeThreshold); }, nruns);
    std::cout << std::endl << label << ":" << std::setw(std::max(1, 20-static_cast<int>(label.length()))) << std::setfill(' ') << " " << result.first/nsteps*time_scale << " +- " << result.second/nsteps*time_scale << " microseconds" << std::endl << std::endl;
}


// --- Main program ---

int main () {
    // benchmark settings
    const int nruns = 10;
    const int nsteps = 5000;
    const int ndim = 1000;
    const double changeThresholds[4] = {1./ndim, 5./ndim, 0.5, 1.};

    std::cout << "=========================================================================================" << std::endl << std::endl;
    std::cout << "Benchmark results (time per sample):" << std::endl;

    // tracking benchmark
    for (auto & threshold : changeThresholds) {
        for (int trackType = 2; trackType < 6; ++trackType) {
            run_single_benchmark("t/step ( type " + std::to_string(trackType) + ", thresh " + std::to_string(threshold) + " )", trackType, nruns, nsteps, ndim, threshold);
        }
    }
    std::cout << "=========================================================================================" << std::endl << std::endl << std::endl;

    return 0;
}
