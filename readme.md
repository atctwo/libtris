# libtris

This is a kind of library that attempts to provide a portable Tetris engine.  If you want to get Tetris running on something, but you don't want to write the game logic, you could use this library so that you only have to build a frontend for the game.  The library consists of a single header file, which provides a class, which provides methods you can use to interact with the game.  Included is an example of how the engine could be used to create a Tetris clone using [raylib](https://www.raylib.com/).

# Todo

- example code in readme
- `getMatrix()` and `getNextBlocks()` specific explanations in the documentation
- that's total garbage! + garbage out and top out
- anticlockwise rotation
- rotation at the bottom of the matrix
- back to back and t-spin recognition
- methods for getting game stats (like number of combos, LPM, etc)
- methods for setting lock delay and extended lockdown maximum moves

# Building

libtris only depends on 4 standard library libraries (`stdlib.h`, `inttypes.h`, `time.h`, and `math.h`).  The entire library is contained in one header file, so it will be compiled with whatever file includes it.

Details on how to compile the example are contained in the `examples/` directory.

The documentation can be built using Doxygen.  You can build the documentation by running `doxygen Doxyfile`.

# Tetris Guidelines

There is a document that specifies how official Tetris games should be implemented, and a lot of is documentated online (for example, at [this Tetris Wiki](https://tetris.wiki/Tetris_Guideline)).  Here is a quick run through of what parts of the Guidelines libtris implements, and what needs to be implemented when writing a frontend.

## Logo
libtris doesn't make use of the Tetris logo (it's not an official Tetris game),  it's probably protected by lots of legal stuff

## Super Rotation System (sometimes called SRS)
libtris (mostly) implements this entirely within the library, so you don't have to do anything.  The tetrimino rotations aren't exactly the same as required by SRS.  Wall kick can be disabled by using the `enableWallKick()` method.

## Tetrimino starting positions
The starting position (and matrix size) are specified by the user when a libtris object is instantiated.  The guidelines specify that tetriminoes spawn on rows 21 and 22.  The raylib example implements this.

## Lock Down
You can pick between three lock down systems.  Classic lock down means that a piece locks down after a fixed period of time after landing on the stack (0.5 seconds by default).  Infinite lock down means that you can still move the piece after it lands on the stack, but it will lock when the piece is kept still for a certain amount of time (0.5s by default).  Extended lock down (which is default) is the same is Infinite lockdown, except that you can only move the piece a certain number of times before it locks (which is 15 by default).  You can set the lock down type by calling `setLockDownType()`.

## Hold
You can hold the current tetrimino by calling the `hold()` method.  Once you hold a tetrimino, you can't use the hold system until the current tetrimino locks.  You can get the state of the hold system by calling `getHoldLock()`, and you can get the blocks that make up the held tetrimino by calling `getHoldBlock()`.

## Piece preview
You can preview up to the next six pieces that will be spawned by using the `getNextBlocks()` method.

## Playfield (sometimes called matrix)
It's up to the user to determine the size of the playfield.  The guidelines specify that the playfield should be 10x20, with an extra 20 cell tall buffer zone above the playfield.  To acheive the buffer zone, you can tell the library to create a playfield that is 10x40, and that the first 20 rows are buffer rows.  You can get the width and height of the playfield using `getMatrixWidth()` and `getMatrixHeight()`, and you can get the size of the playfield that should be visible (not buffer space) by using `getVisibleMatrixHeight()`.

## Piece colours
Because the datatype that represents colour is provided by the user, the user also has to specify the colours of each piece (and each piece ghost) when initalising a libtris object.

## Random Generator
Tetriminoes aren't randomly generated one after the other.  The game kind of puts each of the tetriminoes into a bag, then shuffles the bag, so you will always get the same set of tetriminoes but in a different order.  The library uses two "bags" so that you can get previews of the next pieces, so if you need to know what piece will be spawned outside of the bag, you can find out.  my words are *very* messed up, i am tired :)

## Ghost piece
The library includes a ghost piece system, which can be enabled or disabled by calling `enableGhost()`.

## Controller mappings
The controller mappings aren't handled by the library, and have to be implemented by the user.

## Timings
The game timings are handled by the library, and are based on the ones used in Tetris Worlds.  The library has an `update()` method, to which you have to pass a value called "delta time", which is the time that the last frame took to render (i think).  You can set DAS, ARR, and ARE by using `setDAS()`, `setARR()`, and `setARE()`.

## Levels and Scoring
The library handles levels and scoring.  The scoring system is based on the one that is used by most games released after Tetris DS (described [here](https://tetris.wiki/Scoring#Recent_guideline_compatible_games)).  The level is determined by clearing lines.  To level up, you need to clear a number of lines that can be determined by multiplying the current level by 5.  Combos have been implemented, but T-Spins and Back-to-Backs haven't.  The user can set the level using `setLevel()`.

## Music
The library doesn't include any music (or sound effects).

## Game over conditions
The game can be ended due to a number of things.  When a game ends, the reason the game ended can be retrieved by calling `getGameOverReason()`.

### Block out
The game will end when a tetrimino spawns inside of a block on the stack.  This is enabled by default, and can be enabled or disabled by calling `enableBlockOut()`.

### Lock out
The game will end when a tetrimino locks entirely within the vanish zone (above the visible playfield).  This is enabled by default, and can be enabled or disabled by calling `enableLockOut()`.

### Partial lock out
The game will end when a tetrimino locks partially within the vanish zone.  This is disabled by default, and can be enabled or disabled by calling `enablePartialLockOut()`.

## Garbage
There is no multiplayer mode, so there is no garbage system.  In saying that, there's nothing stopping you creating two instances of libtris, so having a garbage method might be a good idea.

## T-Spin detection
I haven't done this yet!! haha!