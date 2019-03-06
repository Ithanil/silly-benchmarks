#include <cmath>
#include <functional>
#include <tuple>

std::pair<double, double> sample_benchmark(const std::function< double () > &run_benchmark /*all parameters bound*/, const int nruns)
{
    double times[nruns];
    double mean = 0., err = 0.;

    for (int i=0; i<nruns; ++i) { // run given benchmark nruns times
        times[i] = run_benchmark();
        mean += times[i];
    }
    mean /= nruns;
    for (int i=0; i<nruns; ++i) { err += pow(times[i]-mean, 2); }
    err /= (nruns-1.)*nruns; // variance of the mean
    err = sqrt(err); // standard error of the mean

    return std::pair<double, double>(mean, err);
}
