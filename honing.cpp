#include "honing.hpp"
#include <algorithm>
#include <numeric>
#include <stack>
// #include <iostream>
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

double HoneCalculation::getBuffComboWeight(const vector<int>& buffUses) {
    double weight = 0;
    for (int i = 0; i < buffs.size(); i++) {
        weight += 1000 * (buffUses[i] == 0 || buffUses[i] == buffs[i].maxUses);
        weight -= buffUses[i];
    }
    return weight;
}

HoneCalculation::HoneCalculation(
    const double percentageBase,
    const double goldCostBase,
    const double percentageFailBonus,
    const double percentageFailBonusMax,
    const double percentageBuffMax,
    std::vector<HoningBuff> buffs,
    bool precise
) : percentageBase(percentageBase),
    goldCostBase(goldCostBase),
    percentageFailBonus(percentageFailBonus),
    percentageFailBonusMax(percentageFailBonusMax),
    percentageBuffMax(percentageBuffMax),
    buffs(buffs),
    precise(precise)
{
    this->buffCombo = {};
    this->buffComboCost = {};
    this->buffComboBoost = {};
    this->buffComboWeight = {};

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
            this->buffComboWeight.push_back(getBuffComboWeight(buffCombo[z[i]]));
        }
        bestBuff = max(bestBuff, buffComboBoost[z[i]]);
    }
    // cout << efficient_count << " pareto efficient out of " << buffCombo.size() << endl;

    this->dfs();
}

// DFS
void HoneCalculation::dfs() {
    // auto timer_start = std::chrono::high_resolution_clock::now();

    this->f = unordered_map<HoneState, HoneStateNodeValue, HoneState_hash_fn>();
    // unordered_map<HoneState, HoneStateNodeValue, HoneState_hash_fn> f;

    stack<HoneState> s;
    HoneState start = HoneState{ 0, 0 };
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
                    double base = 0;

                    vector<double> score(this->buffCombo.size());

                    // for y in A(x)
                    for (int i = 0; i < this->buffCombo.size(); i++) {
                        const double boostCost = this->buffComboCost[i];
                        const double boostAmm = this->buffComboBoost[i];
                        const double prob = getSuccessProb(x, boostAmm);
                        const HoneState y = nextStateOnFail(x, boostAmm);

                        const double extra_fail_cost = f[y].minAvgCost;
                        score[i] = goldCostBase + boostCost + (prob / 100) * 0 + (100 - prob) / 100 * extra_fail_cost;
                        if ((score[i] < best_score) || (score[i] == best_score && this->buffComboWeight[i] > this->buffComboWeight[best_index])) {
                            best_score = score[i];
                            best_index = i;
                            base = f[y].minAvgCost;
                        }
                    }

                    int best_index_1 = best_index;
                    double best_score_1 = best_score;

                    if (!precise && base != 0) {
                        for (int i = 0; i < this->buffCombo.size(); i++) {
                            if (((score[i]-base) <= (best_score-base)*1.15) && (best_index_1 == -1 || this->buffComboWeight[i] > this->buffComboWeight[best_index_1])) {
                                best_score_1 = score[i];
                                best_index_1 = i;
                            }
                        }
                    }

                    fx->minAvgCost = best_score_1;
                    fx->buffComboUse = best_index_1;
                    fx->dfs_state = 2;
                    break;
                }
            case 0:
                // s.artisans_energy_percent >= 100
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
    // auto finish = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> elapsed = finish - timer_start;
    // std::cout << "new: " << 1000 * elapsed.count() << "ms\n";

    // const auto fx = f[start];
    // cout << fx.minAvgCost << endl;
}

double HoneCalculation::getSuccessProb(const HoneState s, const double boostPercentage) const {
    if (s.artisans_energy_percent >= 215) return 100;
    return min(percentageBase + min(percentageFailBonus * s.failed_attempts, percentageFailBonusMax) + boostPercentage, 100.0);
}


HoneState HoneCalculation::nextStateOnFail(HoneState s, const double boostPercentage) const {
    s.failed_attempts++;
    s.artisans_energy_percent += getSuccessProb(s, boostPercentage);
    // to reduce number of states, round down to 3dp
    s.artisans_energy_percent = floor(100 * s.artisans_energy_percent) / 100;
    return s;
}


CalculationOutput HoneCalculation::calcMinAvgCost(HoneState s) {
    if (f.count(s)) {
        return CalculationOutput{ f[s].minAvgCost, buffCombo[f[s].buffComboUse] };
    }
    else {
        // unknown
        return CalculationOutput{ -1, {} };
    }
}

int HoneCalculation::getNumStates() {
    return f.size();
}
