
#ifndef TRACKING_NEXTLVL_HPP
#define TRACKING_NEXTLVL_HPP

#include "OnewayBitfield.hpp"

#include "../change_tracking/tracking.hpp"

#include <cmath>
#include <algorithm>

// --- Cascade of functions that implement the bitfield tracking approach ---

template<typename SizeT, typename AllocT>
void newPositionBitfieldTrack(const int ndim, double x[], OnewayBitfield<SizeT, AllocT> & flags_xchanged, const double changeThreshold)
{
    flags_xchanged.reset();
    for (int i=0; i<ndim; ++i) {
        if (rand()*(1.0 / RAND_MAX) < changeThreshold) {
            x[i] += rand()*(1.0 / RAND_MAX) - 0.5;
            flags_xchanged.set(i);
        }
    }
}


template<typename SizeT, typename AllocT>
double calcObsBitfieldTrack(const int ndim, const double x[], const OnewayBitfield<SizeT, AllocT> & flags_xchanged, double lastObs[])
{
    double obs = 0.;
    bool flags[ndim];
    flags_xchanged.getAll(flags); // slower for small ndim, faster for larger ndim
    for (int i=0; i<ndim; ++i) {
        if (flags[i]) { lastObs[i] = calcObsElement(x[i]); }
        obs += lastObs[i];
    }
    return obs;
}


template<typename SizeT, typename AllocT>
double sampleBitfieldTrack(const int nsteps, const int ndim, const double changeThreshold)
{
    double obs = 0.;
    double x[ndim];
    double lastObs[ndim];
    OnewayBitfield<SizeT, AllocT> flags_xchanged(ndim);

    std::fill(x, x+ndim, 0.);
    std::fill(lastObs, lastObs+ndim, 0.);

    for (int i=0; i<nsteps; ++i) {
        newPositionBitfieldTrack(ndim, x, flags_xchanged, changeThreshold);
        obs += calcObsBitfieldTrack(ndim, x, flags_xchanged, lastObs);
    }
    return obs;
}

#endif
