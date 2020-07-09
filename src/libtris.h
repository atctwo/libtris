
#ifndef LIBTRIS_H
#define LIBTRIS_H

// what the hell am i doing???

#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>

/**
 * Describes the rotation of the current tetrimino.
 */
typedef enum RotationStatus {
    ROTATION_SPAWN,                     /**< not rotated, or rotated back to the starting rotation */
    ROTATION_RIGHT,                     /**< rotated 90 degrees right (clockwise) */
    ROTATION_LEFT,                      /**< rotated 90 degrees left (anticlockwise) */
    ROTATION_2,                         /**< rotated 180 degrees from the starting rotation (in any direction) */
    ROTATION_BOTTOM_TEXT                /**< unused rotation that i made when i couldn't get this working */
} RotationStatus;

/**
 * Describes the type of block that the block is???
 */
typedef enum BlockType {
    BLOCK_EMPTY,                        /**< this means that there is nothing in the matrix at this block's position (ie: empty) */
    BLOCK_STATIC,                       /**< this means that the block is part of a tetrimino that has locked down (part of the stack) */
    BLOCK_CURRENT,                      /**< this means that the block is part of the current tetrimino */
    BLOCK_GHOST                         /**< this means that the block is part of the current tetrimino's ghost */
} BlockType;

/**
 * When the game ends, this will describe the reason why the game ended
 */
typedef enum GameOverReason {
    GAME_OVER_REASON_NONE,              /**< the game hasn't ended yet, or the game over reason is unknown */
    GAME_OVER_REASON_BLOCK_OUT,         /**< a tetrimino was spawned on top of another tetrimino */
    GAME_OVER_REASON_LOCK_OUT,          /**< a tetrimino locked down when it was entirely above the visible part of the matrix */
    GAME_OVER_REASON_PARTIAL_LOCK_OUT,  /**< a tetrimino locked down when it was partially above the visible part of the matrix */
    GAME_OVER_REASON_GARBAGE_OUT,       /**< a block is still out of bounds after lines are cleared and garbage is added, currently unused*/
    GAME_OVER_REASON_TOP_OUT            /**< a block was pushed above the vanish zone by garbage, currently unused */
} GameOverReason;

/**
 * Describes when a tetrimino should lock down after landing on the stack
 */
typedef enum LockDownType {
    LOCK_DOWN_TYPE_CLASSIC,             /**< the tetrimino locks down a fixed amount of time after landing on the stack.  the time is determined by the lock delay */
    LOCK_DOWN_TYPE_INFINITE,            /**< same as above, but the lock timer resets whenever the tetrimino is moved or rotated */
    LOCK_DOWN_TYPE_EXTENDED             /**< same as above, but the tetrimino will lock after 15 moves or rotations */
} LockDownType;

/**
 * A struct to contain data on a block that makes up the matrix.
 */
template <class colour_t>
struct block_info {
    colour_t            colour;                     /**< the colour of the block */
    BlockType           block_type;                 /**< the type of the block */
    int                 standard_tetrimino_type;    /**< the index of the first dimension of the array `standard_tetriminoes` that describes the shape of the tetrimino that this block makes up */
    bool                origin;                     /**< whether or not this block is the rotation origin of the tetrmino */
    bool                what_the_hell;              /**< for some reason, this field always wants to be a BlockType, and I can't figure out why */
    RotationStatus      rotation;                   /**< the rotation state of the tetrimino that this block makes up */
};

/**
 * An array that contains the shapes of all 7 of the standard tetriminoes.  This array is used when spawning a new tetrimino.
 * 
 * Here is what each index means:
 * 0. index of the tetrimino
 * 1. y axis
 * 2. x axis
 * 
 * yes it is weird shut up
 */
bool standard_tetriminoes[][2][4] = {

    { // 0: yellow, O
        {0, 1, 1, 0}, // [][]
        {0, 1, 1, 0}  // [][]
    },
    { // 1: cyan, I
        {0, 0, 0, 0}, //
        {1, 1, 1, 1}  // [][][][]
    },
    { // 2: orange, L
        {0, 0, 1, 0}, //     []
        {1, 1, 1, 0}  // [][][]
    },
    { // 3: blue, backwards L
        {1, 0, 0, 0}, // []
        {1, 1, 1, 0}  // [][][]
    },
    { // 4: green, S
        {0, 1, 1, 0}, //   [][]
        {1, 1, 0, 0}  // [][]
    },
    { // 5: red, Z
        {1, 1, 0, 0}, // [][]
        {0, 1, 1, 0}  //   [][]
    },
    { // 6: purple, T
        {0, 1, 0, 0}, //   []
        {1, 1, 1, 0}  // [][][]
    }

};

/**
 * a table that describes how each block should be moved when rotating the tetrimino
 * indicies:
 * 0. row
 * 1. column
 * 2. axis
 * 
 * if rotating clockwise, axis[0] refers to the change in x, and axis[1] refers to the change in y.
 */
int8_t rotation_table[5][5][2] = {
    { {4, 0},   {3, 1},   {2, 2},   {1, 3},   {0, 4}   },
    { {3, -1},  {2, 0},   {1, 1},   {0, 2},   {-1, 3}  },
    { {2, -2},  {1, -1},  {0, 0},   {-1, 1},  {-2, 2}  },
    { {1, -3},  {0, -2},  {-1, -1}, {-2, 0},  {-3, 1}  },
    { {0, -4},  {-1, -3}, {-2, -2}, {-3, -1}, {-4, 0}  }
};

/**
 * this table stores the positions that are tested when trying to perform a wall kick (as part of SRS).
 * 
 * in the reference table that I used (https://tetris.wiki/Super_Rotation_System#Wall_Kicks), the x component
 * is expressed in a "positive x rightwards" format, which is the same as the engine uses.  the y components
 * are expressed in a "positive y upwards" format, which is the opposite of what the engine uses.  as such, all
 * of the y components have been negated (eg: 2 becomes -2)
 * 
 * the positions used depend on what previous rotations have been performed on the tetrimino.  this table
 * uses the same notation used by the tetris wiki article, that is:
 *  0: spawn state
 *  R: rotated clockwise (right) from spawn
 *  L: rotated anticlockwise (left) from spawn
 *  2: rotated twice (180 deg) from spawn in either direction
 * each block stores the rotation state of the tetrimino as part of it's block info.
 * 
 * here are what each of the indexes represent:
 * 0. tetrimino
 * 1. rotation
 * 2. test number
 * 3. axis
 */
int8_t srs_wall_kick_positions[2][8][5][2] = {
    // test number            0         1         2         3         4

    { // 0: J, L, S, T, Z
        { /* 0: 0 -> R */     {0, 0}  , {-1, 0} , {-1, -1}, {0,  2} , {-1,  2} },
        { /* 1: R -> 0 */     {0, 0}  , {1, 0}  , {1, 1}  , {0, -2} , {1, -2}  },
        { /* 2: R -> 2 */     {0, 0}  , {1, 0}  , {1, 1}  , {0, -2} , {1, -2}  },
        { /* 3: 2 -> R */     {0, 0}  , {-1, 0} , {-1, -1}, {0, 2}  , {-1, 2}  },
        { /* 4: 2 -> L */     {0, 0}  , {1, 0}  , {1, -1} , {0, 2}  , {1, 2}   },
        { /* 5: L -> 2 */     {0, 0}  , {-1, 0} , {-1, 1} , {0, -2} , {-1, -2} },
        { /* 6: L -> 0 */     {0, 0}  , {-1, 0} , {-1, 1} , {0, -2} , {-1, -2} },
        { /* 7: 0 -> L */     {0, 0}  , {1, 0}  , {1, -1} , {0, 2}  , {1, 2}   }
    },
    { // 1: I
        { /* 0: 0 -> R */     {0, 0}  , {-2, 0} , {1, 0},   {-2,  1}, {1,  -2} },
        { /* 1: R -> 0 */     {0, 0}  , {2, 0}  , {-1, 0} , {2, -1} , {-1, 2}  },
        { /* 2: R -> 2 */     {0, 0}  , {-1, 0} , {2, 0}  , {-1, -2}, {2, 1}   },
        { /* 3: 2 -> R */     {0, 0}  , {1, 0}  , {-2, 0} , {1, 2}  , {-2, -1} },
        { /* 4: 2 -> L */     {0, 0}  , {2, 0}  , {-1, 0} , {2, -1} , {-1, 2}  },
        { /* 5: L -> 2 */     {0, 0}  , {-2, 0} , {1, 0}  , {-2, 1} , {1, -2}  },
        { /* 6: L -> 0 */     {0, 0}  , {1, 0}  , {-2, 0} , {1, 2}  , {-2, -1} },
        { /* 7: 0 -> L */     {0, 0}  , {-1, 0} , {2, 0}  , {-1, -2}, {2,  1}  }
    }
};

