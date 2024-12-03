// src/main.rs

mod cards;
mod hand_evaluator;

use cards::{Card, Deck};
use hand_evaluator::{evaluate_hand, HandRank};

fn main() {
    // Create a new deck and shuffle it
    let mut deck = Deck::new();
    deck.shuffle();

    // Number of players
    let num_players = 4;

    // Deal two cards to each player (Texas Hold'em)
    let mut players_hands: Vec<Vec<Card>> = Vec::new();
    for _ in 0..num_players {
        let mut hand = Vec::new();
        for _ in 0..2 {
            if let Some(card) = deck.deal() {
                hand.push(card);
            }
        }
        players_hands.push(hand);
    }

    // Deal five community cards
    let mut community_cards = Vec::new();
    for _ in 0..5 {
        if let Some(card) = deck.deal() {
            community_cards.push(card);
        }
    }

    // Display the community cards
    println!("Community Cards:");
    for card in &community_cards {
        println!("{}", card);
    }
    println!();

    // Evaluate each player's best hand
    let mut best_hand_ranks = Vec::new();

    for (i, hand) in players_hands.iter().enumerate() {
        // Combine player's hand with community cards
        let mut full_hand = hand.clone();
        full_hand.extend(community_cards.clone());

        // Find the best hand rank from all possible 5-card combinations
        let best_hand_rank = find_best_hand(&full_hand);

        // Store the player's best hand rank
        best_hand_ranks.push((i, best_hand_rank.clone()));

        // Display the player's hand and best hand rank
        println!("Player {}'s Hand:", i + 1);
        for card in hand {
            println!("{}", card);
        }
        println!("Best Hand Rank: {:?}\n", best_hand_rank);
    }

    // Determine the winner
    best_hand_ranks.sort_by(|a, b| b.1.cmp(&a.1)); // Sort descending by hand rank

    // Get the highest hand rank
    let winning_rank = best_hand_ranks[0].1.clone();

    // Find all players who have the winning rank (possible tie)
    let winners: Vec<_> = best_hand_ranks
        .iter()
        .filter(|(_, rank)| *rank == winning_rank)
        .collect();

    // Announce the winner(s)
    if winners.len() == 1 {
        println!(
            "Player {} wins with a {:?}!",
            winners[0].0 + 1,
            winners[0].1
        );
    } else {
        let winner_ids: Vec<String> = winners.iter().map(|(i, _)| format!("{}", i + 1)).collect();
        println!(
            "Players {} tie with a {:?}!",
            winner_ids.join(", "),
            winning_rank
        );
    }
}

// Helper function to find the best hand rank from all possible 5-card combinations
fn find_best_hand(cards: &[Card]) -> HandRank {
    let combinations = get_five_card_combinations(cards);

    combinations
        .into_iter()
        .map(|hand| evaluate_hand(&hand))
        .max()
        .unwrap()
}

// Function to generate all possible 5-card combinations from a set of cards
fn get_five_card_combinations(cards: &[Card]) -> Vec<Vec<Card>> {
    let mut combinations = Vec::new();
    let card_count = cards.len();

    let indices = (0..card_count).collect::<Vec<_>>();

    for i1 in 0..(card_count - 4) {
        for i2 in (i1 + 1)..(card_count - 3) {
            for i3 in (i2 + 1)..(card_count - 2) {
                for i4 in (i3 + 1)..(card_count - 1) {
                    for i5 in (i4 + 1)..card_count {
                        let hand = vec![
                            cards[indices[i1]],
                            cards[indices[i2]],
                            cards[indices[i3]],
                            cards[indices[i4]],
                            cards[indices[i5]],
                        ];
                        combinations.push(hand);
                    }
                }
            }
        }
    }

    combinations
}
