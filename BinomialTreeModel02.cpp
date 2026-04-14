#include "BinomialTreeModel02.h"
#include <iostream>
#include <cmath>

namespace fre {
    double BinomialTreeModel::RiskNeutProb() const
    {
        return (R - D) / (U - D);
    }

    double BinomialTreeModel::CalculateAssetPrice(int n, int i) const
    {
        return S0 * pow(U, i) * pow(D, n - i);
    }

    void BinomialTreeModel::UpdateBinomialTreeModel(double S0_, double U_, double D_, double R_)
    {
        S0 = S0_;
        U = U_;
        D = D_;
        R = R_;
    }

    void BinomialTreeModel::GetInputData()
    {
        using std::cout;
        using std::cin;
        using std::endl;
        cout << "Enter S0: "; cin >> S0;
        cout << "Enter U:  "; cin >> U;
        cout << "Enter D:  "; cin >> D;
        cout << "Enter R:  "; cin >> R;
        cout << endl;
        ValidateInputData();
    }

    void BinomialTreeModel::ValidateInputData() const
    {
        if (S0 <= 0.0 || U <= 0.0 || D <= 0.0 || U <= D || R <= 0.0)
        {
            throw std::invalid_argument("Illegal data ranges: S0, U, D, R must be positive and U > D");
        }
        if (R >= U || U <= D)
        {
            throw std::invalid_argument("Arbitrage exists: R must be less than U and U > D");
        }
        std::cout << "Input data checked" << std::endl;
        std::cout << "There is no arbitrage" << std::endl << std::endl;
    }
}