/**
 * this table describes what test set should be used, based on the current rotation state and the rotation direction.
 * 
 * here is what each of the indicies mean:
 * 0. rotation direction (0 means clockwise and 1 means anticlockwise)
 * 1. the current rotation state
 */
uint8_t srs_rotation_wall_kick_map[2][4] = {
    
    /*                        from  0  R  L  2  */
    /*                clockwise */ {0, 2, 6, 4},
    /*  rotation  anticlockwise */ {7, 1, 5, 3}
};

/**
 * describes a rotation state after rotation in a certain direction.
 * 
 * here is what each of the indicies mean:
 * 0. rotation direction (0 means clockwise and 1 means anticlockwise)
 * 1. the current rotation state
 */
RotationStatus srs_rotation_states[2][4] = {
    { ROTATION_RIGHT, ROTATION_2, ROTATION_SPAWN, ROTATION_LEFT },
    { ROTATION_LEFT, ROTATION_SPAWN, ROTATION_2, ROTATION_RIGHT }
};

/**
 * The libtris class, that handles most of the game logic for Tetris.
 */
template <class colour_t>
class libtris
{
private:
    
    uint16_t tetris_das = 133;                      /**< the time (im milliseconds) it takes after a keypress to start the key repeat process  */
    uint16_t tetris_arr = 10;                       /**< the time (in milliseconds) between each automatic key repeat */
    uint16_t tetris_are = 750;                      /**< the time (in milliseconds) between a piece locking and a new piece spawning */
    uint16_t tetris_lock_delay = 500;               /**< the time (in milliseconds) between a piece landing on the stack and locking */
    uint8_t tetris_extended_lock_down_limit = 15;   /**< if the lock down type is extended, this sets the maximum number of moves or rotations that can be made* before the piece locks automatically */
    bool tetris_enable_block_out = true;            /**< if this is true, then the game can be ended when a tetrimino spawns on top of another tetrimino */
    bool tetris_enable_lock_out = true;             /**< if this is true, then the game can be ended when a tetrimino locks completely above the visible zone */
    bool tetris_enable_partial_lock_out = false;    /**< if this is true, then the game can be ended when a tetrimino locks partially above the visible zone */
    uint16_t spawn_row = 0;                         /**< this is the row on which tetrimino are spawned */
    uint16_t buffer_size = 0;                       /**< this is the number of rows that are in the buffer zone */
    bool constant_gravity = false;                  /**< if this is true, then the gravity is determined by the level.  if this is false, then the gravity will always be the same */
    bool enable_ghost = true;                       /**< is this is true, then a ghost piece will be drawn to the matrix */
    bool enable_wall_kick = true;                   /**< if this is true, then the game will use wall kick when a tetrimino is rotated */
    bool use_are_after_locking = true;              /**< this is used to disable ARE when using hard drop */

    uint16_t matrix_width;                          /**< the width of the matrix */
    uint16_t matrix_height;                         /**< the height of the matrix */
    block_info<colour_t> **matrix;                  /**< the matrix itself */
    block_info<colour_t> **rotation_matrix;         /**< a small virtual matrix used to handle tetrimino rotation */
    colour_t *tetrimino_colours;                    /**< the colours of each tetrimino and the ghost tetriminoes */
    uint16_t current_tetrimino_positions[4][2];     /**< the positions of each of the blocks that make up the current tetrimino */
    uint16_t current_tetrimino_origin[2];           /**< the position of the origin block */
    uint16_t **ghost_position;                      /**< ghost?  i can't remember what this is */
    uint8_t tetrimino_bag[14] = {0, 1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5, 6}; /**< two bags that are shuffled to determine the next tetrimino */
    uint8_t next_pointer = 0;                       /**< a pointer to the next bag position */
    uint8_t hold_piece = 255;                       /**< the tetrimino type of the held block (255 = no block held) */
    uint8_t lock_down_moves = 0;                    /**< how many moves a tetrimino has made after landing on the stack */
    
    bool hold_lock = false;                         /**< if this is true, the hold feature is locked, and you can't swap the block */
    bool game_active = false;                       /**< if this is true, then the game isn't over */
    GameOverReason game_over_reason = GAME_OVER_REASON_NONE; /**< the reason why the game ended */
    LockDownType lock_down_type = LOCK_DOWN_TYPE_EXTENDED;   /**< the type of lock down used */

    uint64_t timer_game = 0;                        /**< the length of the game, but not really */
    uint64_t timer_frame = 0;                       /**< a timer that controls the time that a frame has lasted for??? */
    uint64_t timer_das = 0;                         /**< a timer that times how long a key has been held for */
    uint64_t timer_arr = 0;                         /**< a timer that times the time between key repeats */
    uint64_t timer_are = 0;                         /**< a timer that times the time after a piece has locked down */
    uint16_t timer_lock = 0;                        /**< a timer that times how long a piece has been on the stack */
    bool enable_lock_timer = false;                 /**< whether or not the lock timer should be timing */

    uint32_t score = 0;                             /**< the score */
    uint16_t level = 1;                             /**< the level */
    uint16_t line_goal = level * 5;                 /**< the number of lines that have to be cleared to level up */
    uint16_t lines_cleared = 0;                     /**< the number of lines cleared */
    uint16_t combo = 0;                             /**< the number of sequential line clears */

    bool current = false;                           /**< whether or not there is a tetrimino that the player can move */
    bool moving_left = false;                       /**< whether or not the tetrimino should be moving left */
    bool moving_right = false;                      /**< whether or not the tetrimino should be moving right */
    bool moving_down = false;                       /**< whether or not the tetrimino should be moving down */
    bool respawn_flag = false;                      /**< whether or not to spawn a new tetrimino */

    /**
     * Sets the current tetrimino
     * @param standard_tetrimino_type the tetrimino type of the tetrimino to spawn
     */
    void setCurrentTetrimino(int standard_tetrimino_type);

    /**
     * Spawn a new tetrimino based on the next piece in the current bag
     */
    void spawnNewTetrimino();

    /**
     * Rotate the current tetrimino
     * @param anticlockwise if this is false, it will spin clockwise, otherwise it will spin anticlockwise
     */
    void rotate(bool anticlockwise);

    /**
     * Shuffle one of the bags used to keep of what tetrimino should be spawned next
     * @param bag_no what bag to shuffle (0 or 1)
     */
    void shuffleBag(bool bag_no);

    /**
     * Lock the current tetrimino in place
     * @param enable_are if this is true, a delay will be added after the tetrimino locks
     */
    void lockCurrentTetrimino(bool enable_are=true);

    /**
     * Ends the game
     */
    void endGame();

public:

    /**
     * @brief Construct a new libtris object
     * 
     * The width and height and buffer size are specified in "cells".  The Tetris Guidelines specify that the matrix should be 10 cells wide,
     * 20 cells tall, and should have a 20 cell tall buffer zone.  This means that you would specify the width as 10, the height as 40, and 
     * the buffer size as 20.
     * 
     * The tetrimino colours should be an array of 14 elements that describe the colour of each tetrimino and the ghost blocks.  Here is a
     * table that tells you what each array index should be the colour of.
     * 
     * | Index    | Colour   |
     * |----------|----------|
     * | 0        | yellow   |
     * | 1        | cyan     |
     * | 2        | orange   |
     * | 3        | blue     |
     * | 4        | green    |
     * | 5        | red      |
     * | 6        | purple   |
     * 
     * Elements 7 to 13 should be the same as 0 to 6, but saturated or transparent.  These elements are used as the colour of ghost blocks.
     * 
     * @param matrix_width the width of the matrix
     * @param matrix_height the height of the matrix
     * @param buffer_size how many rows at the top of the matrix are invisible and are used as buffer space
     * @param spawn_row the row on which tetriminoes are spawned
     * @param tetrimino_colours an array of the colours that tetrimino should be drawn
     */
    libtris(uint16_t matrix_width, uint16_t matrix_height, uint16_t buffer_size, uint16_t spawn_row, colour_t *tetrimino_colours);

