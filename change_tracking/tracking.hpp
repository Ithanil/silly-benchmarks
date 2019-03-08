#ifndef TRACKING_HPP
#define TRACKING_HPP

#include <cmath>
#include <algorithm>

// --- Cascade of functions that implement the 3 tracking approaches ---

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
        if (flags_xchanged[i]) { lastObs[i] = calcObsElement(x[i]); }
        obs += lastObs[i];
    }
    return obs;
}

double calcObsCheck(const int ndim, const double xnew[], const double xold[], double lastObs[])
{
    double obs = 0.;
    for (int i=0; i<ndim; ++i) {
        if (xnew[i] != xold[i]) { lastObs[i] = calcObsElement(xnew[i]); }
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

#endif
