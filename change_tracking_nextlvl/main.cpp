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


// Benchmark of different approaches to handle observable accumulation
// during MC integration, when not all particles change on every step.

// We compare the following approaches (for code see tracking(_nextlvl).hpp):
// Approach 1:
// Approach 2:
//
// The following settings are configured:
// 10 runs per benchmark, 10000 steps per run.
// Using walker with 100 dimensions and change
// thresholds of: 2./ndim = 0.02, 1/2 and 1.
//
// Result (GCC 8: g++ -O3 -flto -march=native):
// On my systems, with the given compilation flags and benchmark settings,...


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
    } else {
        obs = sampleBitfieldTrack<int, uint_fast8_t>(nsteps, ndim, changeThreshold);
    }
    const double time = timer.elapsed();

    std::cout << "obs = " << obs/nsteps << std::endl;
    return time;
}


void run_single_benchmark(const std::string &label, const int trackingType, const int nruns, const int nsteps, const int ndim, const double changeThreshold) {
    std::pair<double, double> result;
    const double time_scale = 1000000.; //microseconds

    result = sample_benchmark([=] { return benchmark_tracking_nextlvl(trackingType, nsteps, ndim, changeThreshold); }, nruns);
    std::cout << label << ":" << std::setw(std::max(1, 20-static_cast<int>(label.length()))) << std::setfill(' ') << " " << result.first/nsteps*time_scale << " +- " << result.second/nsteps*time_scale << " microseconds" << std::endl;
}


// --- Main program ---

int main () {
    // benchmark settings
    const int nruns = 10;
    const int nsteps = 10000;
    const int ndim = 100;
    const double changeThresholds[3] = {2./ndim, 0.5, 1.};

    std::cout << "=========================================================================================" << std::endl << std::endl;
    std::cout << "Benchmark results (time per sample):" << std::endl;

    // tracking benchmark
    for (auto & threshold : changeThresholds) {
        for (int trackType = 1; trackType < 4; ++trackType) {
            run_single_benchmark("t/step ( type " + std::to_string(trackType) + ", thresh " + std::to_string(threshold) + " )", trackType, nruns, nsteps, ndim, threshold);
        }
    }
    std::cout << "=========================================================================================" << std::endl << std::endl << std::endl;

    return 0;
}