    /**
     * @brief Destroy the libtris object
     */
    ~libtris();

    /**
     * @brief get the matrix width
     * @return uint16_t the matrix width
     */
    uint16_t getMatrixWidth();

    /**
     * @brief get the total matrix height (including both the visible zone and the buffer zone)
     * @return uint16_t the matrix height
     */
    uint16_t getMatrixHeight();

    /**
     * @brief get the height of the visible part of the matrix
     * @return uint16_t the height of the visible part of the matrix
     */
    uint16_t getVisibleMatrixHeight();

    /**
     * @brief get the array that contains the colours of each tetrimino and ghost blocks
     * @return colour_t* the tetrimino colour array
     */
    colour_t *getTetriminoColours();

    /**
     * @brief get the matrix that contains each block
     * @return block_info<colour_t>** the matrix
     */
    block_info<colour_t> **getMatrix();

    /**
     * @brief calculates the ghost block position, and renders it to the matrix.
     * this is done automatically in the `update()` method.
     */
    void getGhostBlocks();

    /**
     * @brief this method clears the current ghost blocks.
     */
    void resetGhostBlocks();

    /**
     * @brief get an array of `block_info`s that represent the blocks that make up the next tetriminoes.
     * this is a complicated method and i'm too tired to explain it right now.
     * @param next_blocks this value determines how many tetriminoes ahead to return.  the maximum number of tetriminoes you can return is 6.
     * @return block_info<colour_t>** the blocks that make up the tetrimino
     */
    block_info<colour_t> **getNextBlocks(uint8_t next_blocks);

    /**
     * @brief this method holds the current tetrimino.
     * if no tetrimino is currently held, the current tetrimino will be held.  if a tetrimino is held, the current and held tetrimino will be swapped.
     * if you hold a tetrimino, the hold system will be "locked" until the current tetrimino is locked.  you can get the status of the hold system
     * using the method `getHoldLock()` 
     */
    void hold();

    /**
     * @brief get the blocks that make up the held tetrimino
     * this method will return an array of `block_info`s that represent the currently held block.  the array is 4 columns by 2 rows, but is accessed lineary.
     * that means that there are 8 blocks that are mapped to indecies 0 to 7.  You can use the following equations to map an index to x (columns) and y (rows) 
     * coordinates:
     * - x = index % 4;
     * - y = index / 4;
     * @return block_info<colour_t>* the blocks that make up the held tetrimino
     */
    block_info<colour_t> *getHoldBlock();

    /**
     * @brief returns whether or not the hold system is locked.
     * if a piece has been held, the hold system "locks" until the current tetrimino has been locked.  this method will return the state of the hold system.
     * @return true the hold system is locked, and you can't hold anything else until the current tetrimino locks.
     * @return false the hold system isn't locked, and you can hold the current tetrimino.
     */
    bool getHoldLock();

    /**
     * @brief this method will check if any lines are full, and will clear them.
     * this method is automatically called before a new tetrimino is spawned.
     */
    void clearLines();

    /**
     * @brief gets the reason that the game ended.
     * when the game ends, the reason that the game ended will be stored.  this method will return the reason that the game ended.  by default (ie: before the game
     * ends), the reason will be GAME_OVER_REASON_NONE, but if this method returns this value, it doesn't neccessarily mean that the game hasn't ended.
     * @return GameOverReason why the game ended
     */
    GameOverReason getGameOverReason();

    /**
     * @brief returns whether or not the game is currently active.
     * when a libtris object is created, the game will be inactive, and this will return false.  if the game is started by calling `startGame()`, this method
     * will return true.  if the game ends, this will return false.
     * @return true the game is active
     * @return false the game is active (hasn't been started, or has ended)
     */
    bool isGameActive();

    /**
     * @brief gets the time between automatic drops due to gravity, in milliseconds
     * this method returns the interval between the current tetrimino automatically dropping due to the game's gravity.  this value is normally dependant on the
     * level, but this can be fixed at 1000 milliseconds by calling `setConstantGravity(true);`
     * @return uint32_t the time between automatic drops due to gravity in ms
     */
    uint32_t getFrameTime();

    /**
     * @brief gets the time between automatic drops due to gravity, in microseconds
     * this method returns the interval between the current tetrimino automatically dropping due to the game's gravity.  this value is normally dependant on the
     * level, but this can be fixed at 1000 microseconds by calling `setConstantGravity(true);`
     * @return uint32_t the time between automatic drops due to gravity in µs
     */
    uint64_t getFrameTime_us();

    /**
     * @brief begins the game.
     * this resets the game state, then starts all of the timers, and starts the game.  when a libtris object is created, the game doesn't automatically start,
     * so this method needs to be called to begin the game.  this method can also be called once the game ends to restart the game.
     */
    void startGame();

    /**
     * @brief update the game
     * this method updates the game's internal timers, ghost block, and everything that is needed to be done once per frame.  this should be called in a loop.
     * @param dt delta time, ie: the time the last frame took to render
     */
    void update(uint64_t dt);

    /**
     * @brief sets the DAS (delayed auto shift)
     * this sets the number of milliseconds after a key is pressed, that the movement caused by the key press is repeated.  this is 133 by default.
     * @param das the DAS time in milliseconds
     */
    void setDAS(uint16_t das);

    /**
     * @brief sets the ARR (auto repeat rate).
     * this sets the time between move repeats when a key is held down (and after a delay specified by the DAS value has elapsed after initially pressing
     * the key).  this is 10 by default.
     * @param arr the ARR interval in milliseconds
     */
    void setARR(uint16_t arr);
    void setARE(uint16_t are);

    /**
     * @brief whether or not to show the ghost piece
     * if this is enabled, a ghost piece will show where the current tetrimino will land.  this is enabled by default.
     * @param ghost whether or not to show the ghost piece.
     */
    void enableGhost(bool ghost);

    /**
     * @brief whether or not SRS wall kick is enabled.
     * if this is enabled, when you rotate a tetrimino, and there is something else in the way, the game will try and place it into another nearby position.
     * the algorithm used for this system is defined by the [Super Rotation System](https://tetris.wiki/Super_Rotation_System).  this is enabled by default.
     * @param wall_kick whether or not wall kick is enabled
     */
    void enableWallKick(bool wall_kick);

    /**
     * @brief sets what type of lock down is used
     * this methods tells the game when a tetrimino should lock down after landing on the stack.  this is set to LOCK_DOWN_TYPE_EXTENDED by default.
     * @param lock_down_type 
     */
    void setLockDownType(LockDownType lock_down_type);

    /**
     * @brief whether or not gravity should be dependant on level
     * in most Tetris games, the gravity (or the speed at which tetriminoes fall) depends on the current level, which depends on how many lines have been cleared.
     * you can use this method to tell the game to disable dynamic gravity.  if dynamic gravity is disabled (ie: constant gravity is enabled), tetriminoes will
     * drop at a fixed rate of 1000 milliseconds.  this is false by default.
     * @param constant_gravity if this is true, constant gravity will be enabled, and the drop rate will be fixed at 1000 ms.  if this is false, the gravity will
     * depend on the level.
     */
    void setConstantGravity(bool constant_gravity);

    /**
     * @brief tells the game whether or not it should end due to a block out.
     * this method tells the game whether or not it should end when a block out occurs (when a tetrimino was spawned on top of another tetrimino).
     * this is enabled by default.
     * @param block_out whether or not the game should end to a block out.
     */
    void enableBlockOut(bool block_out);

