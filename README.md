# Chess-Sim

A chess simulation engine built in modern C++ with a class-based OOP design.

## Class Design

```
Piece              (abstract base — color, position, symbol, getLegalMoves())
├── Pawn
├── Rook
├── Knight
├── Bishop
├── Queen
└── King

Board              (8×8 grid of Piece*, move execution, legality checks)
Game               (turn management, check/checkmate/stalemate, game loop)
Player             (human or AI base — future: LLM subclass)
```

- `Piece` defines the virtual interface. Every derived piece implements its own movement rules and legal move generation.
- `Board` owns the pieces, validates move legality, and exposes the game state.
- `Game` orchestrates turns, detects game-ending conditions, and drives the loop.

## Features (planned)

- Full legal move generation with pin/check awareness
- Turn enforcement
- Check, checkmate, and stalemate detection
- Move validation (prevents illegal moves)
- Console-based UI

## Future Scope

- **LLM-playable interface**: export board state as FEN/JSON, accept moves from an LLM via stdin or API
- **Pluggable `AIPlayer`**: subclass `Player` so any AI (rule-based, LLM, MCTS) can drop in
- **PGN export**: save and replay games
- **UCI protocol**: compatibility with chess GUIs

## Quick Start

```bash
# Clone and build
git clone https://github.com/<you>/Chess-Sim
cd Chess-Sim
g++ -std=c++17 -o chess-sim src/main.cpp

# Run
./chess-sim
```
