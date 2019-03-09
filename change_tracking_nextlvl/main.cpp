#include "OnewayBitfield.hpp"

#include "tracking_nextlvl.hpp"

#include "../change_tracking/tracking.hpp"
#include "../common/Timer.hpp"
#include "../common/benchtools.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <functional>


// Next level of change tracking benchmark (see ../change_tracking)
// This time I went a step further and implemented a one-way bitfield
// in OnewayBitfield.hpp, which is a dynamically allocated bitfield
// specialized to support accumulating/merging positive bit flips
// (starting from all bit 0). Unfortunately, it cannot really show
// its strength here, because that would require using the fast merge
// method that OnewayBitfield provides. Anyway, I wanted to know how
// it performs in this scenario here, compared to either raw boolean array
// and std::vector<bool> bitfield.
//
// We compare the following approaches (for code see tracking(_nextlvl).hpp):
// Approach 1 (NoTrack): Just calculate everything on every step
// Approach 2 (Track): Let the xupdate method note down which x it changes (in a raw boolean array).
// Approach 3 (Bitfield (int8)): Like approach 2, but using a OnewayBitfield with blocks of 1 byte.
// Approach 4 (Bitfield (int64)): Like approach 3, but using blocks of 8 byte.
// Approach 5:(Bitvector): Like approach 2, but using a std::vector<bool>.
//
// The following settings are configured:
// 10 runs per benchmark, 20000 steps per run.
// Using walker with 100 dimensions and change
// thresholds of: 2./ndim = 0.02, 1/2 and 1.
//
// Result (GCC 8: g++ -O3 -flto -march=native):
// On my systems, with the given compilation flags and benchmark settings,
// approach 4 (8 byte blocks) manages to marginally outperform or match
// approach 2 in all cases. Approach 3 (1 byte blocks) is typically a bit slower,
// but both 3 and 4 are always faster than std::vector<bool>. Yay, seems like my
// implementation isn't too bad! :D
// Anyway, the gains with OnewayBitfield are probably too marginal to consider
// using this over raw bools, in MCI. But maybe the fast merge can be used to
// positive effect there, I'll see.


// --- Benchmark execution ---

double benchmark_tracking_nextlvl(const int trackingType /* 1 notrack 2 track 3 bitfield track */, const int nsteps, const int ndim, const double changeThreshold) {
    Timer timer(1.);
    double obs;

    srand(1337);
    timer.reset();
    if (trackingType == 1) {
        obs = sampleNoTrack(nsteps, ndim, changeThreshold);
    } else if (trackingType == 2) {
        obs = sampleTrack(nsteps, ndim, changeThreshold);
    } else if (trackingType == 3) {
        obs = sampleBitfieldTrack<size_t, uint8_t>(nsteps, ndim, changeThreshold);
    } else if (trackingType == 4) {
        obs = sampleBitfieldTrack<size_t, uint64_t>(nsteps, ndim, changeThreshold);
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
    const int nsteps = 20000;
    const int ndim = 100;
    const double changeThresholds[3] = {2./ndim, 0.5, 1.};

    std::cout << "=========================================================================================" << std::endl << std::endl;
    std::cout << "Benchmark results (time per sample):" << std::endl;

    // tracking benchmark
    for (auto & threshold : changeThresholds) {
        for (int trackType = 1; trackType < 6; ++trackType) {
            run_single_benchmark("t/step ( type " + std::to_string(trackType) + ", thresh " + std::to_string(threshold) + " )", trackType, nruns, nsteps, ndim, threshold);
        }
    }
    std::cout << "=========================================================================================" << std::endl << std::endl << std::endl;

    return 0;
}