    /**
     * @brief tells the game whether or not it should end due to a lock out.
     * this method tells the game whether or not it should end when a lock out occurs (when a tetrimino locked down when it was entirely above the visible part of the matrix).
     * this is enabled by default
     * @param block_out whether or not the game should end to a lock out.
     */
    void enableLockOut(bool lock_out);

    /**
     * @brief tells the game whether or not it should end due to a partial lock out.
     * this method tells the game whether or not it should end when a partial lock out occurs (when a tetrimino locked down when it was partially above the visible part of the matrix).
     * this is disabled by default
     * @param block_out whether or not the game should end to a partial lock out.
     */
    void enablePartialLockOut(bool partial_lock_out);

    /**
     * @brief this tells libtris whether the current tetrimino should be moving left
     * this method directly calls moveLeft(), but considers DAS and ARR (key delay).
     * @param moving whether or not the current tetrimino should be moving left.
     */
    void setMovingLeft(bool moving);

    /**
     * @brief this tells libtris whether the current tetrimino should be moving right
     * this method directly calls moveRight(), but considers DAS and ARR (key delay).
     * @param moving whether or not the current tetrimino should be moving right.
     */
    void setMovingRight(bool moving);

    /**
     * @brief this tells libtris whether the current tetrimino should be moving down
     * this method directly calls moveDown(), but considers DAS and ARR (key delay).  the tetrimino moves down at an interval (defined by ARR), so this method
     * can be used to implement soft drop.
     * @param moving whether or not the current tetrimino should be moving down.
     */
    void setMovingDown(bool moving);

    void moveLeft();                        /**< moves the current tetrimino left by one cell */
    void moveRight();                       /**< moves the current tetrimino right by one cell */
    void moveDown();                        /**< moves the current tetrimino down by one cell, and checks if it has landed on the stack */

    /**
     * @brief immediately moves the current tetrimino to the top of the stack.
     * @param lock if this is true, the tetrimino locks immediately after dropping.  if this is false, the tetrimino doesn't lock, and you can still move it
     */
    void hardDrop(bool lock=true);
    void rotateClockwise();                 /**< rotates the current tetrimino clockwise */
    void rotateAnticlockwise();             /**< rotates the current tetrimino anticlockwise */

    /**
     * @brief set the game level.
     * the level starts at 1, and determines how fast tetriminoes move, and how many points are scored.  many Tetris games let the player choose the starting
     * level before the game begins.  you can get the current level using getLevel().
     * @param level the level
     */
    void setLevel(uint16_t level);

    /**
     * @brief gets the current score
     * the scoring system used is described [here](https://tetris.wiki/Scoring#Recent_guideline_compatible_games).  libtris doesn't detect t-spins or
     * back-to-backs yet.
     * @return uint32_t the current score
     */
    uint32_t getScore();

    /**
     * @brief get the current level
     * the level starts at 1, and determines how fast tetriminoes move, and how many points are scored.  you can set the level using setLevel().
     * @return uint16_t the current level
     */
    uint16_t getLevel();

    /**
     * @brief returns how many total lines need to be cleared to level up
     * this will return how many lines need to have been cleared through the whole game to level up.  for example, if you have cleared 42 lines, this method
     * will return 50 (you need to clear 50 lines to level up).
     * @return uint16_t the number of total lines that need to be cleared to level up
     */
    uint16_t getLineGoal();

    /**
     * @brief returns how many more lines need to be cleared to level up
     * this will return how many more lines you need to clear to level up.  for example, if you have cleared 42 lines, this method will return 8 (you need
     * to clear 8 more lines to level up)
     * @return uint16_t how many more lines need to be cleared to level up
     */
    uint16_t getLinesLeft();

    /**
     * @brief returns the number of lines cleared
     * returns the number of lines cleared by filling a line with blocks.
     * @return uint16_t the number of lines cleared
     */
    uint16_t getLinesCleared();

    /**
     * @brief gets the number of consecutive line clears
     * returns how many consecitive tetrimino drops have resulted in a line clear.  if a tetrimino drop clears a line, the value this method returns is
     * incremented by one, and if the drop doesn't clear a line, the value is reset to zero.
     * @return uint16_t the number of consecutive line clears
     */
    uint16_t getCombo();
};

template <class colour_t>
libtris<colour_t>::libtris(uint16_t matrix_width, uint16_t matrix_height, uint16_t buffer_size, uint16_t spawn_row, colour_t *tetrimino_colours)
{
    // set matrix size
    this->matrix_width = matrix_width;
    this->matrix_height = matrix_height;
    this->buffer_size = buffer_size;
    this->spawn_row = spawn_row;

    // set tetrimino colours
    this->tetrimino_colours = tetrimino_colours;

    // allocate memory for matrix
    this->matrix = (block_info<colour_t>**) malloc(matrix_width * sizeof(block_info<colour_t>*));  // allocate space for an array of pointers (to rows)
    for (uint16_t i = 0; i < this->matrix_width; i++)
    {
        this->matrix[i] = (block_info<colour_t>*) malloc(this->matrix_height * sizeof(block_info<colour_t>));
    }

    // allocate memory for rotation matrix
    this->rotation_matrix = (block_info<colour_t>**) malloc(5 * sizeof(block_info<colour_t>*));  // allocate space for an array of pointers (to rows)
    for (uint16_t i = 0; i < 5; i++)
    {
        this->rotation_matrix[i] = (block_info<colour_t>*) malloc(5 * sizeof(block_info<colour_t>));
    }

    // allocate memory for ghost position
    this->ghost_position = (uint16_t**) malloc(4 * sizeof(uint16_t*));  // allocate space for an array of pointers (to rows)
    for (uint16_t i = 0; i < 4; i++)
    {
        this->ghost_position[i] = (uint16_t*) malloc(2 * sizeof(uint16_t));
    }

    // shuffle bag
    this->shuffleBag(0);
    this->shuffleBag(1);
}

template <class colour_t>
libtris<colour_t>::~libtris()
{
    // free matrix memory
    free(this->matrix);
    free(this->rotation_matrix);
}

template <class colour_t>
void libtris<colour_t>::setCurrentTetrimino(int standard_tetrimino_type)
{
    // calculate tetrimino position
    uint16_t matrix_w_over_2 = (this->matrix_width + 2 - 1) / 2;
    uint16_t container_x = matrix_w_over_2 - 2; // the 2 is derived from the standard tetrimino container width / 2
    uint16_t container_y = this->spawn_row;

    // copy standard tetrimino
    uint8_t block_number = 0;
    for (uint16_t y = 0; y < 2; y++)
    {
        for (uint16_t x = 0; x < 4; x++)
        {
            // if there is a block at the current position
            if (standard_tetriminoes[standard_tetrimino_type][y][x])
            {
                // if there is already a block at this position in the matrix, end the game (block out)
                if (this->tetris_enable_block_out && this->matrix[container_x + x][container_y + y].block_type == BLOCK_STATIC)
                {
                    this->game_over_reason = GAME_OVER_REASON_BLOCK_OUT;
                    this->endGame();
                }

                // copy block info into matrix
                this->matrix[container_x + x][container_y + y] = (block_info<colour_t>){
                    this->tetrimino_colours[standard_tetrimino_type],
                    BLOCK_CURRENT, 
                    standard_tetrimino_type,
                    ((standard_tetrimino_type != 4 && block_number == 2) || (standard_tetrimino_type == 4 && block_number == 0)),
                    BLOCK_EMPTY,
                    ROTATION_SPAWN
                };

                // update current tetrimono position store
                this->current_tetrimino_positions[block_number][0] = container_x + x;
                this->current_tetrimino_positions[block_number][1] = container_y + y;

                // update current tetrimino origin store
                if ((standard_tetrimino_type != 4 && block_number == 2) || (standard_tetrimino_type == 4 && block_number == 0))
                {
                    this->current_tetrimino_origin[0] = container_x + x;
                    this->current_tetrimino_origin[1] = container_y + y;
                }

                // increment block number
                block_number++;
            }
        }
    }

    // set current flag
    this->current = true;
}

