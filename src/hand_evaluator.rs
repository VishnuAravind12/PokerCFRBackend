// src/hand_evaluator.rs

use crate::cards::{Card, Rank, Suit};
use std::collections::HashMap;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub enum HandRank {
    HighCard(Rank),
    OnePair(Rank),
    TwoPair(Rank, Rank),
    ThreeOfAKind(Rank),
    Straight(Rank),
    Flush(Rank),
    FullHouse(Rank, Rank),
    FourOfAKind(Rank),
    StraightFlush(Rank),
    RoyalFlush,
}

pub fn evaluate_hand(cards: &[Card]) -> HandRank {
    let mut ranks: Vec<Rank> = cards.iter().map(|card| card.rank).collect();
    ranks.sort_by(|a, b| b.cmp(a)); // Sort ranks descending

    let is_flush = cards.iter().all(|card| card.suit == cards[0].suit);
    let is_straight = is_consecutive(&ranks);

    let rank_counts = get_rank_counts(&ranks);

    match (is_flush, is_straight, &ranks[..]) {
        (true, true, [Rank::Ace, Rank::King, Rank::Queen, Rank::Jack, Rank::Ten, ..]) => {
            HandRank::RoyalFlush
        }
        (true, true, _) => HandRank::StraightFlush(ranks[0]),
        (_, _, _) if has_four_of_a_kind(&rank_counts) => {
            let quad_rank = get_n_of_a_kind_rank(&rank_counts, 4);
            HandRank::FourOfAKind(quad_rank)
        }
        (_, _, _) if has_full_house(&rank_counts) => {
            let trip_rank = get_n_of_a_kind_rank(&rank_counts, 3);
            let pair_rank = get_n_of_a_kind_rank(&rank_counts, 2);
            HandRank::FullHouse(trip_rank, pair_rank)
        }
        (true, false, _) => HandRank::Flush(ranks[0]),
        (false, true, _) => HandRank::Straight(ranks[0]),
        (_, _, _) if has_three_of_a_kind(&rank_counts) => {
            let trip_rank = get_n_of_a_kind_rank(&rank_counts, 3);
            HandRank::ThreeOfAKind(trip_rank)
        }
        (_, _, _) if has_two_pair(&rank_counts) => {
            let high_pair = get_high_pair_rank(&rank_counts);
            let low_pair = get_low_pair_rank(&rank_counts);
            HandRank::TwoPair(high_pair, low_pair)
        }
        (_, _, _) if has_one_pair(&rank_counts) => {
            let pair_rank = get_n_of_a_kind_rank(&rank_counts, 2);
            HandRank::OnePair(pair_rank)
        }
        _ => HandRank::HighCard(ranks[0]),
    }
}

fn is_consecutive(ranks: &[Rank]) -> bool {
    let mut sorted_ranks = ranks.to_vec();
    sorted_ranks.sort();

    // Handle the wheel (A-2-3-4-5) straight
    if sorted_ranks == vec![Rank::Two, Rank::Three, Rank::Four, Rank::Five, Rank::Ace] {
        return true;
    }

    for i in 0..(sorted_ranks.len() - 1) {
        if (sorted_ranks[i + 1] as u8) != (sorted_ranks[i] as u8 + 1) {
            return false;
        }
    }
    true
}

fn get_rank_counts(ranks: &[Rank]) -> HashMap<Rank, usize> {
    let mut counts = HashMap::new();
    for &rank in ranks {
        *counts.entry(rank).or_insert(0) += 1;
    }
    counts
}

fn has_four_of_a_kind(rank_counts: &HashMap<Rank, usize>) -> bool {
    rank_counts.values().any(|&count| count == 4)
}

fn has_full_house(rank_counts: &HashMap<Rank, usize>) -> bool {
    rank_counts.values().any(|&count| count == 3)
        && rank_counts.values().any(|&count| count == 2)
}

fn has_three_of_a_kind(rank_counts: &HashMap<Rank, usize>) -> bool {
    rank_counts.values().any(|&count| count == 3)
}

fn has_two_pair(rank_counts: &HashMap<Rank, usize>) -> bool {
    rank_counts.values().filter(|&&count| count == 2).count() == 2
}

fn has_one_pair(rank_counts: &HashMap<Rank, usize>) -> bool {
    rank_counts.values().any(|&count| count == 2)
}

fn get_n_of_a_kind_rank(rank_counts: &HashMap<Rank, usize>, n: usize) -> Rank {
    rank_counts
        .iter()
        .filter(|&(_, &count)| count == n)
        .map(|(&rank, _)| rank)
        .max()
        .unwrap()
}

fn get_high_pair_rank(rank_counts: &HashMap<Rank, usize>) -> Rank {
    rank_counts
        .iter()
        .filter(|&(_, &count)| count == 2)
        .map(|(&rank, _)| rank)
        .max()
        .unwrap()
}

fn get_low_pair_rank(rank_counts: &HashMap<Rank, usize>) -> Rank {
    rank_counts
        .iter()
        .filter(|&(_, &count)| count == 2)
        .map(|(&rank, _)| rank)
        .min()
        .unwrap()
}
