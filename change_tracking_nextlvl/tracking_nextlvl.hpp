#ifndef TRACKING_NEXTLVL_HPP
#define TRACKING_NEXTLVL_HPP

#include "OnewayBitfield.hpp"

#include "../change_tracking/tracking.hpp"

#include <cmath>
#include <algorithm>
#include <vector>

// --- Cascade of functions that perform the bitfield tracking approach ---

template<typename SizeT, typename AllocT>
void newPositionBitfieldTrack(const int ndim, double x[], OnewayBitfield<SizeT, AllocT> & flags_xchanged, const double changeThreshold)
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
double calcObsBitfieldTrack(const int ndim, const double x[], const OnewayBitfield<SizeT, AllocT> & flags_xchanged, double lastObs[])
{
    double obs = 0.;
    //bool flags[ndim]; // one could alternatively first getAll into these flags
    //flags_xchanged.getAll(flags);
    for (int i=0; i<ndim; ++i) {
        //if (flags[i]) { lastObs[i] = calcObsElement(x[i]); }
        if (flags_xchanged.get(i)) { lastObs[i] = calcObsElement(x[i]); }
        obs += lastObs[i];
    }
    return obs;
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
double sampleBitfieldTrack(const int nsteps, const int ndim, const double changeThreshold)
{
    double obs = 0.;
    double x[ndim];
    double lastObs[ndim];
    OnewayBitfield<SizeT, AllocT> flags_xchanged(ndim);

    std::fill(x, x+ndim, 0.);
    std::fill(lastObs, lastObs+ndim, 0.);
    flags_xchanged.setAll();

    for (int i=0; i<nsteps; ++i) {
        newPositionBitfieldTrack(ndim, x, flags_xchanged, changeThreshold);
        obs += calcObsBitfieldTrack(ndim, x, flags_xchanged, lastObs);
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