template <class colour_t>
void libtris<colour_t>::spawnNewTetrimino()
{
    // printf("next pointer: %d\n", this->next_pointer);
    // printf("bag 0: %d, %d, %d, %d, %d, %d, %d\n", tetrimino_bag[0], tetrimino_bag[1], tetrimino_bag[2], tetrimino_bag[3], tetrimino_bag[4], tetrimino_bag[5], tetrimino_bag[6]);
    // printf("bag 1: %d, %d, %d, %d, %d, %d, %d\n", tetrimino_bag[7], tetrimino_bag[8], tetrimino_bag[9], tetrimino_bag[10], tetrimino_bag[11], tetrimino_bag[12], tetrimino_bag[13]);

    // increment next pointer
    this->next_pointer++;
    if (this->next_pointer > 13) this->next_pointer = 0;

    // if the pointer now points to a different bag, shuffle the previous bag
    if (this->next_pointer == 0) this->shuffleBag(1);
    if (this->next_pointer == 7) this->shuffleBag(0);

    // set tetrimino
    this->setCurrentTetrimino(this->tetrimino_bag[this->next_pointer]);
}

template <class colour_t>
void libtris<colour_t>::shuffleBag(bool bag_no)
{
    // set random seed
    srand(time(NULL));

    // shuffle bag
    uint8_t offset = bag_no * 7;
    for (uint8_t i = 0; i < 7; i++)
    {
        // pick a random element and swap it with the current element
        uint8_t j = rand() % 7;
        uint8_t j_backup = this->tetrimino_bag[offset + j];
        this->tetrimino_bag[offset + j] = this->tetrimino_bag[offset + i];
        this->tetrimino_bag[offset + i] = j_backup;
    }
}

template <class colour_t>
void libtris<colour_t>::lockCurrentTetrimino(bool enable_are)
{
    // create static blocks
    bool lock_out_flag = true;
    for (int8_t i = 0; i < 4; i++)
    {
        uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
        this->matrix[pos[0]][pos[1]].block_type = BLOCK_STATIC;

        // if any of the blocks are in the vanish zone (partial lock out)
        if (this->tetris_enable_partial_lock_out && pos[1] < this->buffer_size)
        {
            this->game_over_reason = GAME_OVER_REASON_PARTIAL_LOCK_OUT;
            this->endGame();
        }
        else lock_out_flag = false;
    }

    // if all of the blocks are in the vanish zone (lock out)
    if (this->tetris_enable_lock_out && lock_out_flag)
    {
        this->game_over_reason = GAME_OVER_REASON_LOCK_OUT;
        this->endGame();
    }

    // clear current flag
    this->current = false;

    // reset hold lock
    this->hold_lock = false;

    // spawn new block
    // ARE will probably be replaced with easy spin at some point
    this->timer_are = enable_are ? 0 : this->tetris_are + 1;
    this->respawn_flag = true;
}

template <class colour_t>
void libtris<colour_t>::endGame()
{
    this->game_active = false;
}

template <class colour_t>
uint16_t libtris<colour_t>::getMatrixWidth()
{
    return this->matrix_width;
}

template <class colour_t>
uint16_t libtris<colour_t>::getMatrixHeight()
{
    return this->matrix_height;
}

template <class colour_t>
uint16_t libtris<colour_t>::getVisibleMatrixHeight()
{
    return this->matrix_height - this->buffer_size;
}

template <class colour_t>
colour_t *libtris<colour_t>::getTetriminoColours()
{
    return this->tetrimino_colours;
}

template <class colour_t>
block_info<colour_t> **libtris<colour_t>::getMatrix()
{
    return this->matrix;
}

template <class colour_t>
void libtris<colour_t>::getGhostBlocks()
{
    uint16_t offset = 0;
    this->resetGhostBlocks();
    while(1)
    {
        // determine if there's anything under the current tetrimino
        bool idk = false;
        for (int8_t i = 0; i < 4; i++)
        {
            uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
            // if the tetrimino has reached the bottom of the matrix
            if (pos[1] + offset + 1 >= this->matrix_height) idk = true;
            // if there is a static block beneath any of the blocks that make up the current tetrimino
            else if (this->matrix[pos[0]][pos[1]+offset+1].block_type == BLOCK_STATIC) idk = true;
        }

        if (idk)
        {
            // create ghost blocks
            for (int8_t i = 0; i < 4; i++)
            {
                uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
                if (this->matrix[pos[0]][pos[1]+offset].block_type == BLOCK_EMPTY) this->matrix[pos[0]][pos[1]+offset] = (block_info<colour_t>) {
                    this->tetrimino_colours[this->matrix[pos[0]][pos[1]].standard_tetrimino_type + 7],
                    BLOCK_GHOST,
                    this->matrix[pos[0]][pos[1]].standard_tetrimino_type,
                    false,
                    BLOCK_EMPTY,
                    ROTATION_SPAWN
                };
                this->ghost_position[i][0] = pos[0];
                this->ghost_position[i][1] = pos[1] + offset;
            }

            break;
        }
        else
        {
            offset++;
        }
    }
}

template <class colour_t>
void libtris<colour_t>::resetGhostBlocks()
{
    for (uint8_t i = 0; i < 4; i++)
    {
        block_info<colour_t> *info = &this->matrix[this->ghost_position[i][0]][this->ghost_position[i][1]];
        if (info->block_type == BLOCK_GHOST) 
        {
            *info = (block_info<colour_t>){NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, BLOCK_EMPTY, ROTATION_SPAWN};
        }
    }
}

template <class colour_t>
block_info<colour_t> **libtris<colour_t>::getNextBlocks(uint8_t next_blocks)
{
    uint8_t internal_next_blocks = next_blocks;
    if (next_blocks > 6) internal_next_blocks = 6;
    uint8_t current_block = this->next_pointer + 1;
    block_info<colour_t> **blocks = (block_info<colour_t>**) malloc(internal_next_blocks * sizeof(block_info<colour_t>*));
    // printf("getting next %d blocks\n", internal_next_blocks);

    for (uint8_t i = 0; i < internal_next_blocks; i++)
    {
        if (current_block > 13) current_block = 0;
        // printf("next piece %d\n", i);
        blocks[i] = (block_info<colour_t>*) malloc(8 * sizeof(block_info<colour_t>));
        for (uint8_t y = 0; y < 2; y++)
        {
            for (uint8_t x = 0; x < 4; x++)
            {
                uint16_t pos = x + (y * 4);
                // printf("current block: %d\n", current_block);
                if (standard_tetriminoes[this->tetrimino_bag[current_block]][y][x])
                {
                    blocks[i][pos] = (block_info<colour_t>) {
                        this->tetrimino_colours[this->tetrimino_bag[current_block]],
                        BLOCK_CURRENT, this->tetrimino_bag[current_block], false, BLOCK_EMPTY, ROTATION_SPAWN
                    };
                }
                else 
                {
                    blocks[i][pos] = (block_info<colour_t>) {
                        NULL, BLOCK_EMPTY, this->tetrimino_bag[current_block], false, BLOCK_EMPTY, ROTATION_SPAWN
                    };
                }
            }
        }

        current_block++;
    }

    return blocks;
}


template <class colour_t>
void libtris<colour_t>::hold()
{
    if (this->current && this->game_active)
    {
        if (!this->hold_lock)
        {
            if (this->hold_piece == 255)
            {
                this->hold_piece = this->matrix[this->current_tetrimino_positions[0][0]][this->current_tetrimino_positions[0][1]].standard_tetrimino_type;
                for (uint8_t i = 0; i < 4; i++) this->matrix[this->current_tetrimino_positions[i][0]][this->current_tetrimino_positions[i][1]] = (block_info<colour_t>) {
                    NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN
                };
                this->spawnNewTetrimino();
            }
            else
            {
                uint8_t held_tetrimino = this->hold_piece;
                this->hold_piece = this->matrix[this->current_tetrimino_positions[0][0]][this->current_tetrimino_positions[0][1]].standard_tetrimino_type;
                for (uint8_t i = 0; i < 4; i++) this->matrix[this->current_tetrimino_positions[i][0]][this->current_tetrimino_positions[i][1]] = (block_info<colour_t>) {
                    NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN
                };
                this->setCurrentTetrimino(held_tetrimino);
            }
            this->hold_lock = true;
        }
    }
    
}

