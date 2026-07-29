#pragma once
#include <cmath>
#include <stdexcept>
#include <iostream>

namespace fre
{

    class BinomialTreeModel
    {
    private:
        double S0, U, D, R;

    public:
        BinomialTreeModel() : S0(0), U(0), D(0), R(0) {}
        // Validates on construction. Previously only GetInputData() validated, so every
        // programmatic construction, which is every use from a test or another module,
        // skipped the checks entirely.
        BinomialTreeModel(double S0_, double U_, double D_, double R_)
            : S0(S0_), U(U_), D(D_), R(R_) { ValidateInputData(); }
        ~BinomialTreeModel() {}

        double GetS0() const { return S0; }
        double GetU() const { return U; }
        double GetD() const { return D; }
        double GetR() const { return R; }

        double RiskNeutProb() const;
        double CalculateAssetPrice(int n, int i) const;
        void UpdateBinomialTreeModel(double S0_, double U_, double D_, double R_);
        void GetInputData();
        void ValidateInputData() const;
    };
}
