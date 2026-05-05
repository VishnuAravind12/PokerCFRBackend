#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// River Subgame CFR Solver — Heads-Up No-Limit Hold'em
//
// Hand strengths are abstracted into NUM_BUCKETS buckets (0 = weakest,
// NUM_BUCKETS-1 = strongest). At showdown the higher bucket wins.
//
// OOP (player 0) acts first on the river. Bet sizes are pot-relative.
// All amounts are in BB. "Investment" means river-street chips put in;
// INITIAL_POT is the dead money from earlier streets.

static const int    NUM_BUCKETS  = 10;
static const double INITIAL_POT  = 2.0;           // pot entering the river (BB)
static const double STACK        = 10.0;           // effective stack (BB) per player
static const int    MAX_RAISES   = 2;
static const std::vector<double> BET_FRACS = {0.5, 1.0};  // half-pot, pot

// ── Node ──────────────────────────────────────────────────────────────────

struct Node {
    std::vector<double>      regret_sum;
    std::vector<double>      strategy_sum;
    std::vector<std::string> labels;

    bool empty() const { return regret_sum.empty(); }
    int  size()  const { return (int)regret_sum.size(); }

    void init(std::vector<std::string> l) {
        labels = std::move(l);
        regret_sum.assign(labels.size(), 0.0);
        strategy_sum.assign(labels.size(), 0.0);
    }

    std::vector<double> current_strategy(double reach) {
        int n = size();
        std::vector<double> s(n);
        double norm = 0.0;
        for (int a = 0; a < n; a++) { s[a] = std::max(regret_sum[a], 0.0); norm += s[a]; }
        for (int a = 0; a < n; a++) {
            s[a] = norm > 0 ? s[a] / norm : 1.0 / n;
            strategy_sum[a] += reach * s[a];
        }
        return s;
    }

    std::vector<double> avg_strategy() const {
        int n = size();
        std::vector<double> avg(n);
        double norm = 0.0;
        for (int a = 0; a < n; a++) norm += strategy_sum[a];
        for (int a = 0; a < n; a++)
            avg[a] = norm > 0 ? strategy_sum[a] / norm : 1.0 / n;
        return avg;
    }
};

static std::unordered_map<std::string, Node> node_map;

// ── Helpers ───────────────────────────────────────────────────────────────

static std::string bb(double x) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(2) << x;
    return o.str();
}

// Net EV for player 0 at showdown.
// inv0/inv1: total river investments by each player at the moment of showdown.
static double showdown_ev(int b0, int b1, double inv0, double inv1) {
    if (b0 > b1) return  INITIAL_POT / 2.0 + inv1;   // P0 wins: gains P1's investment + P0's share of initial pot
    if (b0 < b1) return -inv0;                         // P1 wins: P0 loses river investment
    return (inv1 - inv0) / 2.0;                        // chop: recover difference symmetrically
}

// ── CFR ───────────────────────────────────────────────────────────────────

// Returns EV for player 0 (OOP).
// inv0/inv1 : cumulative river investments for OOP / IP
// to_call   : additional chips the actor must put in to call (0 = no outstanding bet)
// actor     : 0 = OOP, 1 = IP
// raises    : number of raises already made this street
double cfr(int b0, int b1,
           double inv0, double inv1,
           double to_call, int actor, int raises,
           const std::string& hist,
           double p0, double p1)
{
    double pot       = INITIAL_POT + inv0 + inv1;
    double act_inv   = actor == 0 ? inv0 : inv1;
    double remaining = STACK - act_inv;      // max additional chips actor can invest total
    bool   facing    = to_call > 1e-9;

    std::vector<double>      bet_amts;
    std::vector<std::string> labels;

    if (!facing) {
        labels.emplace_back("check");
        for (double f : BET_FRACS) {
            double b = std::min(f * pot, remaining);
            if (b > 1e-9) {
                bet_amts.push_back(b);
                labels.push_back("bet " + bb(b) + "BB");
            }
        }
    } else {
        labels.emplace_back("fold");
        labels.push_back("call " + bb(to_call) + "BB");
        if (raises < MAX_RAISES && remaining > to_call + 1e-9) {
            double pot_ac = pot + to_call;          // pot if actor were to call
            for (double f : BET_FRACS) {
                double rt = std::min(to_call + f * pot_ac, remaining);
                if (rt > to_call + 1e-9) {
                    bet_amts.push_back(rt);
                    labels.push_back("raise " + bb(rt) + "BB");
                }
            }
        }
    }

    int num_actions = (int)labels.size();
    int ab = actor == 0 ? b0 : b1;
    std::string key = std::to_string(actor) + "_" + std::to_string(ab) + "|" + hist;

    Node& node = node_map[key];
    if (node.empty()) node.init(labels);

    double reach = actor == 0 ? p0 : p1;
    auto   strat = node.current_strategy(reach);
    std::vector<double> util(num_actions);

    for (int a = 0; a < num_actions; a++) {
        double np0 = actor == 0 ? p0 * strat[a] : p0;
        double np1 = actor == 1 ? p1 * strat[a] : p1;

        if (!facing && a == 0) {                        // check
            if (actor == 0) {
                util[a] = cfr(b0, b1, inv0, inv1, 0.0, 1, raises,
                              hist + "x", np0, np1);
            } else {
                util[a] = showdown_ev(b0, b1, inv0, inv1);
            }
        } else if (facing && a == 0) {                  // fold
            util[a] = actor == 0 ? -inv0 : INITIAL_POT / 2.0 + inv1;
        } else if (facing && a == 1) {                  // call → showdown
            double ni0 = actor == 0 ? inv0 + to_call : inv0;
            double ni1 = actor == 1 ? inv1 + to_call : inv1;
            util[a] = showdown_ev(b0, b1, ni0, ni1);
        } else {                                        // bet or raise
            int    bi  = a - (facing ? 2 : 1);
            double amt = bet_amts[bi];
            double ni0 = actor == 0 ? inv0 + amt : inv0;
            double ni1 = actor == 1 ? inv1 + amt : inv1;
            double ntc = facing ? amt - to_call : amt;  // opponent's new to_call
            std::string tag = facing ? "r" : "b";
            util[a] = cfr(b0, b1, ni0, ni1, ntc, 1 - actor,
                          raises + (facing ? 1 : 0),
                          hist + tag + std::to_string(bi),
                          np0, np1);
        }
    }

    double node_util = 0.0;
    for (int a = 0; a < num_actions; a++) node_util += strat[a] * util[a];

    double opp_reach = actor == 0 ? p1 : p0;
    double sign      = actor == 0 ? 1.0 : -1.0;  // P1 wants to minimise P0's EV
    for (int a = 0; a < num_actions; a++)
        node.regret_sum[a] += sign * opp_reach * (util[a] - node_util);

    return node_util;
}

