# Piece classes, Board integration, and movement restrictions

How to get from `char board[8][8]` to a real `Piece` hierarchy without the renderer
noticing, and how to layer movement rules on top of it.

Read in order. Each section builds on the last.

---

## 0. The one decision that matters: who owns a piece's position

Your README says `Piece` holds `position`. **Don't do that.** The board grid already
encodes position — a piece at `grid[3][4]` *is* on d5. If the piece also stores
`(3,4)`, you now have two sources of truth, and every move has to update both. Miss
one update and you get a piece that renders on d5 but generates moves from e2. This is
the single most common bug in a homemade chess engine, and it is unforced.

**Rule: the grid owns position. A piece never knows where it is.** Where a piece
needs its own square (and it does, to generate moves), the board passes it in:

```cpp
p->pseudoMoves(board, r, c, out);   // "you are at (r,c). what can you do?"
```

Everything below follows from this.

---

## 1. What the base class should contain

### It must have

| Member | Why |
|---|---|
| `Color color` | Decides move direction, what's capturable, whose turn. Needed by every rule. |
| `bool hasMoved` | Castling rights and the pawn's two-square opening both depend on it. Cannot be derived from the board alone. |
| `virtual char letter() const` | `'k' 'q' 'r' 'b' 'n' 'p'`. The only per-class piece of identity data. |
| `virtual void pseudoMoves(...) const` | The piece's movement pattern. The whole point of the hierarchy. |
| `virtual ~Piece()` | You will delete through a `Piece*`. Without this it is undefined behavior. |

### It must NOT have

- **`position`** — see section 0.
- **A `Board*` back-pointer.** Pass the board into the one method that needs it. A
  back-pointer means a piece can't be tested in isolation and creates a reference cycle
  to reason about.
- **`points` / `value`** (pawn=1, queen=9). Nothing needs it until you write an AI, and
  when you do it's a free function: `int value(char sym)`. YAGNI.
- **`isAlive` / `isCaptured`.** A captured piece is *removed from the grid*. A bool that
  must agree with the grid is a second source of truth — the same bug as `position`.
- **Sprite / texture / color-to-draw info.** That's the renderer's job, and it already
  derives everything from `symbol()`.
- **Move history.** That belongs to `Game`.

### The header

```cpp
// Piece.h
#pragma once
#include <vector>

enum class Color { White, Black };

struct Move {
    int fromR, fromC, toR, toC;
};

class Board;   // forward declare — Piece only needs a reference

class Piece {
public:
    explicit Piece(Color c) : color(c) {}
    virtual ~Piece() = default;

    Color color;
    bool  hasMoved = false;

    virtual char letter() const = 0;   // lowercase: 'k','q','r','b','n','p'

    // Every square this piece could move to, ignoring whether it exposes its own
    // king. "Pseudo" = pattern-legal, not necessarily legal. See section 5.
    virtual void pseudoMoves(const Board& b, int r, int c,
                             std::vector<Move>& out) const = 0;

    // Squares this piece ATTACKS. Identical to pseudoMoves for everything except
    // the pawn, which pushes forward but does not attack forward. See section 5.3.
    virtual void attacks(const Board& b, int r, int c,
                         std::vector<Move>& out) const {
        pseudoMoves(b, r, c, out);
    }

    // Case encodes color, so the renderer keeps working untouched.
    char symbol() const {
        char l = letter();
        return color == Color::White ? char(l - 32) : l;   // 'r' -> 'R'
    }

protected:
    // The two helpers that collapse five of the six pieces. See section 2.
    void slide(const Board&, int r, int c, const int dirs[][2], int n,
               std::vector<Move>& out) const;
    void hop  (const Board&, int r, int c, const int offs[][2], int n,
               std::vector<Move>& out) const;
};

inline bool inBounds(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }
```

---

## 2. Two helpers do almost all the work

Chess pieces move in exactly two ways:

- **Sliders** (rook, bishop, queen) walk a direction until they hit something.
- **Hoppers** (knight, king) jump to a fixed set of offsets, once.

Write those two loops *once*, in the base class. Then each subclass is a direction
table and nothing else. If you write the "walk until blocked" loop three times, you
will fix the same off-by-one three times.

```cpp
// Piece.cpp
void Piece::slide(const Board& b, int r, int c, const int dirs[][2], int n,
                  std::vector<Move>& out) const {
    for (int i = 0; i < n; i++) {
        for (int k = 1; k < 8; k++) {
            int nr = r + dirs[i][0] * k;
            int nc = c + dirs[i][1] * k;
            if (!inBounds(nr, nc)) break;

            const Piece* t = b.at(nr, nc);
            if (!t) {                                  // empty: keep going
                out.push_back({r, c, nr, nc});
            } else {
                if (t->color != color)                 // enemy: capture it...
                    out.push_back({r, c, nr, nc});
                break;                                 // ...but stop either way
            }
        }
    }
}

void Piece::hop(const Board& b, int r, int c, const int offs[][2], int n,
                std::vector<Move>& out) const {
    for (int i = 0; i < n; i++) {
        int nr = r + offs[i][0];
        int nc = c + offs[i][1];
        if (!inBounds(nr, nc)) continue;

        const Piece* t = b.at(nr, nc);
        if (!t || t->color != color)                   // empty or enemy
            out.push_back({r, c, nr, nc});
    }
}
```

