# American Option Lattice Engine

This repository contains a C++ pricing engine for options in a binomial-lattice setting, with support for both standard Cox-Ross-Rubinstein valuation and early-exercise pricing using a Snell-envelope style backward induction step. The project was originally developed as part of NYU financial computing coursework and is presented here as a standalone portfolio project because it demonstrates reusable C++ design for quantitative finance.

## Overview

The codebase includes:

- a binomial asset-price model
- a reusable binomial lattice container
- option abstractions for call and put payoffs
- CRR pricing for European-style valuation
- Snell-based backward induction for American-style pricing
- hedge-ratio and cash-position tree generation

## Key Features

- **Binomial tree model**
  Encapsulates `S0`, up/down factors, growth factor, validation, and risk-neutral probability.

- **Reusable lattice structure**
  Provides a templated binomial lattice class for storing pricing trees, hedge trees, and stopping decisions.

- **Option abstraction**
  Implements a base `Option` interface and concrete call and put payoff types.

- **American option pricing**
  Uses backward induction with comparison between continuation value and exercise value to price early-exercise options.

- **Replicating portfolio output support**
  Builds `xTree` and `yTree` values for delta-style hedge and cash-position representation.

## Repository Layout

```text
BinomialTreeModel02.h/.cpp   Binomial asset-price model
BinLattice02.h               Templated lattice container
Option08.h/.cpp              Option classes and pricing routines
main.cpp                     Example driver program
Makefile                     Build target
```

## Build

```bash
make
./HW07
```

## Example Configuration

The sample driver uses:

- `S0 = 106.0`
- `r = 0.058`
- `sigma = 0.46`
- `T = 9/12`
- `K = 100.0`
- `N = 8`

Expected output includes:

```text
American call option price = 21.682
```
