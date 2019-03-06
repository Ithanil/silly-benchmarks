#include "../common/Timer.hpp"
#include "../common/benchtools.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <functional>


// Benchmark of 2 (3) different approaches to handle observable accumulation
// during MC integration, when not all particles change on every step.
// NOTE: In boundary-pushing large-scale MC applications it will not matter
// too much, because the computation is dominated by other parts than
// MC integration infrastructure. So consider this benchmark as purely academic.
//
// This benchmark is crafted to somewhat resemble a realistic MC
// sampling. However, it is all simplified down to the position
// update / observable accumulation process. We use a simple
// random walk and a simple, but expensive observable, which is
// able to take advantage of knowing which positions were changed.
//
// NOTE: Every new step is considered an accepted step, but not
// necessarily all positions change on each step. The probability
// of change for each position element is given by changeThreshold.

// We compare the following approaches:
// Approach 1 (NoTrack): Just calculate everything on every step
// Approach 2 (Track): Let the xupdate method note down which x it changes.
// Approach 3 (Check): Let the observable check which x were changed.
//
// The following settings are configured:
// 10 runs per benchmark, 10000 steps per run.
// Using walker with 100 dimensions and change
// thresholds of: 2./ndim = 0.02, 1/2 and 1.
//
// Result (GCC 8: g++ -O3 -flto -march=native):
// On my systems, with the given compilation flags and benchmark settings,
// approach 2 is the best in every situation (without g++ flags, 3 is best).
// For reasons not yet understood, it wins marginally over approach 1
// even with changeThreshold = 1, where approach 1 should be the best.
// However, approach 3 is always only marginally slower than approach 2.
// Both approach 2 and 3 bring different, but in my opinion similarly
// valued, difficulties to the table, so in principle both are absolutely
// valid strategies.
// But one should keep in mind that the check of approach 3 has to be done
// again within every sampling function and observable, because the
// information about change is not shared. If they want to share the
// information however, they could directly use approach 2. So approach 2
// is considered the most efficient general choice for MC integration.


// --- Cascade of functions that implement the 3 approaches ---

void newPositionNoTrack(const int ndim, double x[], const double changeThreshold)
{
    for (int i=0; i<ndim; ++i) {
        if (rand()*(1.0 / RAND_MAX) < changeThreshold) {
            x[i] += rand()*(1.0 / RAND_MAX) - 0.5;
        }
    }
}

void newPositionTrack(const int ndim, double x[], bool flags_xchanged[], const double changeThreshold)
{
    std::fill(flags_xchanged, flags_xchanged+ndim, false);
    for (int i=0; i<ndim; ++i) {
        if (rand()*(1.0 / RAND_MAX) < changeThreshold) {
            x[i] += rand()*(1.0 / RAND_MAX) - 0.5;
            flags_xchanged[i] = true;
        }
    }
}

void newPositionCheck(const int ndim, double xnew[], const double xold[], const double changeThreshold)
{
    for (int i=0; i<ndim; ++i) {
        if (rand()*(1.0 / RAND_MAX) < changeThreshold) {
            xnew[i] = xold[i] + rand()*(1.0 / RAND_MAX) - 0.5;
        } else {
            xnew[i] = xold[i];
        }
    }
}

double calcObsElement(const double x)
{
    double out = 0.;
    for (double i=-2.; i<2.1; i+=1.) {
        //std::cout << "x " << x+i << std::endl;
        //std::cout << "f " << sin(x+i)*cos(x+i)*sqrt(fabs(x+i))*log(fabs(x+i))*exp(-fabs(x+i)) << std::endl;
        out += sin(x+i)*cos(x+i)*sqrt(fabs(x+i))*log(std::max(0.1, fabs(x+i)))*exp(-fabs(x+i));
    }
    return out;
}

double calcObsNoTrack(const int ndim, const double x[])
{
    double obs = 0.;
    for (int i=0; i<ndim; ++i) {
        obs += calcObsElement(x[i]);
    }
    return obs;
}

double calcObsTrack(const int ndim, const double x[], const bool flags_xchanged[], double lastObs[])
{
    double obs = 0.;
    for (int i=0; i<ndim; ++i) {
        if (flags_xchanged[i]) lastObs[i] = calcObsElement(x[i]);
        obs += lastObs[i];
    }
    return obs;
}

double calcObsCheck(const int ndim, const double xnew[], const double xold[], double lastObs[])
{
    double obs = 0.;
    for (int i=0; i<ndim; ++i) {
        if (xnew[i] != xold[i]) lastObs[i] = calcObsElement(xnew[i]);
        obs += lastObs[i];
    }
    return obs;
}

double sampleNoTrack(const int nsteps, const int ndim, const double changeThreshold)
{
    double obs = 0.;
    double x[ndim];
    std::fill(x, x+ndim, 0.);

    for (int i=0; i<nsteps; ++i) {
        newPositionNoTrack(ndim, x, changeThreshold);
        obs += calcObsNoTrack(ndim, x);
    }
    return obs;
}

double sampleTrack(const int nsteps, const int ndim, const double changeThreshold)
{
    double obs = 0.;
    double x[ndim];
    double lastObs[ndim];
    bool flags_xchanged[ndim];

    std::fill(x, x+ndim, 0.);
    std::fill(lastObs, lastObs+ndim, 0.);

    for (int i=0; i<nsteps; ++i) {
        newPositionTrack(ndim, x, flags_xchanged, changeThreshold);
        obs += calcObsTrack(ndim, x, flags_xchanged, lastObs);
    }
    return obs;
}

double sampleCheck(const int nsteps, const int ndim, const double changeThreshold)
{
    double obs = 0.;
    auto * xnew = new double[ndim];
    auto * xold = new double[ndim];
    double lastObs[ndim];

    std::fill(xnew, xnew+ndim, 0.);
    std::fill(xold, xold+ndim, 0.);
    std::fill(lastObs, lastObs+ndim, 0.);

    for (int i=0; i<nsteps; ++i) {
        newPositionCheck(ndim, xnew, xold, changeThreshold);
        obs += calcObsCheck(ndim, xnew, xold, lastObs);
        std::swap(xnew, xold);
    }
    delete [] xold;
    delete [] xnew;
    return obs;
}


// --- Benchmark execution ---

double benchmark_tracking(const int trackingType /* 1 notrack 2 track 3 check */, const int nsteps, const int ndim, const double changeThreshold) {
    Timer timer(1.);
    double obs;

    srand(1337);
    timer.reset();
    if (trackingType == 1) {
        obs = sampleNoTrack(nsteps, ndim, changeThreshold);
    } else if (trackingType == 2) {
        obs = sampleTrack(nsteps, ndim, changeThreshold);
    } else {
        obs = sampleCheck(nsteps, ndim, changeThreshold);
    }
    const double time = timer.elapsed();

    std::cout << "obs = " << obs/nsteps << std::endl;
    return time;
}


void run_single_benchmark(const std::string &label, const int trackingType, const int nruns, const int nsteps, const int ndim, const double changeThreshold) {
    std::pair<double, double> result;
    const double time_scale = 1000000.; //microseconds

    result = sample_benchmark([=] { return benchmark_tracking(trackingType, nsteps, ndim, changeThreshold); }, nruns);
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
