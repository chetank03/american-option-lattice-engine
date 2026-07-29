// Compares the two American pricing paths: the full triangular lattice, which is O(N^2) in
// memory, against in-place backward induction over one level vector, which is O(N).
//
// Both return the same price; test_pricing.cpp asserts that to 1e-12. The only difference is
// what it costs to get there.
//
// Build and run: make bench

#include "../BinomialTreeModel02.h"
#include "../Option08.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <new>
#include <stdexcept>

using namespace fre;
using clk = std::chrono::steady_clock;

namespace {

double ms_since(clk::time_point t0)
{
    return std::chrono::duration<double, std::milli>(clk::now() - t0).count();
}

// The full lattice stores every node: (N+1)(N+2)/2 doubles for prices, plus a bit-packed
// bool of the same shape for the stopping tree.
double full_lattice_mb(long long N)
{
    const double nodes = (double)(N + 1) * (double)(N + 2) / 2.0;
    return (nodes * sizeof(double) + nodes / 8.0) / (1024.0 * 1024.0);
}

double level_vector_mb(long long N)
{
    return ((double)(N + 1) * sizeof(double)) / (1024.0 * 1024.0);
}

BinomialTreeModel crr_model(double S0, double r, double sigma, double T, int N)
{
    const double h = T / N;
    const double U = std::exp(sigma * std::sqrt(h));
    return BinomialTreeModel(S0, U, 1.0 / U, std::exp(r * h));
}

}  // namespace

// CI builds with -DBENCH_MAX_STEPS=5000 to skip the large sizes: at 50,000 steps the
// O(N^2) path takes minutes and allocates about 10 GB, which is not a CI workload.
#ifndef BENCH_MAX_STEPS
#define BENCH_MAX_STEPS 50000
#endif

int main()
{
    const double S0 = 106.0, K = 100.0, r = 0.058, sigma = 0.46, T = 0.75;
    const int all_sizes[] = {1000, 5000, 10000, 50000};

    std::printf("American put, S0=%.0f K=%.0f r=%.3f sigma=%.2f T=%.2f\n\n", S0, K, r, sigma, T);
    std::printf("%8s | %12s %10s | %12s %10s | %8s\n",
                "steps", "O(N^2) ms", "mem MB", "O(N) ms", "mem MB", "speedup");
    std::printf("---------+-------------------------+-------------------------+---------\n");

    for (int N : all_sizes)
    {
        if (N > BENCH_MAX_STEPS) continue;
        BinomialTreeModel model = crr_model(S0, r, sigma, T, N);
        Put put(N, K);
        OptionCalculation calc(&put);

        double full_ms = -1.0;
        double full_price = 0.0;
        bool full_ok = true;
        try
        {
            BinLattice<double> priceTree;
            BinLattice<bool> stoppingTree;
            const clk::time_point t0 = clk::now();
            full_price = calc.PriceBySnell(model, priceTree, stoppingTree);
            full_ms = ms_since(t0);
        }
        catch (const std::bad_alloc&)
        {
            full_ok = false;
        }
        catch (const std::length_error&)
        {
            full_ok = false;
        }

        const clk::time_point t1 = clk::now();
        const double lean_price = calc.PriceBySnellLowMemory(model);
        const double lean_ms = ms_since(t1);

        if (full_ok)
        {
            std::printf("%8d | %12.1f %10.1f | %12.1f %10.3f | %7.1fx\n",
                        N, full_ms, full_lattice_mb(N), lean_ms, level_vector_mb(N),
                        full_ms / lean_ms);
            if (std::fabs(full_price - lean_price) > 1e-9)
            {
                std::printf("  MISMATCH: %.10f vs %.10f\n", full_price, lean_price);
                return 1;
            }
        }
        else
        {
            std::printf("%8d | %12s %10.1f | %12.1f %10.3f | %8s\n",
                        N, "OUT OF MEM", full_lattice_mb(N), lean_ms, level_vector_mb(N), "n/a");
        }
        std::printf("         | price %.6f\n", lean_price);
        std::fflush(stdout);
    }

    return 0;
}
