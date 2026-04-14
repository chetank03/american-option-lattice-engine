#include "Option08.h"
#include "BinomialTreeModel02.h"
#include "BinLattice02.h"
#include <iostream>
#include <cmath>

namespace fre {
    using namespace std;

    Option::~Option() {}

    double Call::Payoff(double z) const
    {
        if (z > K) return z - K;
        return 0.0;
    }

    double Put::Payoff(double z) const
    {
        if (z < K) return K - z;
        return 0.0;
    }

    double OptionCalculation::PriceByCRR(const BinomialTreeModel& Model)
    {
        double q = Model.RiskNeutProb();
        int N = pOption->GetN();
        vector<double> Price(N + 1);
        for (int i = 0; i <= N; i++)
        {
            Price[i] = pOption->Payoff(Model.CalculateAssetPrice(N, i));
        }
        for (int n = N - 1; n >= 0; n--)
        {
            for (int i = 0; i <= n; i++)
            {
                Price[i] = (q * Price[i + 1] + (1 - q) * Price[i]) / Model.GetR();
            }
        }
        return Price[0];
    }

    double OptionCalculation::PriceBySnell(const BinomialTreeModel& Model,
                                           BinLattice<double>& PriceTree,
                                           BinLattice<bool>& StoppingTree)
    {
        double q = Model.RiskNeutProb();
        int N = pOption->GetN();
        PriceTree.SetN(N);
        StoppingTree.SetN(N);
        double ContVal = 0;
        double ExerciseVal = 0;
        for (int i = 0; i <= N; i++)
        {
            PriceTree.SetNode(N, i, pOption->Payoff(Model.CalculateAssetPrice(N, i)));
            StoppingTree.SetNode(N, i, 1);
        }
        for (int n = N - 1; n >= 0; n--)
        {
            for (int i = 0; i <= n; i++)
            {
                ContVal = (q * PriceTree.GetNode(n + 1, i + 1) +
                    (1 - q) * PriceTree.GetNode(n + 1, i)) / Model.GetR();
                ExerciseVal = pOption->Payoff(Model.CalculateAssetPrice(n, i));
                PriceTree.SetNode(n, i, ExerciseVal);
                StoppingTree.SetNode(n, i, (ExerciseVal > 0.0));
                if (ContVal > PriceTree.GetNode(n, i))
                {
                    PriceTree.SetNode(n, i, ContVal);
                    StoppingTree.SetNode(n, i, 0);
                }
            }
        }
        return PriceTree.GetNode(0, 0);
    }

    double OptionCalculation::PriceByCRR(const BinomialTreeModel& Model,
                                         BinLattice<double>& PriceTree,
                                         BinLattice<double>& xTree,
                                         BinLattice<double>& yTree)
    {
        double q = Model.RiskNeutProb();
        int N = pOption->GetN();

        PriceTree.SetN(N);
        xTree.SetN(N);
        yTree.SetN(N);

        for (int i = 0; i <= N; i++)
        {
            PriceTree.SetNode(N, i, pOption->Payoff(Model.CalculateAssetPrice(N, i)));
        }

        for (int n = N - 1; n >= 0; n--)
        {
            for (int i = 0; i <= n; i++)
            {
                double H_up   = PriceTree.GetNode(n + 1, i + 1);
                double H_down = PriceTree.GetNode(n + 1, i);
                double S_up   = Model.CalculateAssetPrice(n + 1, i + 1);
                double S_down = Model.CalculateAssetPrice(n + 1, i);

                double x = (H_up - H_down) / (S_up - S_down);
                double y = (H_down - x * S_down) / Model.GetR();

                xTree.SetNode(n, i, x);
                yTree.SetNode(n, i, y);

                PriceTree.SetNode(n, i, (q * H_up + (1 - q) * H_down) / Model.GetR());
            }
        }

        return PriceTree.GetNode(0, 0);
    }
}
