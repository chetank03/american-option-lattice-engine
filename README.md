# American Option Lattice Engine

A C++ binomial-lattice pricing engine: Cox-Ross-Rubinstein valuation for European options and Snell-envelope backward induction for American early exercise, with a low-memory pricing path that handles 50,000 steps in 1.6 seconds and 0.4 MB.

Correctness is validated against Black-Scholes and put-call parity rather than against itself. `make test` runs 30 checks.

## Results

### Convergence to Black-Scholes

European call, `S0 = 106, K = 100, r = 0.058, sigma = 0.46, T = 0.75`. Closed-form value is **21.563762**.

| steps | binomial | absolute error |
| --- | --- | --- |
| 10 | 21.701593 | 0.137831 |
| 50 | 21.638194 | 0.074432 |
| 100 | 21.585145 | 0.021383 |
| 500 | 21.570111 | 0.006349 |
| 1000 | 21.566399 | 0.002637 |
| 5000 | 21.563848 | **0.000085** |

### Low-memory American pricing

`PriceBySnell` stores the whole triangular lattice because the exercise boundary needs it. `PriceBySnellLowMemory` returns the same price from one level vector walked in place, and computes asset prices incrementally so the sweep costs O(N) `pow` calls instead of O(N squared).

American put, same parameters, measured on g++ 13.1 at `-O2`:

| steps | O(N squared) time | O(N squared) memory | O(N) time | O(N) memory | speedup |
| --- | --- | --- | --- | --- | --- |
| 1,000 | 52 ms | 3.9 MB | 0.7 ms | 0.008 MB | 80x |
| 5,000 | 1,530 ms | 96.9 MB | 16 ms | 0.038 MB | 95x |
| 10,000 | 6,609 ms | 387.5 MB | 60 ms | 0.076 MB | 110x |
| 50,000 | 172,541 ms | 9,686.3 MB | **1,583 ms** | **0.381 MB** | **109x** |

Both paths agree to within 1e-10, asserted in the test suite. Reproduce with `make bench`.

The memory reduction and the speedup are separate wins. Dropping the lattice cuts storage by roughly 25,000x at 50,000 steps, but on its own it does not change the runtime, because both paths visit the same O(N squared) nodes. The 109x comes from removing the two `pow` calls per node, which dominated the sweep.

## Fixed along the way

Three defects the test suite was written to catch, all in `BinomialTreeModel`:

- **The no-arbitrage check was incomplete.** It rejected `R >= U` but not `R <= D`, so `U = 1.2, D = 1.1, R = 1.05` was accepted and `RiskNeutProb()` returned **-0.5**. A negative probability, no exception, and every downstream price meaningless. The condition is `D < R < U`.
- **Validation never ran on the paths in use.** Both constructors and `UpdateBinomialTreeModel` assigned fields directly; only the interactive `GetInputData()` validated, so every programmatic construction skipped the checks.
- **A `const` validator wrote to stdout.** It printed "There is no arbitrage" instead of simply returning or throwing.

Writing the tests also surfaced a genuine constraint of the CRR parameterisation: since `U = exp(sigma * sqrt(h))` and `R = exp(r * h)`, no-arbitrage requires `sigma > r * sqrt(h)`. A near-zero volatility is not priceable and now throws rather than returning nonsense.

## Design

Four pieces, so a new instrument or payoff needs no change to the pricing code:

- **`BinomialTreeModel`** holds `S0` and the gross per-step factors `U`, `D`, `R`, enforces `D < R < U` on construction, and derives the risk-neutral probability.
- **`BinLattice<T>`** is a templated triangular container, used for price trees, hedge trees, and the boolean stopping tree.
- **`Option`** is an abstract payoff, with `Call` and `Put` implementing it. Pricing depends only on `Payoff(z)`.
- **`OptionCalculation`** provides the four pricing routines:

| Method | Style | Memory | Also returns |
| --- | --- | --- | --- |
| `PriceByCRR(model)` | European | O(N) | price only |
| `PriceByCRR(model, price, x, y)` | European | O(N squared) | replicating portfolio, hedge ratio and cash position |
| `PriceBySnell(model, price, stopping)` | American | O(N squared) | exercise boundary |
| `PriceBySnellLowMemory(model)` | American | O(N) | price only |

Use `PriceBySnell` when you need the exercise boundary and `PriceBySnellLowMemory` when you only need the number. They agree to 1e-10.

## Repository Layout

```text
BinomialTreeModel02.h/.cpp   Binomial asset-price model, with no-arbitrage validation
BinLattice02.h               Templated lattice container
Option08.h/.cpp              Option classes and pricing routines
main.cpp                     Example driver program
tests/test_pricing.cpp       30 checks against Black-Scholes, parity, and known identities
tests/bench.cpp              O(N squared) vs O(N) time and memory comparison
Makefile                     Build, test, and bench targets
```

## Build

```bash
make          # builds ./lattice
make test     # 30 correctness checks
make bench    # timing and memory table, reaches 50,000 steps
```

`main.cpp` is a small driver at `N = 8` on the parameters above; it prints the model factors and

```text
American call option price = 21.682
```

Requires g++ with C++17. Built and measured on g++ 13.1.

## Origin

Started as NYU FRE-GY 6883 financial computing coursework. The pricing structure is from that
assignment; the test suite, the benchmark, the low-memory pricing path, the validation fixes
and CI were added afterwards and are not part of any submitted work.