That `break` after a capture is the entire concept of "blocking." A rook can't jump
over a piece because the loop stops the moment it meets one.

---

## 3. The six subclasses

Five of them are three lines. This is what the base-class helpers buy you.

```cpp
// Pieces.h
#include "Piece.h"

class Rook : public Piece {
public:
    using Piece::Piece;
    char letter() const override { return 'r'; }
    void pseudoMoves(const Board& b, int r, int c, std::vector<Move>& out) const override {
        static const int D[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        slide(b, r, c, D, 4, out);
    }
};

class Bishop : public Piece {
public:
    using Piece::Piece;
    char letter() const override { return 'b'; }
    void pseudoMoves(const Board& b, int r, int c, std::vector<Move>& out) const override {
        static const int D[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
        slide(b, r, c, D, 4, out);
    }
};

class Queen : public Piece {
public:
    using Piece::Piece;
    char letter() const override { return 'q'; }
    void pseudoMoves(const Board& b, int r, int c, std::vector<Move>& out) const override {
        static const int D[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};
        slide(b, r, c, D, 8, out);   // a queen is a rook and a bishop. Same loop.
    }
};

class Knight : public Piece {
public:
    using Piece::Piece;
    char letter() const override { return 'n'; }
    void pseudoMoves(const Board& b, int r, int c, std::vector<Move>& out) const override {
        static const int O[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
        hop(b, r, c, O, 8, out);
    }
};

class King : public Piece {
public:
    using Piece::Piece;
    char letter() const override { return 'k'; }
    void pseudoMoves(const Board& b, int r, int c, std::vector<Move>& out) const override {
        static const int O[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
        hop(b, r, c, O, 8, out);
        // castling is added here later — see section 6.
    }
};
```

The pawn is the only piece that earns real code. It is the only one that moves and
captures differently, that cares about direction, and that has a first-move bonus.

```cpp
class Pawn : public Piece {
public:
    using Piece::Piece;
    char letter() const override { return 'p'; }

    void pseudoMoves(const Board& b, int r, int c, std::vector<Move>& out) const override {
        // Row 0 is rank 8 (your setupBoard puts black there), so White moves toward
        // row 0 — i.e. dir = -1.
        int dir = (color == Color::White) ? -1 : +1;
        int one = r + dir;

        if (inBounds(one, c) && !b.at(one, c)) {
            out.push_back({r, c, one, c});

            int two = r + 2 * dir;                       // only from the start rank...
            if (!hasMoved && inBounds(two, c) && !b.at(two, c))
                out.push_back({r, c, two, c});           // ...and only if BOTH squares
        }                                                //    are empty (note the nesting)

        // captures: diagonal only, and only if an enemy is actually there
        for (int dc : {-1, +1}) {
            int nc = c + dc;
            if (!inBounds(one, nc)) continue;
            const Piece* t = b.at(one, nc);
            if (t && t->color != color)
                out.push_back({r, c, one, nc});
        }
    }

    // A pawn ATTACKS the diagonals whether or not anything is standing there,
    // and never attacks straight ahead. This distinction matters — section 5.3.
    void attacks(const Board& b, int r, int c, std::vector<Move>& out) const override {
        int one = r + ((color == Color::White) ? -1 : +1);
        for (int dc : {-1, +1})
            if (inBounds(one, c + dc))
                out.push_back({r, c, one, c + dc});
    }
};
```

Note the double-push is nested *inside* the single-push check. A pawn on e2 with a
piece on e3 cannot jump to e4. Writing the two checks as siblings is a classic bug.

---

## 4. Board: swap `char` for `unique_ptr<Piece>`

The grid owns the pieces, so it should own them literally. `unique_ptr` means a
captured piece is freed when you overwrite its square — no `delete`, no leak, no
double-free.

```cpp
// Board.h
#include <memory>
#include <vector>
#include "Piece.h"

class Board {
private:
    std::unique_ptr<Piece> grid[8][8];

public:
    Board();
    void setupBoard();
    void displayBoard() const;

    // What Piece code asks for.
    const Piece* at(int r, int c) const { return grid[r][c].get(); }   // nullptr = empty

    // What the RENDERER asks for. Same char encoding as before: 'R', 'p', '.'
    char symbolAt(int r, int c) const {
        return grid[r][c] ? grid[r][c]->symbol() : '.';
    }

    bool movePiece(int fromR, int fromC, int toR, int toC);
    bool movePiece(std::string from, std::string to);   // keep — just re-parse and delegate

    std::vector<Move> legalMoves(int r, int c);         // section 5
    bool isSquareAttacked(int r, int c, Color by) const;
    bool inCheck(Color side) const;
};
```

