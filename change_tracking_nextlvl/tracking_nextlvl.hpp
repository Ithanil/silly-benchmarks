#ifndef TRACKING_NEXTLVL_HPP
#define TRACKING_NEXTLVL_HPP

#include "../bitsets/OnewayBitset.hpp"

#include "../change_tracking/tracking.hpp"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <vector>

// --- Cascade of functions that perform the bitset tracking approach ---

template<typename SizeT, typename AllocT>
void newPositionBitsetTrack(const int ndim, double x[], OnewayBitset<SizeT, AllocT> & flags_xchanged, const double changeThreshold)
{
    for (int i=0; i<ndim; ++i) {
        if (rand()*(1.0 / RAND_MAX) < changeThreshold) {
            x[i] += rand()*(1.0 / RAND_MAX) - 0.5;
            flags_xchanged.set(i);
        }
    }
}

void newPositionBoolvecTrack(const int ndim, double x[], std::vector<bool> &flags_xchanged, const double changeThreshold)
{
    for (int i=0; i<ndim; ++i) {
        if (rand()*(1.0 / RAND_MAX) < changeThreshold) {
            x[i] += rand()*(1.0 / RAND_MAX) - 0.5;
            flags_xchanged[i] = true;
        }
    }
}


template<typename SizeT, typename AllocT>
double calcObsBitsetTrack(const int ndim, const double x[], const OnewayBitset<SizeT, AllocT> & flags_xchanged, double lastObs[])
{
    // make use of fast flag-based any()
    if (flags_xchanged.any()) {
        double obs = 0.;
        for (int i=0; i<ndim; ++i) {
            if (flags_xchanged.get(i)) { lastObs[i] = calcObsElement(x[i]); }
            obs += lastObs[i];
        }
        return obs;
    }
    else { return std::accumulate(lastObs, lastObs+ndim, 0.); }

    /* other approach
    double obs = 0.;
    int idx = 0;
    int blkidx = 0;
    unsigned int bitidx = 0; // will only count up 64 max
    while (idx < ndim) { // we can use tuple based indexing of the bits
        if (flags_xchanged.get(blkidx, bitidx)) { lastObs[idx] = calcObsElement(x[idx]); }
        obs += lastObs[idx];
        ++idx;
        ++bitidx;
        if (bitidx >= flags_xchanged.blocksize) {
            bitidx = 0;
            ++blkidx;
        }
    }
    return obs;
    */
}

double calcObsBoolvecTrack(const int ndim, const double x[], const std::vector<bool> & flags_xchanged, double lastObs[])
{
    double obs = 0.;
    for (int i=0; i<ndim; ++i) {
        if (flags_xchanged[i]) { lastObs[i] = calcObsElement(x[i]); }
        obs += lastObs[i];
    }
    return obs;
}


template<typename SizeT, typename AllocT>
double sampleBitsetTrack(const int nsteps, const int ndim, const double changeThreshold)
{
    double obs = 0.;
    double x[ndim];
    double lastObs[ndim];
    OnewayBitset<SizeT, AllocT> flags_xchanged(ndim);

    std::fill(x, x+ndim, 0.);
    std::fill(lastObs, lastObs+ndim, 0.);
    flags_xchanged.setAll();

    for (int i=0; i<nsteps; ++i) {
        newPositionBitsetTrack(ndim, x, flags_xchanged, changeThreshold);
        obs += calcObsBitsetTrack(ndim, x, flags_xchanged, lastObs);
        flags_xchanged.reset();
    }
    return obs;
}

double sampleBoolvecTrack(const int nsteps, const int ndim, const double changeThreshold)
{
    double obs = 0.;
    double x[ndim];
    double lastObs[ndim];
    std::vector<bool> flags_xchanged(ndim);

    std::fill(x, x+ndim, 0.);
    std::fill(lastObs, lastObs+ndim, 0.);
    std::fill(flags_xchanged.begin(), flags_xchanged.end(), true);

    for (int i=0; i<nsteps; ++i) {
        newPositionBoolvecTrack(ndim, x, flags_xchanged, changeThreshold);
        obs += calcObsBoolvecTrack(ndim, x, flags_xchanged, lastObs);
        std::fill(flags_xchanged.begin(), flags_xchanged.end(), false);
    }
    return obs;
}


#endif
