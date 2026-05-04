#include <array>
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>

// Kuhn Poker: 3 cards (J=0, Q=1, K=2), 2 players, 1-chip ante each.
// Actions: p = pass/check, b = bet (1 chip)

static const int NUM_ACTIONS = 2; // 0=pass, 1=bet
static const char ACTIONS[2] = {'p', 'b'};

struct Node {
    std::array<double, NUM_ACTIONS> regret_sum = {0.0, 0.0};
    std::array<double, NUM_ACTIONS> strategy_sum = {0.0, 0.0};

    std::array<double, NUM_ACTIONS> get_strategy(double realization_weight) {
        std::array<double, NUM_ACTIONS> strategy;
        double norm = 0.0;
        for (int a = 0; a < NUM_ACTIONS; a++) {
            strategy[a] = std::max(regret_sum[a], 0.0);
            norm += strategy[a];
        }
        for (int a = 0; a < NUM_ACTIONS; a++) {
            strategy[a] = (norm > 0) ? strategy[a] / norm : 1.0 / NUM_ACTIONS;
            strategy_sum[a] += realization_weight * strategy[a];
        }
        return strategy;
    }

    std::array<double, NUM_ACTIONS> get_average_strategy() const {
        std::array<double, NUM_ACTIONS> avg;
        double norm = 0.0;
        for (int a = 0; a < NUM_ACTIONS; a++) norm += strategy_sum[a];
        for (int a = 0; a < NUM_ACTIONS; a++)
            avg[a] = (norm > 0) ? strategy_sum[a] / norm : 1.0 / NUM_ACTIONS;
        return avg;
    }
};

std::unordered_map<std::string, Node> node_map;

// Returns payoff from player 0's perspective given terminal history.
double terminal_payoff(const std::array<int, 2>& cards, const std::string& history) {
    int p0 = cards[0], p1 = cards[1];
    bool p0_wins = p0 > p1;

    if (history == "pp")  return p0_wins ? 1.0 : -1.0;           // both check, showdown
    if (history == "bp")  return 1.0;                              // p1 folds
    if (history == "pbp") return -1.0;                             // p0 folds after p1 bet
    if (history == "bb")  return p0_wins ? 2.0 : -2.0;            // p0 bet, p1 called
    if (history == "pbb") return p0_wins ? 2.0 : -2.0;            // p0 check, p1 bet, p0 called
    return 0.0; // unreachable
}

bool is_terminal(const std::string& h) {
    return h == "pp" || h == "bp" || h == "pbp" || h == "bb" || h == "pbb";
}

// Returns expected value for player 0. p0, p1 are reach probabilities.
double cfr(const std::array<int, 2>& cards, const std::string& history, double p0, double p1) {
    if (is_terminal(history))
        return terminal_payoff(cards, history);

    int player = history.size() % 2 == 0 ? 0 : 1; // p0 acts on even-length histories
    // After "p", player 1 acts; after "pb", player 0 acts again.
    // Recompute based on actual game tree position:
    // history length: 0 -> p0, 1 -> p1, 2 -> p0 (only "pb" reaches here), never 3+ non-terminal
    int acting = (history.size() == 1) ? 1 : 0;

    std::string info_set = std::to_string(cards[acting]) + history;
    Node& node = node_map[info_set];

    double reach = (acting == 0) ? p0 : p1;
    auto strategy = node.get_strategy(reach);

    std::array<double, NUM_ACTIONS> action_util;
    double node_util = 0.0;
    for (int a = 0; a < NUM_ACTIONS; a++) {
        std::string next = history + ACTIONS[a];
        if (acting == 0)
            action_util[a] = cfr(cards, next, p0 * strategy[a], p1);
        else
            action_util[a] = cfr(cards, next, p0, p1 * strategy[a]);
        node_util += strategy[a] * action_util[a];
    }

    // Update regrets (from acting player's perspective; flip sign for player 1)
    double sign = (acting == 0) ? 1.0 : -1.0;
    double opponent_reach = (acting == 0) ? p1 : p0;
    for (int a = 0; a < NUM_ACTIONS; a++)
        node.regret_sum[a] += sign * opponent_reach * (action_util[a] - node_util);

    return node_util;
}

int main() {
    std::array<int, 3> deck = {0, 1, 2};
    std::mt19937 rng(42);

    const int ITERATIONS = 100000;
    double total_util = 0.0;

    for (int i = 0; i < ITERATIONS; i++) {
        std::shuffle(deck.begin(), deck.end(), rng);
        std::array<int, 2> cards = {deck[0], deck[1]};
        total_util += cfr(cards, "", 1.0, 1.0);
    }

    std::cout << "Game value (player 0): " << total_util / ITERATIONS << "\n\n";

    const char* card_names[] = {"J", "Q", "K"};
    std::cout << "Average strategy per info set:\n";
    for (auto& [key, node] : node_map) {
        int card = key[0] - '0';
        std::string hist = key.substr(1);
        auto avg = node.get_average_strategy();
        std::cout << "  Card=" << card_names[card] << " history=\"" << hist
                  << "\": pass=" << avg[0] << " bet=" << avg[1] << "\n";
    }

    return 0;
}