Setup builds objects instead of copying chars:

```cpp
void Board::setupBoard() {
    for (auto& row : grid) for (auto& sq : row) sq.reset();

    auto backRank = [&](int r, Color col) {
        grid[r][0] = std::make_unique<Rook>(col);
        grid[r][1] = std::make_unique<Knight>(col);
        grid[r][2] = std::make_unique<Bishop>(col);
        grid[r][3] = std::make_unique<Queen>(col);
        grid[r][4] = std::make_unique<King>(col);
        grid[r][5] = std::make_unique<Bishop>(col);
        grid[r][6] = std::make_unique<Knight>(col);
        grid[r][7] = std::make_unique<Rook>(col);
    };

    backRank(0, Color::Black);
    backRank(7, Color::White);
    for (int c = 0; c < 8; c++) {
        grid[1][c] = std::make_unique<Pawn>(Color::Black);
        grid[6][c] = std::make_unique<Pawn>(Color::White);
    }
}
```

And the move itself — note it's now a `bool`, and it sets `hasMoved`:

```cpp
bool Board::movePiece(int fromR, int fromC, int toR, int toC) {
    for (const Move& m : legalMoves(fromR, fromC))
        if (m.toR == toR && m.toC == toC) {
            grid[toR][toC] = std::move(grid[fromR][fromC]);  // captured piece freed here
            grid[toR][toC]->hasMoved = true;
            return true;
        }
    return false;                                            // illegal — board unchanged
}
```

`displayBoard()` changes one line: print `symbolAt(i, j)` instead of `board[i][j]`.

### The renderer does not change

This is the payoff of `symbol()` keeping the case convention. In `render.cpp`, one
character:

```cpp
if (atlasCell(board.symbolAt(r, c), ac, ar)) {   // was board.at(r, c)
```

`atlasCell` still gets `'R'` / `'p'` / `'.'`, still maps case to the atlas row.
The rendering code never learns that pieces became objects — which is exactly what
you want from the abstraction. Delete `char at(int,int)` from `Board.h` once you've
switched; it's the old thing wearing the new thing's name.

---

## 5. Movement restrictions, in three layers

Legality is not one check. It is three, and they must run in this order.

### 5.1 Pattern — "can this piece move like that?"

That's `pseudoMoves()`. Already done, section 3.

### 5.2 Blocking and capture — "is the path clear, and is the target mine?"

Already done too — it's the `break` in `slide()` and the `t->color != color` in both
helpers. This is why they live in the base class: these rules are identical for every
piece, so they should exist in exactly one place.

### 5.3 King safety — "does this move expose my own king?"

This is the layer everyone skips, and it's what makes chess *chess*. You may not make
a move that leaves your king attacked. That covers pins (a bishop can't step aside if
it's the only thing shielding the king) and check evasion (if you're in check, your
only legal moves are the ones that end the check) with a single rule.

The way to check it is to **make the move, look, and take it back**:

```cpp
bool Board::inCheck(Color side) const {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            const Piece* p = at(r, c);
            if (p && p->color == side && p->letter() == 'k')
                return isSquareAttacked(r, c, side == Color::White ? Color::Black
                                                                   : Color::White);
        }
    return false;   // no king on the board (only in tests)
}

bool Board::isSquareAttacked(int r, int c, Color by) const {
    std::vector<Move> mv;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++) {
            const Piece* p = at(i, j);
            if (!p || p->color != by) continue;

            mv.clear();
            p->attacks(*this, i, j, mv);          // <-- attacks(), NOT legalMoves()
            for (const Move& m : mv)
                if (m.toR == r && m.toC == c) return true;
        }
    return false;
}

std::vector<Move> Board::legalMoves(int r, int c) {
    const Piece* p = at(r, c);
    if (!p) return {};

    std::vector<Move> pseudo, legal;
    p->pseudoMoves(*this, r, c, pseudo);

    for (const Move& m : pseudo) {
        // make
        std::unique_ptr<Piece> captured = std::move(grid[m.toR][m.toC]);
        grid[m.toR][m.toC] = std::move(grid[m.fromR][m.fromC]);

        bool exposed = inCheck(p->color);

        // unmake — exactly inverse, and it must be, or the board silently corrupts
        grid[m.fromR][m.fromC] = std::move(grid[m.toR][m.toC]);
        grid[m.toR][m.toC] = std::move(captured);

        if (!exposed) legal.push_back(m);
    }
    return legal;
}
```

