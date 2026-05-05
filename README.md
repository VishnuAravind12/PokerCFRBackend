# Poker CFR Solvers

Two C++ counterfactual regret minimization (CFR) solvers for heads-up poker.

## What is CFR?

Counterfactual Regret Minimization is an iterative algorithm for finding Nash equilibria in extensive-form games. Each iteration samples a deal, traverses the game tree, and accumulates **regret** — the difference between how much a player would have won by deviating to a better action and what they actually earned. After many iterations the **average strategy** (weighted by visit frequency) converges to a Nash equilibrium.

---

## Solvers

### 1. Kuhn Poker (`kuhn_cfr/`)

Solves the classic 3-card, 2-player toy game (J/Q/K) with a 1-chip ante and a single bet/call/fold decision tree. The known Nash equilibrium is reproduced in ~100k iterations.

**Game rules:**
- Each player antes 1 chip and receives one card (J, Q, or K)
- Player 0 acts first: pass or bet (1 chip)
- Player 1 responds; one more action may follow
- Higher card wins at showdown

### 2. River Subgame (`river_cfr/`)

Solves the **river betting round** of heads-up No-Limit Hold'em using a hand-strength abstraction.

**Model:**
- Hand strengths are discretized into `NUM_BUCKETS` buckets (0 = weakest, 9 = strongest); bucket comparison resolves showdowns
- OOP (out-of-position, player 0) acts first
- Configurable pot size (`INITIAL_POT`), effective stack (`STACK`), bet sizes (`BET_FRACS`), and raise cap (`MAX_RAISES`)
- Default: 2 BB pot, 10 BB effective stack, half-pot/pot bet sizes, up to 2 raises

**Payoff convention (relative to river start):**
| Outcome | Player 0 EV |
|---|---|
| P0 folds | −inv₀ |
| P1 folds | INITIAL_POT/2 + inv₁ |
| Showdown P0 wins | INITIAL_POT/2 + inv₁ |
| Showdown P1 wins | −inv₀ |
| Chop | (inv₁ − inv₀) / 2 |

where inv₀, inv₁ are each player's river-street investments.

**Output:** convergence EV and a strategy table for each information set showing action probabilities broken down by hand-strength bucket.

---

## Building

Both solvers use CMake. Build pattern (run from each solver's directory):

```bash
cmake -S . -B build
cmake --build build
./build/kuhn_cfr        # or ./build/river_cfr
```

**Requirements:** C++17-capable compiler (GCC ≥ 7, Clang ≥ 5), CMake ≥ 3.10.

---

## Tuning

| Parameter | File | Default | Effect |
|---|---|---|---|
| `ITERATIONS` | `kuhn_cfr.cpp` | 100 000 | More → tighter convergence |
| `ITERS` | `river_cfr.cpp` | 300 000 | More → tighter convergence |
| `NUM_BUCKETS` | `river_cfr.cpp` | 10 | More buckets → finer hand-strength granularity |
| `INITIAL_POT` | `river_cfr.cpp` | 2.0 BB | Size of pot entering the river |
| `STACK` | `river_cfr.cpp` | 10.0 BB | Effective stack size |
| `BET_FRACS` | `river_cfr.cpp` | {0.5, 1.0} | Available bet sizes as fractions of pot |
| `MAX_RAISES` | `river_cfr.cpp` | 2 | Maximum raises per street |

---

## References

- Zinkevich et al. (2007) — *Regret Minimization in Games with Incomplete Information*
- Neller & Lanctot (2013) — *An Introduction to Counterfactual Regret Minimization*
