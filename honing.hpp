#include <vector>
#include <unordered_map>

struct HoningBuff {
	const double percentageGain;
	const double goldCost;
	const int maxUses;
};

struct CalculationOutput {
	double minAvgCost;
	std::vector<int> buffUses;
};

struct HoneStateNodeValue {
	double minAvgCost;
	int buffComboUse;
	int dfs_state;
};

struct HoneState {
	int32_t fail_stacks;
	int32_t failed_prob_sum; // ! integer (out of 10000)

	// `operator==` is required to compare keys in case of a hash collision
	bool operator==(const HoneState& p) const {
		return fail_stacks == p.fail_stacks && failed_prob_sum == p.failed_prob_sum;
	}
};
// The specialized hash function for `unordered_map` keys
struct HoneState_hash_fn
{
	std::size_t operator() (const HoneState& node) const
	{
		std::size_t h1 = std::hash<int>()(node.fail_stacks);
		std::size_t h2 = std::hash<int>()(node.failed_prob_sum);
		return h1 ^ h2;
	}
};

class HoneCalculation {
public:
	HoneCalculation(
		const double percentageBase,
		const double goldCostBase,
		const double percentageFailBonus,
		const double percentageFailBonusMax,
		const double percentageBuffMax,
		const std::vector<HoningBuff> buffs);
	CalculationOutput calcMinAvgCost(const HoneState s);
	double getSuccessProb(const HoneState s, const double boostPercentage) const;
	HoneState nextStateOnFail(const HoneState s, const double boostPercentage) const;

	double getBoostPercentage(const std::vector<int>& buffUses);
	double getBoostCost(const std::vector<int>& buffUses);

	int getNumStates();
private:
	const double percentageBase;
	const double goldCostBase;
	const double percentageFailBonus;
	const double percentageFailBonusMax;
	const double percentageBuffMax;
	std::vector<HoningBuff> buffs;

	const int maxFailStacks;

	std::vector<std::vector<int>> buffCombo;
	std::vector<double> buffComboBoost;
	std::vector<double> buffComboCost;

	void dfs(HoneState start);

	std::unordered_map<HoneState, HoneStateNodeValue, HoneState_hash_fn> f;
};