**Two traps here, and they will both bite you:**

1. **`isSquareAttacked` must call `attacks()` / `pseudoMoves()`, never `legalMoves()`.**
   Think about what happens otherwise: `legalMoves` → `inCheck` → `isSquareAttacked` →
   `legalMoves` → infinite recursion, stack overflow. Attack detection *does not care*
   whether the attacker would be exposing its own king — an illegally-pinned bishop
   still stops your king from stepping onto that diagonal. Pseudo-legal is the correct
   notion here, not a shortcut.

2. **This is why `attacks()` exists separately from `pseudoMoves()`.** A pawn's forward
   push is a *move* but not an *attack*. If `isSquareAttacked` used `pseudoMoves`, a
   white pawn on e4 would appear to "attack" e5 — and your king could never legally
   stand on e5 even though a pawn cannot capture forward. Every other piece can use the
   default `attacks() = pseudoMoves()`, which is why it's a virtual with a body rather
   than a pure virtual.

Once `legalMoves` is right, checkmate and stalemate are nearly free:

```cpp
// side has no legal move anywhere:
//   in check  -> checkmate
//   not in check -> stalemate
```

### Wire it back into the GUI

In `render.cpp` there's a `ponytail:` comment marking the spot. `movePiece` now returns
`bool` and rejects illegal moves itself, so the click handler needs no new logic — an
illegal click just does nothing. To highlight legal destinations, call
`board.legalMoves(selRow, selCol)` when a piece is selected and tint those squares.

---

## 6. Special moves and the state they need

These are the reason `hasMoved` is on the base class, and the reason you'll eventually
need one more field on `Board`.

| Move | State required | Where it lives |
|---|---|---|
| **Pawn double-step** | `hasMoved` | already on `Piece` |
| **Castling** | king's `hasMoved`, rook's `hasMoved`, plus: king not currently in check, and the two squares it crosses are not attacked (`isSquareAttacked` — you already have it) | `Piece` + `King::pseudoMoves` |
| **En passant** | the *previous move* — you can only capture en passant immediately | a `Move lastMove` member on `Board` |
| **Promotion** | none; it's just "pawn reached row 0 or 7" | inside `Board::movePiece` — replace the `unique_ptr` with a new `Queen` |

Add these **after** section 5 works. Castling in particular depends on
`isSquareAttacked` already being correct, and there's no point debugging castling
through a broken check detector.

---

## 7. Turn enforcement

Not a `Board` concern and not a `Piece` concern. `Board` answers "is this move legal
in isolation"; whose turn it is, is a *game* fact.

```cpp
class Game {
    Board board;
    Color turn = Color::White;
public:
    bool tryMove(int fr, int fc, int tr, int tc) {
        const Piece* p = board.at(fr, fc);
        if (!p || p->color != turn) return false;      // not your piece, not your turn
        if (!board.movePiece(fr, fc, tr, tc)) return false;
        turn = (turn == Color::White) ? Color::Black : Color::White;
        return true;
    }
};
```

The renderer then holds a `Game`, not a `Board`, and calls `tryMove`. That's the last
piece of the "one side can move twice" problem I flagged earlier.

---

## 8. Build order

Do them in this sequence. Each step leaves you with a program that runs, which means
when something breaks you know what broke it.

1. `Piece.h` + `slide`/`hop` + all six subclasses. **Nothing else changes yet** —
   the pieces compile and are unused.
2. Swap `Board`'s grid to `unique_ptr<Piece>`, add `symbolAt()`, fix `displayBoard()`,
   change one line in `render.cpp`. **The GUI should look identical.** If it doesn't,
   your setup or your `symbol()` case convention is wrong — fix it here, where there
   are only two suspects.
3. Add `legalMoves()` *without* the king-safety filter (just return pseudo-moves) and
   have `movePiece` consult it. Now pieces move like pieces. Rooks stop at blockers,
   knights jump, pawns push. Play with it.
4. Add `isSquareAttacked` / `inCheck` and turn on the king-safety filter. Now pins and
   check work. Verify: put your king on e1, a rook on e2, an enemy rook on e8 — e2 must
   have no legal moves.
5. Add `Game` and turn enforcement.
6. Only now: castling, en passant, promotion.

### One check worth writing

Before you trust any of it, run a **perft** — count the leaf nodes of the move tree at
depth N from the starting position. The numbers are fixed and published, so it's a
total correctness test of move generation in about fifteen lines:

```cpp
long perft(Board& b, Color side, int depth);   // depth 1 -> 20, 2 -> 400, 3 -> 8902
```

If depth 3 gives you 8902, your movement rules are almost certainly right. If it gives
you 8903, something is subtly wrong and you'd never have found it by playing. This is
the single highest-value test in the whole project — worth more than any number of unit
tests on individual pieces.
