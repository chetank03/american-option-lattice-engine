// Correctness tests for the lattice engine. Plain asserts, no framework: every case below
// has an answer that is known independently of this code, which is the point. A test that
// only compares the engine against itself proves nothing.
//
// Build and run: make test

#include "../BinomialTreeModel02.h"
#include "../Option08.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace fre;

namespace {

int checks = 0;

void check(bool cond, const char* what)
{
    ++checks;
    if (!cond)
    {
        std::printf("FAIL: %s\n", what);
        std::abort();
    }
}

// Standard normal CDF via erfc, which avoids the cancellation you get from 0.5*(1+erf(x))
// in the left tail.
double norm_cdf(double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); }

// Reference implementation, deliberately independent of the lattice code.
double black_scholes(double S0, double K, double r, double sigma, double T, bool is_call)
{
    const double sqrtT = std::sqrt(T);
    const double d1 = (std::log(S0 / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrtT);
    const double d2 = d1 - sigma * sqrtT;
    if (is_call)
        return S0 * norm_cdf(d1) - K * std::exp(-r * T) * norm_cdf(d2);
    return K * std::exp(-r * T) * norm_cdf(-d2) - S0 * norm_cdf(-d1);
}

// CRR parameterisation: the model takes gross per-step factors, not r and sigma.
BinomialTreeModel crr_model(double S0, double r, double sigma, double T, int N)
{
    const double h = T / N;
    const double U = std::exp(sigma * std::sqrt(h));
    return BinomialTreeModel(S0, U, 1.0 / U, std::exp(r * h));
}

const double S0 = 106.0, K = 100.0, r = 0.058, sigma = 0.46, T = 0.75;

// ---------------------------------------------------------------------------

// The engine's European price must approach Black-Scholes as steps rise. This is the only
// test here that validates the numerics against the outside world.
void test_converges_to_black_scholes()
{
    const double exact = black_scholes(S0, K, r, sigma, T, true);
    const int steps[] = {10, 50, 100, 500, 1000, 5000};
    double prev_err = 1e9;

    std::printf("\n  European call, binomial vs Black-Scholes (exact = %.6f)\n", exact);
    std::printf("  %8s %14s %14s\n", "steps", "binomial", "abs error");

    for (int N : steps)
    {
        BinomialTreeModel model = crr_model(S0, r, sigma, T, N);
        Call call(N, K);
        OptionCalculation calc(&call);
        const double price = calc.PriceByCRR(model);
        const double err = std::fabs(price - exact);
        std::printf("  %8d %14.6f %14.6f\n", N, price, err);

        // Binomial converges in 1/N but oscillates between odd and even N, so require the
        // trend rather than strict monotonicity at every step.
        if (N >= 500) check(err < 0.05, "error should be small once N >= 500");
        prev_err = err;
    }
    check(prev_err < 0.01, "N = 5000 should be within a cent of Black-Scholes");
}

// Put-call parity holds exactly in the binomial model, not just in the limit, so this is a
// tight tolerance and it catches discounting errors that convergence tests can hide.
void test_put_call_parity()
{
    const int N = 500;
    BinomialTreeModel model = crr_model(S0, r, sigma, T, N);

    Call call(N, K);
    Put put(N, K);
    OptionCalculation cc(&call), pc(&put);

    const double lhs = cc.PriceByCRR(model) - pc.PriceByCRR(model);
    const double rhs = S0 - K * std::exp(-r * T);
    check(std::fabs(lhs - rhs) < 1e-6, "European put-call parity");
}

// An American call on a non-dividend-paying stock is never exercised early, so it must equal
// the European call exactly. If the early-exercise branch is wrong, this fails.
void test_american_call_equals_european()
{
    const int N = 200;
    BinomialTreeModel model = crr_model(S0, r, sigma, T, N);
    Call call(N, K);
    OptionCalculation calc(&call);

    BinLattice<double> priceTree;
    BinLattice<bool> stoppingTree;

    const double european = calc.PriceByCRR(model);
    const double american = calc.PriceBySnell(model, priceTree, stoppingTree);
    check(std::fabs(american - european) < 1e-9, "American call == European call, no dividends");
}

// An American put may be exercised early, so it is worth at least the European put, and the
// premium must be strictly positive for a put that is near or in the money.
void test_american_put_dominates_european()
{
    const int N = 200;
    BinomialTreeModel model = crr_model(S0, r, sigma, T, N);
    Put put(N, K);
    OptionCalculation calc(&put);

    BinLattice<double> priceTree;
    BinLattice<bool> stoppingTree;

    const double european = calc.PriceByCRR(model);
    const double american = calc.PriceBySnell(model, priceTree, stoppingTree);
    check(american >= european - 1e-12, "American put >= European put");
    check(american > european, "early exercise premium is positive for this put");
}

// The O(N) path must agree with the full-lattice path, or it is not an optimisation, it is a
// second implementation with its own bugs.
//
// Tolerance is 1e-10 rather than exact: the fast path builds asset prices incrementally by
// multiplying by U/D along a level, while PriceBySnell calls pow() per node. Same values
// mathematically, different rounding, and the incremental error grows with level width. So
// require agreement far below any price anyone would quote, not bitwise equality.
void test_low_memory_matches_full_lattice()
{
    for (int N : {1, 2, 8, 51, 200, 1000})
    {
        BinomialTreeModel model = crr_model(S0, r, sigma, T, N);
        BinLattice<double> priceTree;
        BinLattice<bool> stoppingTree;

        Put put(N, K);
        OptionCalculation pc(&put);
        const double full = pc.PriceBySnell(model, priceTree, stoppingTree);
        const double lean = pc.PriceBySnellLowMemory(model);
        check(std::fabs(full - lean) < 1e-10, "American put: O(N) matches O(N^2)");

        Call call(N, K);
        OptionCalculation cc(&call);
        BinLattice<double> pt2;
        BinLattice<bool> st2;
        check(std::fabs(cc.PriceBySnell(model, pt2, st2) - cc.PriceBySnellLowMemory(model)) < 1e-10,
              "American call: O(N) matches O(N^2)");
    }
}

// A single step is small enough to verify by hand:
//   price = (q * payoff_up + (1 - q) * payoff_down) / R,  q = (R - D) / (U - D)
void test_single_step_by_hand()
{
    const double U = 1.2, D = 0.8, R = 1.05, s0 = 100.0, k = 100.0;
    BinomialTreeModel model(s0, U, D, R);
    Call call(1, k);
    OptionCalculation calc(&call);

    const double q = (R - D) / (U - D);                       // 0.625
    const double expected = (q * (s0 * U - k) + (1 - q) * 0.0) / R;
    check(std::fabs(calc.PriceByCRR(model) - expected) < 1e-12, "one step matches hand calculation");
}

// Low volatility collapses the tree, so an in-the-money call approaches its discounted
// forward intrinsic value.
//
// Note the floor on sigma. CRR sets U = exp(sigma * sqrt(h)) and R = exp(r * h), so
// no-arbitrage (R < U) requires sigma > r * sqrt(h). At N = 100 here that is sigma > 0.005,
// and sigma = 1e-7 correctly throws rather than pricing. That is a property of the
// parameterisation, not a bug, and it is why this test cannot use sigma = 0.
void test_low_volatility_approaches_intrinsic()
{
    const int N = 100;
    const double h = T / N;
    const double sigma_floor = r * std::sqrt(h);
    const double low = 0.02;
    check(low > sigma_floor, "test sigma must clear the no-arbitrage floor");

    BinomialTreeModel model = crr_model(S0, r, low, T, N);
    Call call(N, K);
    OptionCalculation calc(&call);

    const double intrinsic = S0 - K * std::exp(-r * T);
    check(std::fabs(calc.PriceByCRR(model) - intrinsic) < 0.05, "low sigma approaches intrinsic");

    // And sigma below the floor must be rejected, not silently priced.
    bool threw = false;
    try { crr_model(S0, r, 1e-7, T, N); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "sigma below the no-arbitrage floor must throw");
}

// The bugs this suite was written for. Each of these constructions used to be accepted.
void test_validation_rejects_arbitrage()
{
    bool threw = false;
    // R below D: the old check tested only R >= U, so this passed and RiskNeutProb()
    // returned (1.05 - 1.1) / (1.2 - 1.1) = -0.5, a negative probability.
    try { BinomialTreeModel(100.0, 1.2, 1.1, 1.05); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "R <= D must be rejected");

    threw = false;
    try { BinomialTreeModel(100.0, 1.2, 0.8, 1.3); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "R >= U must be rejected");

    threw = false;
    try { BinomialTreeModel(100.0, 0.8, 1.2, 1.0); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "U <= D must be rejected");

    threw = false;
    try { BinomialTreeModel(-1.0, 1.2, 0.8, 1.05); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "negative S0 must be rejected");

    // UpdateBinomialTreeModel used to bypass validation entirely.
    threw = false;
    try
    {
        BinomialTreeModel model(100.0, 1.2, 0.8, 1.05);
        model.UpdateBinomialTreeModel(100.0, 1.2, 1.1, 1.05);
    }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw, "UpdateBinomialTreeModel must validate too");

    // And a valid model must still produce a probability in [0, 1].
    BinomialTreeModel good(100.0, 1.2, 0.8, 1.05);
    const double q = good.RiskNeutProb();
    check(q > 0.0 && q < 1.0, "valid model gives a probability in (0, 1)");
}

}  // namespace

int main()
{
    test_converges_to_black_scholes();
    test_put_call_parity();
    test_american_call_equals_european();
    test_american_put_dominates_european();
    test_low_memory_matches_full_lattice();
    test_single_step_by_hand();
    test_low_volatility_approaches_intrinsic();
    test_validation_rejects_arbitrage();

    std::printf("\n  %d checks passed\n", checks);
    return 0;
}
