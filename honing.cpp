#include "honing.hpp"
#include <algorithm>
#include <numeric>
#include <stack>
#include <iostream> // @@
// #include <chrono> 

#define DBL_MAX 1.79769313486231570815e+308
#define EPSILON 1e-9

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
    percentageFailBonus(percentageFailBonus),
    percentageFailBonusMax(percentageFailBonusMax),
    percentageBuffMax(percentageBuffMax),
    buffs(buffs),
    maxFailStacks(percentageFailBonusMax / percentageFailBonus)
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

    this->f = unordered_map<HoneState, HoneStateNodeValue, HoneState_hash_fn>();
}


// DFS
void HoneCalculation::dfs(HoneState start) {
    stack<HoneState> s;
    s.push(start);

    while (!s.empty()) {
        const auto x = s.top();
        HoneStateNodeValue* fx = &this->f[x];
        switch (fx->dfs_state) {
            case 1:
                {
                s.pop();
                    int best_index = -1;
                    double best_score = DBL_MAX;

                    // for y in A(x)
                    for (int i = 0; i < this->buffCombo.size(); i++) {
                        const double boostCost = this->buffComboCost[i];
                        const double boostAmm = this->buffComboBoost[i];
                        const double prob = getSuccessProb(x, boostAmm);
                        const HoneState y = nextStateOnFail(x, boostAmm);

                        const double extra_fail_cost = f[y].minAvgCost;
                        const double score = goldCostBase + boostCost + (prob / 100) * 0 + (100 - prob) / 100 * extra_fail_cost;
                        if (score < best_score) {
                            best_score = score;
                            best_index = i;
                        }
                    }

                    fx->minAvgCost = best_score;
                    fx->buffComboUse = best_index;
                    fx->dfs_state = 2;
                    break;
                }
            case 0:
                if (getSuccessProb(x, 0) >= 100) {
                    s.pop();
                    fx->minAvgCost = goldCostBase;
                    fx->buffComboUse = 0;
                    fx->dfs_state = 2;
                    continue;
                }
                else {
                    fx->dfs_state = 1; // change state to 1 but leave on stack

                    // for y in A(x)
                    for (const double& boost : this->buffComboBoost) {
                        s.push(nextStateOnFail(x, boost));
                    }
                }
                break;
            default:
                s.pop();
                break;
        }
    }
}

double HoneCalculation::getSuccessProb(const HoneState s, const double boostPercentage) const {
    if (s.failed_prob_sum >= 21500) return 100;
    return min(percentageBase + min(percentageFailBonus * s.fail_stacks, percentageFailBonusMax) + boostPercentage, 100.0);
}

HoneState HoneCalculation::nextStateOnFail(const HoneState s, const double boostPercentage) const {
    HoneState t = s;
    if (t.fail_stacks < maxFailStacks)
        t.fail_stacks++;
    t.failed_prob_sum += 100 * getSuccessProb(s, boostPercentage);
    return t;
}

CalculationOutput HoneCalculation::calcMinAvgCost(HoneState s) {
    if (f.count(s)) {
        return CalculationOutput{ f[s].minAvgCost, buffCombo[f[s].buffComboUse] };
    }
    this->dfs(s);

    if (f.count(s)) {
        return CalculationOutput{ f[s].minAvgCost, buffCombo[f[s].buffComboUse] };
    }

    // unknown
    return CalculationOutput{ -1, {} };
}

int HoneCalculation::getNumStates() {
    return f.size();
}