// ── Main ──────────────────────────────────────────────────────────────────

int main() {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> bdist(0, NUM_BUCKETS - 1);

    const int ITERS = 300'000;
    double total_ev = 0.0;
    for (int i = 0; i < ITERS; i++) {
        int b0 = bdist(rng), b1 = bdist(rng);
        total_ev += cfr(b0, b1, 0.0, 0.0, 0.0, 0, 0, "", 1.0, 1.0);
    }

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "River Subgame CFR  |  " << ITERS << " iterations\n";
    std::cout << "Initial pot: " << INITIAL_POT << " BB   Effective stack: "
              << STACK << " BB   Buckets: " << NUM_BUCKETS << "\n";
    std::cout << "EV (OOP / player 0): " << total_ev / ITERS << " BB\n\n";

    // ── Print strategies ──────────────────────────────────────────────────
    // Group nodes by (actor, history). Each group has one row per bucket.

    // "group_key" -> sorted list of buckets present
    std::map<std::string, std::vector<int>> groups;
    for (auto& [k, _] : node_map) {
        auto sep    = k.find('|');
        auto us     = k.find('_');
        int  actor  = std::stoi(k.substr(0, us));
        int  bucket = std::stoi(k.substr(us + 1, sep - us - 1));
        std::string hist = k.substr(sep + 1);
        groups[std::to_string(actor) + "|" + hist].push_back(bucket);
    }
    for (auto& [g, bkts] : groups) std::sort(bkts.begin(), bkts.end());

    std::cout << "Average strategies per information set:\n";
    std::cout << std::string(72, '-') << "\n";

    for (auto& [gkey, bkts] : groups) {
        auto  gs    = gkey.find('|');
        int   actor = std::stoi(gkey.substr(0, gs));
        std::string hist = gkey.substr(gs + 1);
        std::string actor_str = actor == 0 ? "OOP" : " IP";

        std::cout << actor_str << "  history=\"" << (hist.empty() ? "<root>" : hist) << "\"\n";

        // Retrieve labels from the first bucket's node
        int first_bkt = bkts[0];
        std::string fkey = std::to_string(actor) + "_" + std::to_string(first_bkt) + "|" + hist;
        auto& lbls = node_map[fkey].labels;

        // Compute column width
        int W = 6;
        for (auto& l : lbls) W = std::max(W, (int)l.size() + 1);

        // Header row
        std::cout << "  bkt";
        for (auto& l : lbls)
            std::cout << "  " << std::setw(W) << std::right << l;
        std::cout << "\n";

        // One row per bucket
        for (int bkt : bkts) {
            std::string k = std::to_string(actor) + "_" + std::to_string(bkt) + "|" + hist;
            auto avg = node_map[k].avg_strategy();
            std::cout << "  " << std::setw(3) << bkt;
            for (double v : avg)
                std::cout << "  " << std::setw(W - 1) << std::setprecision(1)
                          << v * 100.0 << "%";
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    return 0;
}
