#include "honing.hpp"
#include <algorithm>

#define DBL_MAX 1.79769313486231570815e+308

using namespace std;

HoneCalculation::HoneCalculation(
    const double percentageBase,
    const double goldCostBase,
    const double percentageFailBonus,
    const double percentageFailBonusMax,
    const double percentageBuffMax,
    std::vector<HoningBuff> buffs
) : percentageBase(percentageBase),
    goldCostBase(goldCostBase),
    buffs(buffs),
    percentageFailBonus(percentageFailBonus),
    percentageFailBonusMax(percentageFailBonusMax),
    percentageBuffMax(percentageBuffMax)
{}

double HoneCalculation::getBoostPercentage(const vector<int>& buffUses) {
    double boost = 0;
    for (int i = 0; i < buffs.size(); i++) {
        boost += buffUses[i] * buffs[i].percentageGain;
    }
    return min(boost, percentageBuffMax);
}

double HoneCalculation::getBoostCost(const vector<int>& buffUses) {
    double cost = 0;
    for (int i = 0; i < buffs.size(); i++) {
        cost += buffUses[i] * buffs[i].goldCost;
    }
    return cost;
}


double HoneCalculation::getSuccessProb(HoneState s, const vector<int>& buffUses) {
    if (s.artisans_energy_percent >= 100) return 100;
    return min(percentageBase + min(percentageFailBonus * s.failed_attempts, percentageFailBonusMax) + getBoostPercentage(buffUses), 100.0);
}


HoneState HoneCalculation::nextStateOnFail(HoneState s, const vector<int>& buffUses) {
    s.failed_attempts++;
    s.artisans_energy_percent += getSuccessProb(s, buffUses) * 0.465;
    // to reduce number of states, round down to 3dp
    s.artisans_energy_percent = floor(100 * s.artisans_energy_percent) / 100;
    return s;
}


CalculationOutput HoneCalculation::calcMinAvgCost(HoneState s) {
    if (m.count(s)) return m[s];
    //cout << "f(" << s.failed_attempts << ", " << s.artisans_energy_percent << ")" << endl;
    CalculationOutput ret = calcMinAvgCostInner(s);
    m[s] = ret;
    //cout << "f(" << s.failed_attempts << ", " << s.artisans_energy_percent << ") = \t" << ret << endl;
    return ret;
}

bool nextZ(vector<int>& z, const vector<int>& zMax) {
    int i = z.size() - 1;
    while (i != -1 && (z[i] == zMax[i]))
        i--;
    if (i == -1) return false;
    z[i]++;
    while (++i != z.size())
        z[i] = 0;
    return true;
}

CalculationOutput HoneCalculation::calcMinAvgCostInner(const HoneState s) {
    if (getSuccessProb(s, vector<int>(buffs.size(), 0)) >= 100)
        return CalculationOutput{ goldCostBase, vector<int>(buffs.size(), 0) };

    vector<int> best_index(buffs.size());
    double best_score = DBL_MAX;
    vector<int> z(buffs.size(), 0);
    vector<int> zMax(buffs.size());
    for (int i = 0; i < buffs.size(); i++)
        zMax[i] = buffs[i].maxUses;

    do {
        double boostCost = getBoostCost(z);
        double prob = getSuccessProb(s, z);

        // expected extra cost if fail
        HoneState t = nextStateOnFail(s, z);
        double extra_fail_cost = calcMinAvgCost(t).minAvgCost;
        // expected extra cost if succeed = 0

        double score = goldCostBase + boostCost + (prob / 100) * 0 + (100 - prob) / 100 * extra_fail_cost;
        if (score < best_score) {
            best_score = score;
            best_index = z;
        }
        //for (int i = 0; i < buffs.size(); i++)
        //    cout << z[i] << " ";
        //cout << "\t(" << t.failed_attempts << "," << t.artisans_energy_percent << ") " << score << endl;
    } while (nextZ(z, zMax));

    // cout << "At (" << s.failed_attempts << "," << s.artisans_energy_percent << "), you should go (" << best_i << "," << best_j << "," << best_k << ") for score " << best_ans << endl;
    return CalculationOutput{ best_score, best_index };
}
