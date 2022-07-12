#include "honing.hpp"
#include <algorithm>
#include <iostream> // @@
#include <numeric>

#define DBL_MAX 1.79769313486231570815e+308
#define EPSILON 1e-5

using namespace std;

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
{
    this->buffCombo = {};
    this->buffComboCost = {};
    this->buffComboBoost = {};

    vector<vector<int>> buffCombo;
    vector<double> buffComboCost;
    vector<double> buffComboBoost;

    {
        vector<int> z(buffs.size(), 0);
        vector<int> zMax(buffs.size());
        for (int i = 0; i < buffs.size(); i++)
            zMax[i] = buffs[i].maxUses;
        do {
            double boostCost = getBoostCost(z);
            double prob = getBoostPercentage(z);

            buffCombo.push_back(z);
            buffComboCost.push_back(boostCost);
            buffComboBoost.push_back(prob);
        } while (nextZ(z, zMax));
    }
    
    vector<int> z(buffCombo.size());
    iota(z.begin(), z.end(), 0);

    sort(z.begin(), z.end(), [&buffComboCost, &buffComboBoost](int i, int j) {
        if (buffComboCost[i] != buffComboCost[j])
            return buffComboCost[i] < buffComboCost[j];
        return buffComboBoost[j] < buffComboBoost[i];
    });

    double bestBuff = -1; // best buff for less gold
    int efficient_count = 0;
    for (int i = 0; i < buffCombo.size(); i++) {
        bool pareto_efficient = buffComboBoost[z[i]] > bestBuff + EPSILON;
        if (pareto_efficient) {
            //for (int j = 0; j < buffs.size(); j++)
            //    cout << buffCombo[z[i]][j] << " ";
            //cout << buffComboCost[z[i]] << "g " << buffComboBoost[z[i]] << "%" << endl;
            efficient_count++;

            this->buffCombo.push_back(buffCombo[z[i]]);
            this->buffComboBoost.push_back(buffComboBoost[z[i]]);
            this->buffComboCost.push_back(buffComboCost[z[i]]);
        }
        bestBuff = max(bestBuff, buffComboBoost[z[i]]);
    }
    cout << efficient_count << " pareto efficient out of " << buffCombo.size() << endl;
}


double HoneCalculation::getSuccessProb(HoneState s, const double boostPercentage) {
    if (s.artisans_energy_percent >= 100) return 100;
    return min(percentageBase + min(percentageFailBonus * s.failed_attempts, percentageFailBonusMax) + boostPercentage, 100.0);
}


HoneState HoneCalculation::nextStateOnFail(HoneState s, const double boostPercentage) {
    s.failed_attempts++;
    s.artisans_energy_percent += getSuccessProb(s, boostPercentage) * 0.465;
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

CalculationOutput HoneCalculation::calcMinAvgCostInner(const HoneState s) {
    if (getSuccessProb(s, 0) >= 100)
        return CalculationOutput{ goldCostBase, vector<int>(buffs.size(), 0) };

    int best_index = -1;
    double best_score = DBL_MAX;

    for(int i = 0; i < buffCombo.size(); i++) {
        double boostCost = buffComboCost[i];
        double boostAmm = buffComboBoost[i];
        double prob = getSuccessProb(s, boostAmm);

        // expected extra cost if fail
        HoneState t = nextStateOnFail(s, boostAmm);
        double extra_fail_cost = calcMinAvgCost(t).minAvgCost;
        // expected extra cost if succeed = 0

        double score = goldCostBase + boostCost + (prob / 100) * 0 + (100 - prob) / 100 * extra_fail_cost;
        if (score < best_score) {
            best_score = score;
            best_index = i;
        }
        //for (int i = 0; i < buffs.size(); i++)
        //    cout << z[i] << " ";
        //cout << "\t(" << t.failed_attempts << "," << t.artisans_energy_percent << ") " << score << endl;
    };

    // cout << "At (" << s.failed_attempts << "," << s.artisans_energy_percent << "), you should go (" << best_i << "," << best_j << "," << best_k << ") for score " << best_ans << endl;
    return CalculationOutput{ best_score, buffCombo[best_index] };
}
