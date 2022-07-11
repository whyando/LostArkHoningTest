// LostArkTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <unordered_map>
#include <map>
#include <algorithm>

#include "honing.hpp"

using namespace std;

int main()
{
    HoneCalculation calc = HoneCalculation(
        3,
        2224,
        0.3,
        3,
        3,
        {
            HoningBuff{ 0.03, 33, 36 },
            HoningBuff{ 0.06, 73, 18 },
            HoningBuff{ 0.17, 185, 6 },
        });
    cout << "per attempt:" << 2224 << endl;

    HoneState s{ 0, 0.0 };
    // HoneState s{ 4, 78.0 };

    for (int t = 1; t <= 100; t++) {
        CalculationOutput out = calc.calcMinAvgCost(s);
        cout << "At (" << s.failed_attempts << "," << s.artisans_energy_percent << "), you should go (" << out.buffUses[0] << "," << out.buffUses[1] << "," << out.buffUses[2] << ") "
            << calc.getSuccessProb(s, out.buffUses) << "% "
            << "for score " << out.minAvgCost << endl;
        
        if (s.artisans_energy_percent >= 100) break;
        s = calc.nextStateOnFail(s, out.buffUses);
    }
    //HoneCalculation calc = HoneCalculation(
    //    3,
    //    2224,
    //    0.3,
    //    3,
    //    3,
    //    {
    //        HoningBuff{ 0.03, 33, 36 },
    //        HoningBuff{ 0.06, 73, 18 },
    //        HoningBuff{ 0.17, 185, 6 },
    //    });
    //auto out = calc.calcMinAvgCost(HoneState{ 0, 0.0 });
    //cout << out.minAvgCost << endl;
    //cout << out.buffUses[0] << endl;
    //cout << out.buffUses[1] << endl;
    //cout << out.buffUses[2] << endl;
}
