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
	int failed_attempts;
	double artisans_energy_percent;

	// `operator==` is required to compare keys in case of a hash collision
	bool operator==(const HoneState& p) const {
		return failed_attempts == p.failed_attempts && artisans_energy_percent == p.artisans_energy_percent;
	}
};
// The specialized hash function for `unordered_map` keys
struct HoneState_hash_fn
{
	std::size_t operator() (const HoneState& node) const
	{
		std::size_t h1 = std::hash<int>()(node.failed_attempts);
		std::size_t h2 = std::hash<double>()(node.artisans_energy_percent);
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
		const std::vector<HoningBuff> buffs,
		bool precise);
	CalculationOutput calcMinAvgCost(const HoneState s);
	double getSuccessProb(const HoneState s, const double boostPercentage) const;
	HoneState nextStateOnFail(const HoneState s, const double boostPercentage) const;

	double getBoostPercentage(const std::vector<int>& buffUses);
	double getBoostCost(const std::vector<int>& buffUses);
	double getBuffComboWeight(const std::vector<int>& buffUses);

	int getNumStates();
private:
	const double percentageBase;
	const double goldCostBase;
	const double percentageFailBonus;
	const double percentageFailBonusMax;
	const double percentageBuffMax;
	std::vector<HoningBuff> buffs;
	bool precise;

	std::vector<std::vector<int>> buffCombo;
	std::vector<double> buffComboBoost;
	std::vector<double> buffComboCost;
	std::vector<double> buffComboWeight;

	void dfs();

	std::unordered_map<HoneState, HoneStateNodeValue, HoneState_hash_fn> f;
};
