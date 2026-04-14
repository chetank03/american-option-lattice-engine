#include "BinomialTreeModel02.h"
#include "Option08.h"
#include <iostream>
#include <iomanip>
#include <cmath>

int main()
{
    using namespace std;
    using namespace fre;

    double S0    = 106.0;
    double r     = 0.058;
    double sigma = 0.46;
    double T     = (double)9 / 12;
    double K     = 100.0;
    int    N     = 8;

    cout << setiosflags(ios::fixed) << setprecision(5);
    cout << "S0 = "    << S0    << endl;
    cout << "r  = "    << r     << endl;
    cout << "sigma = " << sigma << endl;
    cout << "T = "     << T     << endl;
    cout << "K = "     << K     << endl;
    cout << "N = "     << N     << endl;
    cout << endl;

    double h = T / N;
    double U = exp(sigma * sqrt(h));
    double D = 1.0 / U;
    double R = exp(r * h);

    cout << "U = " << U << endl;
    cout << "D = " << D << endl;
    cout << "R = " << R << endl;
    cout << endl;

    BinomialTreeModel binModel(S0, U, D, R);
    Call call(N, K);

    BinLattice<double> priceTree;
    BinLattice<bool>   stoppingTree;

    try
    {
        binModel.ValidateInputData();

        OptionCalculation optionCalculation(&call);

        cout << fixed << setprecision(3);
        cout << "American call option price = "
             << optionCalculation.PriceBySnell(binModel, priceTree, stoppingTree)
             << endl;
    }
    catch (const exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }
    catch (...)
    {
        cerr << "Unknown error occurred." << endl;
        return -1;
    }

    return 0;
}

/*
S0 = 106.00000
r  = 0.05800
sigma = 0.46000
T = 0.75000
K = 100.00000
N = 8

U = 1.15125
D = 0.86862
R = 1.00545

Input data checked
There is no arbitrage

American call option price = 21.682
*/
