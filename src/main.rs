use axum::{
    routing::{get, post},
    Router,
    Json, Extension,
};
use std::net::SocketAddr;
use serde::{Serialize, Deserialize};
use tower_http::cors::{CorsLayer, Any};

use poker_cfrbackend::game::Game; // Assuming your crate is named `poker_cfrbackend` and `game.rs` is modded in `lib.rs`

#[tokio::main]
async fn main() {
    // Initialize your game state. In a real scenario, you might store this in a shared state
    // using `Arc<Mutex<...>>` if you need to mutate it across requests.
    let game = Game::new(100); // 100 BB starting stack, for example

    let cors = CorsLayer::new()
        .allow_methods(Any)
        .allow_origin(Any) // In production, specify your React app's origin
        .allow_headers(Any);

    let app = Router::new()
        .route("/game_state", get(get_game_state))
        .route("/player_action", post(post_player_action))
        .layer(Extension(game))
        .layer(cors);

    let addr = SocketAddr::from(([0,0,0,0], 8080));
    println!("Listening on {}", addr);
    axum::Server::bind(&addr).serve(app.into_make_service()).await.unwrap();
}

// Handler to get game state
async fn get_game_state(Extension(game): Extension<Game>) -> Json<GameStateResponse> {
    // You need to define a response type that matches the data you want to send back.
    // For example, a summary of player states, pot size, etc.
    let state = build_game_state_response(&game);
    Json(state)
}

// Handler to handle player actions (fold, check, call, bet, raise)
#[derive(Deserialize)]
struct PlayerActionRequest {
    player_idx: usize,
    action: String,
    amount: Option<u32>, // only needed for bet/raise
}

#[derive(Serialize)]
struct ActionResponse {
    success: bool,
    message: String,
}

async fn post_player_action(
    Extension(mut game): Extension<Game>,
    Json(req): Json<PlayerActionRequest>
) -> Json<ActionResponse> {
    // Translate the request action string into GameAction enum variant
    let action = match req.action.as_str() {
        "fold" => GameAction::Fold,
        "check" => GameAction::Check,
        "call" => GameAction::Call,
        "bet" => {
            let amt = req.amount.unwrap_or(0);
            GameAction::Bet(amt)
        }
        "raise" => {
            let amt = req.amount.unwrap_or(0);
            GameAction::Raise(amt)
        }
        _ => {
            return Json(ActionResponse {
                success: false,
                message: "Invalid action".to_string(),
            })
        }
    };

    // Apply the action to the game. You might need a method in `Game` that applies an action.
    let result = apply_player_action(&mut game, req.player_idx, action);

    Json(ActionResponse {
        success: result,
        message: if result { "Action applied".to_string() } else { "Action failed".to_string() },
    })
}

// Example: Building a game state response
#[derive(Serialize)]
struct PlayerState {
    name: String,
    chips: u32,
    folded: bool,
    current_bet: u32,
    hand: Vec<String>, // or no hole cards if hidden from the other player
}

#[derive(Serialize)]
struct GameStateResponse {
    pot: u32,
    community_cards: Vec<String>,
    current_player: usize,
    players: Vec<PlayerState>,
}

fn build_game_state_response(game: &Game) -> GameStateResponse {
    GameStateResponse {
        pot: game.pot,
        community_cards: game.community_cards.iter().map(|c| format!("{}", c)).collect(),
        current_player: game.current_player,
        players: game.players.iter().map(|p| PlayerState {
            name: p.name.clone(),
            chips: p.chips,
            folded: p.folded,
            current_bet: p.current_bet,
            hand: p.hand.iter().map(|c| format!("{}", c)).collect(),
        }).collect(),
    }
}

// Example: A function to apply a player action (you must implement logic)
fn apply_player_action(game: &mut Game, player_idx: usize, action: GameAction) -> bool {
    // You must integrate with `game.rs` logic. For example:
    // - If it's not player_idx's turn, return false
    // - Otherwise, call handle_action or something similar
    // This might require modifying your `Game` struct to publicly expose methods 
    // that simulate a single player action. The logic is up to you.

    // Placeholder: return true for now
    true
}

// Ensure `GameAction` is public in `io_interface.rs` if needed. If `GameAction` is from `io_interface`
// you might need to clone or copy it. Possibly derive Clone, Copy for GameAction if you haven't already.