template <class colour_t>
block_info<colour_t> *libtris<colour_t>::getHoldBlock()
{
    block_info<colour_t> *hold_block = (block_info<colour_t>*) malloc(8 * sizeof(block_info<colour_t>));
    if (this->hold_piece == 255)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            hold_block[i] = (block_info<colour_t>){NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
        }
    }
    else
    {
        for (uint8_t y = 0; y < 2; y++)
        {
            for (uint8_t x = 0; x < 4; x++)
            {
                uint16_t pos = x + (y * 4);
                //printf("current block: %d\n", current_block);
                if (standard_tetriminoes[this->hold_piece][y][x])
                {
                    hold_block[pos] = (block_info<colour_t>) {
                        this->tetrimino_colours[this->hold_piece],
                        BLOCK_CURRENT, this->hold_piece, false, BLOCK_EMPTY, ROTATION_SPAWN
                    };
                }
                else 
                {
                    hold_block[pos] = (block_info<colour_t>) {
                        NULL, BLOCK_EMPTY, this->hold_piece, false, BLOCK_EMPTY, ROTATION_SPAWN
                    };
                }
            }
        }
    }
    return hold_block;
}

template <class colour_t>
bool libtris<colour_t>::getHoldLock()
{
    return this->hold_lock;
}

template <class colour_t>
void libtris<colour_t>::clearLines()
{
    uint16_t current_line = this->matrix_height - 1;
    uint8_t cleared_lines = 0;
    while(1)
    {
        //printf("checking line %d\n", current_line);

        // check if the line is full
        bool full = true;
        for (uint16_t x = 0; x < this->matrix_width; x++)
        {
            if (this->matrix[x][current_line].block_type != BLOCK_STATIC)  full = false;
        }

        // if the line is full, clear the line, and move every block above down by 1
        if (full)
        {
            //printf("line full\n");
            cleared_lines++;

            this->resetGhostBlocks();

            // clear the line
            for (uint16_t x = 0; x < this->matrix_width; x++) this->matrix[x][current_line] = (block_info<colour_t>){NULL, BLOCK_EMPTY, 0, false};

            // move every block above the line down by 1
            for (int16_t y = current_line-1; y >= 0; y--)
            {
                //printf("moving line %d", y);
                for (uint16_t x = 0; x < this->matrix_width; x++)
                {
                    this->matrix[x][y+1] = this->matrix[x][y];
                    this->matrix[x][y] = (block_info<colour_t>) {NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
                }
            }

            // todo: add point system
            continue;
        }

        if (current_line == 0) break;
        else current_line--;
    }

    // update score
    if      (cleared_lines == 1) this->score += 100 * this->level; // single
    else if (cleared_lines == 2) this->score += 300 * this->level; // double
    else if (cleared_lines == 3) this->score += 500 * this->level; // triple
    else if (cleared_lines == 4) this->score += 800 * this->level; // tetris

    if (this->combo > 0)
    {
        this->score += 50 * this->combo * this->level;
    }

    if (cleared_lines == 0) this->combo = 0;
    else this->combo++;    

    // update lines cleared
    //printf("Lines cleared: %d", cleared_lines);
    this->lines_cleared += cleared_lines;
    if (this->lines_cleared >= this->line_goal)
    {
        this->level++;
        this->line_goal += this->level * 5;
    }
}

template <class colour_t>
GameOverReason libtris<colour_t>::getGameOverReason()
{
    return this->game_over_reason;
}

template <class colour_t>
bool libtris<colour_t>::isGameActive()
{
    return this->game_active;
}

template <class colour_t>
uint32_t libtris<colour_t>::getFrameTime()
{
    // https://tetris.fandom.com/wiki/Tetris_Worlds#Gravity
    uint32_t frame = pow( 0.8 - ((this->level - 1) * 0.007), this->level - 1) * 1000;
    return (this->constant_gravity) ? 1000 : frame;
}

template <class colour_t>
uint64_t libtris<colour_t>::getFrameTime_us()
{
    // https://tetris.fandom.com/wiki/Tetris_Worlds#Gravity
    uint64_t frame = pow( 0.8 - ((this->level - 1) * 0.007), this->level - 1) * 1000000;
    return (this->constant_gravity) ? 1000000 : frame;
}

template <class colour_t>
void libtris<colour_t>::startGame()
{
    // clear matrix
    for (uint16_t y = 0; y < this->matrix_height; y++)
    {
        for (uint16_t x = 0; x < this->matrix_width; x++)
        {
            this->matrix[x][y] = (block_info<colour_t>){NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
        }
    }

    // set game over reason
    this->game_over_reason = GAME_OVER_REASON_NONE;

    // place tetrimino
    this->spawnNewTetrimino();

    // set game active flag
    this->game_active = true;
}

template <class colour_t>
void libtris<colour_t>::update(uint64_t dt)
{
    if (this->game_active)
    {
        //printf("[tetris] frame update\n");

        // update timers
        this->timer_game += dt;
        this->timer_frame += dt;
        this->timer_das += dt;
        this->timer_arr += dt;
        this->timer_are += dt;
        this->timer_lock += dt;

        // handle frame timer
        if (this->current)
        {
            if (this->timer_frame > this->getFrameTime())
            {
                this->moveDown();
                this->timer_frame = 0;
            }
        }

        // handle das and arr timers
        if (this->moving_left)
        {
            if (this->timer_das > this->tetris_das)
            {
                if (this->timer_arr > this->tetris_arr)
                {
                    this->moveLeft();
                    this->timer_arr = 0;
                }
            }
        }
        if (this->moving_right)
        {
            if (this->timer_das > this->tetris_das)
            {
                if (this->timer_arr > this->tetris_arr)
                {
                    this->moveRight();
                    this->timer_arr = 0;
                }
            }
        }
        if (this->moving_down)
        {
            if (this->timer_arr > this->tetris_arr)
            {
                this->moveDown();
                this->timer_arr = 0;
            }
        }

        // handle tetrimino lock timer and check lock down moves
        if (this->enable_lock_timer)
        {
            if (this->timer_lock > this->tetris_lock_delay)
            {
                this->lockCurrentTetrimino(this->use_are_after_locking);
                this->timer_lock = 0;
                this->enable_lock_timer = false;
            }
            else
            {
                // determine if there's anything under the current tetrimino
                bool idk = false;
                for (int8_t i = 0; i < 4; i++)
                {
                    uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
                    // if the tetrimino has reached the bottom of the matrix
                    if (pos[1] + 1 >= this->matrix_height) idk = true;
                    // if there is a static block beneath any of the blocks that make up the current tetrimino
                    else if (this->matrix[pos[0]][pos[1]+1].block_type == BLOCK_STATIC) idk = true;
                }
                if (!idk) this->enable_lock_timer = false;
            }

            if (this->lock_down_type == LOCK_DOWN_TYPE_EXTENDED)
            {
                if (this->lock_down_moves > this->tetris_extended_lock_down_limit)
                {
                    this->lockCurrentTetrimino(this->use_are_after_locking);
                    this->timer_lock = 0;
                    this->enable_lock_timer = false;
                }
            }
            
        }

        // handle tetrimino respawn
        if (this->respawn_flag)
        {
            if (this->timer_are > this->tetris_are)
            {
                // todo: add delay after clearing lines
                this->clearLines();
                this->spawnNewTetrimino();
                this->respawn_flag = false;
            }
        }

        // update ghost blocks
        if (this->enable_ghost) this->getGhostBlocks();

    }
}


template <class colour_t>
void libtris<colour_t>::setDAS(uint16_t das)
{
    this->tetris_das = das;
}

template <class colour_t>
void libtris<colour_t>::setARR(uint16_t arr)
{
    this->tetris_arr = arr;
}

template <class colour_t>
void libtris<colour_t>::setARE(uint16_t are)
{
    this->tetris_are = are;
}

template <class colour_t>
void libtris<colour_t>::enableGhost(bool ghost)
{
    this->enable_ghost = ghost;
}

template <class colour_t>
void libtris<colour_t>::enableWallKick(bool wall_kick)
{
    this->enable_wall_kick = wall_kick;
}

template <class colour_t>
void libtris<colour_t>::setLockDownType(LockDownType lock_down_type)
{
    this->lock_down_type = lock_down_type;
}

template <class colour_t>
void libtris<colour_t>::setConstantGravity(bool constant_gravity)
{
    this->constant_gravity = constant_gravity;
}

template <class colour_t>
void libtris<colour_t>::enableBlockOut(bool block_out)
{
    this->tetris_enable_block_out = block_out;
}

template <class colour_t>
void libtris<colour_t>::enableLockOut(bool lock_out)
{
    this->tetris_enable_lock_out = lock_out;
}

template <class colour_t>
void libtris<colour_t>::enablePartialLockOut(bool partial_lock_out)
{
    this->tetris_enable_partial_lock_out = partial_lock_out;
}

template <class colour_t>
void libtris<colour_t>::setMovingLeft(bool moving)
{
    // if moving is true, and the flag was previously false
    if (moving && !this->moving_left)
    {
        this->timer_das = 0;
        this->timer_arr = 0;
        this->moveLeft();
    }

    // set moving flag
    this->moving_left = moving;
}

template <class colour_t>
void libtris<colour_t>::setMovingRight(bool moving)
{
    // if moving is true, and the flag was previously false
    if (moving && !this->moving_right)
    {
        this->timer_das = 0;
        this->timer_arr = 0;
        this->moveRight();
    }

    // set moving flag
    this->moving_right = moving;
}

template <class colour_t>
void libtris<colour_t>::setMovingDown(bool moving)
{
    // if moving is true, and the flag was previously false
    if (moving && !this->moving_down)
    {
        this->timer_arr = 0;
        this->moveDown();
    }

    // set moving flag
    this->moving_down = moving;
}

template <class colour_t>
void libtris<colour_t>::moveLeft()
{
    if (this->current && this->game_active)
    {
        // determine if there is something to the left of any block
        bool idk = false;
        for (int8_t i = 0; i < 4; i++)
        {
            uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
            // if the block is at the left most side of the matrix
            if (pos[0] == 0) idk = true;
            // if there is a static block to the left of this block
            else if (this->matrix[pos[0]-1][pos[1]].block_type == BLOCK_STATIC) idk = true;
        }

        // move current tetrimino
        if (!idk)
        {
            // move blocks
            for (int8_t i = 0; i < 4; i++)
            {
                // get block position
                uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
                //std::cout << this->matrix[pos[0]][pos[1]].standard_tetrimino_type;

                // copy block info to next space
                this->matrix[pos[0] - 1][pos[1]] = this->matrix[pos[0]][pos[1]];

                // update current block
                this->matrix[pos[0]][pos[1]] = (block_info<colour_t>) {NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
                pos[0]--;
            }

            // update tetrimino orign
            this->current_tetrimino_origin[0]--;

            // update lock down timer
            if (this->lock_down_type == LOCK_DOWN_TYPE_EXTENDED || this->lock_down_type == LOCK_DOWN_TYPE_INFINITE)
            {
                this->timer_lock = 0;
                this->lock_down_moves++;
            }
        }
    }
}

template <class colour_t>
void libtris<colour_t>::moveRight()
{
    if (this->current && this->game_active)
    {
        // determine if there is something to the right of any block
        bool idk = false;
        for (int8_t i = 0; i < 4; i++)
        {
            uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
            // if the block is at the right most side of the matrix
            if (pos[0] + 1 >= this->matrix_width) idk = true;
            // if there is a static block to the right of this block
            else if (this->matrix[pos[0]+1][pos[1]].block_type == BLOCK_STATIC) idk = true;
        }

        // move current tetrimino
        if (!idk)
        {
            // update blocks
            for (int8_t i = 3; i >= 0; i--)
            {
                // get block position
                uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
                //std::cout << this->matrix[pos[0]][pos[1]].standard_tetrimino_type;

                // copy block info to next space
                this->matrix[pos[0] + 1][pos[1]] = this->matrix[pos[0]][pos[1]];

                // update current block
                this->matrix[pos[0]][pos[1]] = (block_info<colour_t>) {NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
                pos[0]++;
            }

            // update tetrimino origin
            this->current_tetrimino_origin[0]++;

            // update lock down timer
            if (this->lock_down_type == LOCK_DOWN_TYPE_EXTENDED || this->lock_down_type == LOCK_DOWN_TYPE_INFINITE)
            {
                this->timer_lock = 0;
                this->lock_down_moves++;
            }
        }
    }
}

template <class colour_t>
void libtris<colour_t>::moveDown()
{
    if (this->current && this->game_active)
    {
        // determine if there's anything under the current tetrimino
        bool idk = false;
        for (int8_t i = 0; i < 4; i++)
        {
            uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
            // if the tetrimino has reached the bottom of the matrix
            if (pos[1] + 1 >= this->matrix_height) idk = true;
            // if there is a static block beneath any of the blocks that make up the current tetrimino
            else if (this->matrix[pos[0]][pos[1]+1].block_type == BLOCK_STATIC) idk = true;
        }

        if (idk)
        {
            if (!this->enable_lock_timer)
            {
                this->enable_lock_timer = true;
                this->use_are_after_locking = true;
                this->timer_lock = 0;
                this->lock_down_moves = 0;
            }
        }
        else
        {
            // move current tetrimino
            for (int8_t i = 3; i >= 0; i--)
            {
                // get block position
                uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
                
                // copy block info to next space
                this->matrix[pos[0]][pos[1] + 1] = this->matrix[pos[0]][pos[1]];

                // update current block
                this->matrix[pos[0]][pos[1]] = (block_info<colour_t>) {NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
                pos[1]++;
            }

            this->current_tetrimino_origin[1]++;
            if (this->moving_down) this->score += 1;
            this->enable_lock_timer = false;
        }
    }
}

template <class colour_t>
void libtris<colour_t>::hardDrop(bool lock)
{
    if (this->current && this->game_active)
    {
        while(1)
        {
            // determine if there's anything under the current tetrimino
            bool idk = false;
            for (int8_t i = 0; i < 4; i++)
            {
                uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
                // if the tetrimino has reached the bottom of the matrix
                if (pos[1] + 1 >= this->matrix_height) idk = true;
                // if there is a static block beneath any of the blocks that make up the current tetrimino
                else if (this->matrix[pos[0]][pos[1]+1].block_type == BLOCK_STATIC) idk = true;
            }

            if (idk)
            {
                if (!this->enable_lock_timer)
                {
                    this->enable_lock_timer = true;
                    this->timer_lock = (lock) ? this->tetris_lock_delay + 1 : 0;
                    this->use_are_after_locking = !lock;
                    this->lock_down_moves = 0;
                }
                break;
            }
            else
            {
                // move current tetrimino
                for (int8_t i = 3; i >= 0; i--)
                {
                    // get block position
                    uint16_t (&pos)[2] = this->current_tetrimino_positions[i];

                    // copy block info to next space
                    this->matrix[pos[0]][pos[1] + 1] = this->matrix[pos[0]][pos[1]];

                    // update current block
                    this->matrix[pos[0]][pos[1]] = (block_info<colour_t>) {NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
                    pos[1]++;
                }
                this->score += 2;
            }
        }
    }
}

template <class colour_t>
void libtris<colour_t>::rotateClockwise()
{
    this->rotate(false);
}

template <class colour_t>
void libtris<colour_t>::rotateAnticlockwise()
{
    this->rotate(true);
}

template <class colour_t>
void libtris<colour_t>::rotate(bool anticlockwise)
{
    if (this->current && this->game_active)
    {
        uint16_t virtual_block_positions[4][2];

        // clear the virtual matrix
        for (uint8_t y = 0; y < 5; y++)
        {
            for (uint8_t x = 0; x < 5; x++)
            {
                this->rotation_matrix[x][y] = (block_info<colour_t>) {NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
            }
        }
        
        // copy rotated positions to virtual matrix
        for (uint8_t i = 0; i < 4; i++)
        {
            //printf("block %d\n", i+1);

            // get block position
            uint16_t (&pos)[2] = this->current_tetrimino_positions[i];
            
            // get the block's offset from the tetrimino origin (offset by 2)
            int16_t dx = pos[0] - this->current_tetrimino_origin[0] + 2;
            int16_t dy = pos[1] - this->current_tetrimino_origin[1] + 2;
            
            //printf("\tdx: %d (%d)\n\tdy: %d (%d)\n", dx-2, dx, dy-2, dy);

            // get the rotated position
            //printf("\ttx: %d\n\tty: %d\n", rotation_table[dy][dx][0], rotation_table[dy][dx][1]);
            int16_t rx = dx + rotation_table[dy][dx][0];
            int16_t ry = dy + rotation_table[dy][dx][1];
            
            //printf("\trx: %d\n\try: %d\n", rx, ry);

            // copy the block into the virtual matrix
            this->rotation_matrix[rx][ry] = this->matrix[pos[0]][pos[1]];
            this->rotation_matrix[rx][ry].rotation = srs_rotation_states[0][this->rotation_matrix[rx][ry].rotation];
            virtual_block_positions[i][0] = rx;
            virtual_block_positions[i][1] = ry;
        }
        


        // for (uint8_t y = 0; y < 5; y++)
        // {
        //     for (uint8_t x = 0; x < 5; x++)
        //     {
        //         if (this->rotation_matrix[x][y].block_type != BLOCK_EMPTY) printf("[]");
        //         else printf("..");
        //     }
        //     printf("\n");
        // }


        // copy the virtual matrix to the real matrix
        uint16_t rotation_matrix_map_pos_x = this->current_tetrimino_origin[0] - 2;
        uint16_t rotation_matrix_map_pos_y = this->current_tetrimino_origin[1] - 2;

        // srs wall kick tests
        // declare i outside of the loop so that it can be used after the loop
        // for i = 0 to 5 (tests)
        //     for j = 0 to 4 (blocks in virtual matrix)
        //         calculate block position
        //             make sure to use the L table for the L blocks (sorry L blocks)
        //         check if block occupies space with a static block
        //         if yes, continue (go to next test)
        //         if no, break the loop, and use i as the index of the successful test
        // if i == 5
        //     all tests failed, don't rotate
        // else
        //    the successful test index is stored in i
        //    copy the rotated blocks to the matrix, using the successful test as an offset

        uint8_t test_no = 0;
        int8_t test_data[2];
        bool break_flag = false;

        if (this->enable_wall_kick)
        {
            for (test_no; test_no < 5; test_no++)
            {
                bool test_failed = false;
                //printf("wall kick test %d\n", test_no);
                for (uint8_t i = 0; i < 4; i++)
                {
                    //printf("\tblock %d\n", i);
                    // get test group
                    uint16_t (&unrotated_pos)[2] = this->current_tetrimino_positions[i];
                    bool test_group = 0;
                    if (this->matrix[unrotated_pos[0]][unrotated_pos[1]].standard_tetrimino_type == 1) test_group = 1;
                    //printf("\t\ttest group: %d\n", test_group);

                    // get rotation
                    RotationStatus test_rotation_from = this->matrix[unrotated_pos[0]][unrotated_pos[1]].rotation;
                    uint8_t test_rotation_action = srs_rotation_wall_kick_map[0][test_rotation_from];
                    //printf("\t\trotation action: %d\n", test_rotation_action);

                    // get data for test
                    test_data[0] = srs_wall_kick_positions[test_group][test_rotation_action][test_no][0];
                    test_data[1] = srs_wall_kick_positions[test_group][test_rotation_action][test_no][1];
                    //printf("\t\toffset x: %d, offset y: %d\n", test_data[0], test_data[1]);

                    // calculate block position
                    int16_t abs_block_x = rotation_matrix_map_pos_x + virtual_block_positions[i][0] + test_data[0];
                    int16_t abs_block_y = rotation_matrix_map_pos_y + virtual_block_positions[i][1] + test_data[1];
                    //printf("\t\tabsolute x: %d, absolute y: %d\n", abs_block_x, abs_block_y);

                    //printf("\t\thello\n");
                    // check if there is already a block here (in the real matrix)
                    bool out_of_bounds = false;
                    if (abs_block_x < 0 || abs_block_x >= this->matrix_width) out_of_bounds = true;// block is outside the matrix
                    //printf("\t\tis block out of bounds? %d\n", out_of_bounds);

                    bool is_block_full = false;
                    if (!out_of_bounds) 
                    {
                        is_block_full = (this->matrix[abs_block_x][abs_block_y].block_type == BLOCK_STATIC);   // there is a static block in that space
                        //printf("is block full? %d\n", is_block_full);
                    }
                                
                    //printf("\t\tis space full / out of bounds? %d\n", out_of_bounds || is_block_full);
                    if (out_of_bounds || is_block_full)
                    {
                        test_failed = true;
                        break;
                    }
                }
                if (!test_failed) 
                {
                    //printf("test %d succeeded\n", test_no);
                    break;
                }
                //else printf("\ttest %d failed\n", test_no);
            }

            if (test_no > 4)
            {
                // all tests failed, don't rotate
                //printf("all tests failed (test no: %d)\n", test_no);
                return;
            }
            //printf("copying blocks from virtual matrix to real matrix\n");
            //printf("using test %d (x: %d, y: %d)\n", test_no, test_data[0], test_data[1]);

        }
        else
        {
            test_data[0] = 0;
            test_data[1] = 0;
        }
        

        uint8_t block_no = 0;
        for (uint8_t y = 0; y < 5; y++)
        {
            for (uint8_t x = 0; x < 5; x++)
            {
                uint16_t abs_block_x = rotation_matrix_map_pos_x + x + test_data[0];
                uint16_t abs_block_y = rotation_matrix_map_pos_y + y + test_data[1];
                
                // if block is in bounds
                if (abs_block_x >= 0 && abs_block_x < this->matrix_width)
                {

                    // if there is a current block in the real matrix
                    if (this->matrix[abs_block_x][abs_block_y].block_type == BLOCK_CURRENT)
                    {
                        this->matrix[abs_block_x][abs_block_y] = (block_info<colour_t>){NULL, BLOCK_EMPTY, 0, false, BLOCK_EMPTY, ROTATION_SPAWN};
                    }

                    // if there is a current block in the virtual matrix
                    if (this->rotation_matrix[x][y].block_type == BLOCK_CURRENT)
                    {
                        this->matrix[abs_block_x][abs_block_y] = this->rotation_matrix[x][y];
                        this->current_tetrimino_positions[block_no][0] = abs_block_x;
                        this->current_tetrimino_positions[block_no][1] = abs_block_y;
                        if (this->matrix[abs_block_x][abs_block_y].origin)
                        {
                            current_tetrimino_origin[0] = abs_block_x;
                            current_tetrimino_origin[1] = abs_block_y;
                        }
                        block_no++;
                    }

                }
            }
        }

        // update lock down timer
        if (this->lock_down_type == LOCK_DOWN_TYPE_EXTENDED || this->lock_down_type == LOCK_DOWN_TYPE_INFINITE)
        {
            this->timer_lock = 0;
            this->lock_down_moves++;
        }
    }
}

template <class colour_t>
void libtris<colour_t>::setLevel(uint16_t level)
{
    this->level = level;
}

template <class colour_t>
uint32_t libtris<colour_t>::getScore()
{
    return this->score;
}

template <class colour_t>
uint16_t libtris<colour_t>::getLevel()
{
    return this->level;
}

template <class colour_t>
uint16_t libtris<colour_t>::getLineGoal()
{
    return this->line_goal;
}

template <class colour_t>
uint16_t libtris<colour_t>::getLinesLeft()
{
    return this->line_goal - this->lines_cleared;
}

template <class colour_t>
uint16_t libtris<colour_t>::getLinesCleared()
{
    return this->lines_cleared;
}

template <class colour_t>
uint16_t libtris<colour_t>::getCombo()
{
    return this->combo;
}

#endif /* LIBTRIS_H */