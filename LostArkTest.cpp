// LostArkTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <chrono> 

#include "honing.hpp"

using namespace std;

int main()
{
    auto timer_start = std::chrono::high_resolution_clock::now();
    //HoneCalculation calc = HoneCalculation(
    //    0.5,
    //    6028.2,
    //    0.05,
    //    0.5,
    //    1,
    //    {
    //        HoningBuff{ 0.01, 33, 48 },
    //        HoningBuff{ 0.02, 73, 24 },
    //        HoningBuff{ 0.04, 185, 8 },
    //    });
    HoneCalculation calc = HoneCalculation(
        0.5,
        3514.4,
        0.05,
        0.5,
        1,
        {
            HoningBuff{ 0.01, 31, 48 },
            HoningBuff{ 0.02, 66, 24 },
            HoningBuff{ 0.04, 122, 8 },
        });
    //HoneCalculation calc = HoneCalculation(
    //    30,
    //    2367,
    //    3,
    //    30,
    //    30,
    //    {
    //        HoningBuff{ 0.84, 33, 12 },
    //        HoningBuff{ 1.67, 73, 6 },
    //        HoningBuff{ 5, 185, 2 },
    //        HoningBuff{ 10, 379, 1 },
    //    });
    auto finish = std::chrono::high_resolution_clock::now();

    HoneState s{ 0, 0.0 };
    // HoneState s{ 4, 78.0 };

    for (int t = 1; t <= 1000; t++) {
        CalculationOutput out = calc.calcMinAvgCost(s);
        double buffAmount = calc.getBoostPercentage(out.buffUses);
        cout << "At (" << s.fail_stacks << "," << s.failed_prob_sum << "), you should go (" << out.buffUses[0] << "," << out.buffUses[1] << "," << out.buffUses[2] << ") "
            << calc.getSuccessProb(s, buffAmount) << "% "
            << "for score " << out.minAvgCost << endl;
        
        if (calc.getSuccessProb(s, buffAmount) >= 100) break;
        s = calc.nextStateOnFail(s, buffAmount);
    }
    cout << "states:" << calc.getNumStates() << endl;
    std::chrono::duration<double> elapsed = finish - timer_start;
    std::cout << "duration: " << 1000 * elapsed.count() << "ms\n";
}
