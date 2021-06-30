#include "../src/libtris.h"
#include <chrono>
#include <stdio.h>
#include <string>
#include <raylib.h>

// instantiate libtris object
float ghost_colour_alpha = 0.3;
Color tetrimino_colours[14] =   {YELLOW, SKYBLUE, ORANGE, BLUE, GREEN, RED, PURPLE,
                                 Fade(YELLOW, ghost_colour_alpha),
                                 Fade(SKYBLUE, ghost_colour_alpha),
                                 Fade(ORANGE, ghost_colour_alpha),
                                 Fade(BLUE, ghost_colour_alpha),
                                 Fade(GREEN, ghost_colour_alpha),
                                 Fade(RED, ghost_colour_alpha),
                                 Fade(PURPLE, ghost_colour_alpha)};
libtris<Color> tetris(10, 40, 20, 19, tetrimino_colours);

int main(void)
{
    uint16_t game_x = 100;
    uint16_t game_y = 0;
    uint16_t game_w = 0;
    uint16_t game_h = 0;

    const int screen_width      = 800;
    const int screen_height     = 500;
    const int target_framerate  = 60;

    InitWindow(screen_width, screen_height, "libtris raylib frontend");
    SetTargetFPS(target_framerate);
    SetExitKey(0);
    
    Font main_font = LoadFontEx("Lato-Regular.ttf", 20, 0, 250);
    Font large_font = LoadFontEx("Lato-Regular.ttf", 40, 0, 250);
    block_info<Color> **matrix = tetris.getMatrix();

    int block_w = 0, block_h = 0;

    if (screen_width > screen_height)
    {
        const int matrix_height_px  = screen_height * 0.8;
        block_h = matrix_height_px / tetris.getVisibleMatrixHeight();
        block_w = block_h;
    }
    else
    {
        const int matrix_width_px  = screen_width  * 0.7;
        block_w = matrix_width_px / tetris.getMatrixWidth();
        block_h = block_w;
    }

    game_w = block_w * (10 + tetris.getMatrixWidth());
    game_x = std::max(0, (GetScreenWidth() / 2) - (game_w / 2));
    const int matrix_x          = game_x + 65;
    game_h = (block_h * (2 + tetris.getVisibleMatrixHeight()));
    game_y = (GetScreenHeight() / 2) - (game_h / 2);
    const int matrix_y          = game_y + 56;

    uint32_t matrix_w = block_w * tetris.getMatrixWidth();
    uint32_t matrix_h = block_h * tetris.getVisibleMatrixHeight();
    auto frame_start_time = std::chrono::high_resolution_clock::now();
    auto frame_end_time = std::chrono::high_resolution_clock::now();
    uint64_t dt = 0;
    long long microseconds = 0;
    bool paused = false;
    const char *paused_text = "Game \nPaused\nPress ESC \nto resume";
    tetris.startGame();

    // Main game loop
    while (!WindowShouldClose())
    {
        frame_start_time = std::chrono::high_resolution_clock::now();

        // update variables

        // move or rotate current tetrimino
        tetris.setMovingLeft(IsKeyDown(KEY_LEFT));
        tetris.setMovingRight(IsKeyDown(KEY_RIGHT));
        tetris.setMovingDown(IsKeyDown(KEY_DOWN));
        if (IsKeyPressed(KEY_SPACE)) tetris.hardDrop();
        if (IsKeyPressed(KEY_UP)) tetris.rotateClockwise();
        if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_LEFT_SHIFT)) tetris.hold();
        if (IsKeyPressed(KEY_ESCAPE)) paused = !paused;
        if (IsKeyPressed(KEY_F4)) ToggleFullscreen();

        // update tetris
        if (!paused) tetris.update(dt);

        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawTextEx(main_font, "libtris frontend built with raylib", (Vector2){10, 10}, main_font.baseSize, 1, BLACK);
            DrawFPS(GetScreenWidth() - 120, 10);

            // draw stats
            uint16_t stats_y = matrix_y - 56;
            DrawTextEx(main_font, "Score: ", (Vector2){game_x, stats_y}, main_font.baseSize, 1, BLACK);
            DrawTextEx(large_font, std::to_string(tetris.getScore()).c_str(), (Vector2){game_x, stats_y + 20}, large_font.baseSize, 1, BLACK);
            DrawTextEx(main_font, "Level: ", (Vector2){game_x, stats_y + 60}, main_font.baseSize, 1, BLACK);
            DrawTextEx(large_font, std::to_string(tetris.getLevel()).c_str(), (Vector2){game_x, stats_y + 80}, large_font.baseSize, 1, BLACK);
            DrawTextEx(main_font, "Goal: ", (Vector2){game_x, stats_y + 120}, main_font.baseSize, 1, BLACK);
            DrawTextEx(large_font, std::to_string(tetris.getLinesLeft()).c_str(), (Vector2){game_x, stats_y + 140}, large_font.baseSize, 1, BLACK);
            if (tetris.getCombo() > 1)
            {
                DrawTextEx(main_font, "Combo: ", (Vector2){game_x, stats_y + 180}, main_font.baseSize, 1, BLACK);
                DrawTextEx(large_font, std::to_string(tetris.getCombo()).c_str(), (Vector2){game_x, stats_y + 200}, large_font.baseSize, 1, BLACK);
            }
            if (paused) 
            {
                DrawTextEx(
                    main_font,
                    paused_text,
                    (Vector2){game_x, stats_y + 240},
                    main_font.baseSize, 1, BLACK
                );
            }
            if (!tetris.isGameActive()) 
            {
                const char *game_over_text;
                switch(tetris.getGameOverReason())
                {
                    case GAME_OVER_REASON_NONE:
                        game_over_text = "Game\nOver\n(none)";
                        break;
                    case GAME_OVER_REASON_BLOCK_OUT:
                        game_over_text = "Game\nOver\n(block\nout)";
                        break;
                    case GAME_OVER_REASON_LOCK_OUT:
                        game_over_text = "Game\nOver\n(lock\nout)";
                        break;
                    case GAME_OVER_REASON_PARTIAL_LOCK_OUT:
                        game_over_text = "Game\nOver\n(partial\nlock\nout)";
                        break;
                    case GAME_OVER_REASON_GARBAGE_OUT:
                        game_over_text = "Game\nOver\n(garbage\nout)";
                        break;
                    case GAME_OVER_REASON_TOP_OUT:
                        game_over_text = "Game\nOver\n(top\nout)";
                        break;
                }
                DrawTextEx(
                    main_font,
                    game_over_text,
                    (Vector2){10, stats_y + 240},
                    main_font.baseSize, 1, BLACK
                );
            }

            // draw matrix
            for (uint16_t y = 0; y < tetris.getVisibleMatrixHeight(); y++)
            {
                for (uint16_t x = 0; x < tetris.getMatrixWidth(); x++)
                {
                    // filled in bit
                    if (!paused) DrawRectangle(
                        matrix_x + (x * block_w),
                        matrix_y + (y * block_h),
                        block_w, block_h, matrix[x][y+20].colour
                    );

                    // outline
                    Rectangle outline = {
                        matrix_x + (x * block_w),
                        matrix_y + (y * block_h),
                        block_w, block_h};
                    DrawRectangleLinesEx(outline, 1, BLACK);
                }
            }

            // draw hold block
            DrawTextEx(main_font, "Held", (Vector2){matrix_x + matrix_w + block_w, stats_y}, main_font.baseSize, 1, BLACK);
            uint8_t next_block_count = 6;
            uint16_t next_blocks_x = matrix_x + matrix_w + block_w;
            uint16_t next_blocks_h = (2.5 * block_h);
            uint16_t total_next_blocks_h = next_blocks_h * next_block_count;
            block_info<Color> *hold_block = tetris.getHoldBlock();
            for (uint16_t pos = 0; pos < 8; pos++)
            {
                uint8_t x = pos % 4;
                uint8_t y = pos / 4;

                // filled in bit
                if (!paused) DrawRectangle(
                    next_blocks_x + (x * block_w),
                    stats_y + (y * block_h) + 20,
                    block_w, block_h, tetris.getHoldLock() ? Fade(hold_block[pos].colour, 0.7) : hold_block[pos].colour
                );

                // outline
                Rectangle outline = {
                    next_blocks_x + (x * block_w),
                    stats_y + (y * block_h) + 20,
                    block_w, block_h};
                DrawRectangleLinesEx(outline, 1, BLACK);
            }
            free(hold_block);

            // draw next blocks
            DrawTextEx(main_font, "Next", (Vector2){matrix_x + matrix_w + block_w, stats_y + next_blocks_h + 20}, main_font.baseSize, 1, BLACK);

            block_info<Color> **next_blocks = tetris.getNextBlocks(next_block_count);
            for (uint8_t i = 0; i < next_block_count; i++)
            {
                for (uint16_t pos = 0; pos < 8; pos++)
                {
                    uint8_t x = pos % 4;
                    uint8_t y = pos / 4;

                    // filled in bit
                    if (!paused) DrawRectangle(
                        next_blocks_x + (x * block_w),
                        (i *next_blocks_h) + stats_y + (y * block_h) + 20 + next_blocks_h + 20,
                        block_w, block_h, next_blocks[i][pos].colour
                    );

                    // outline
                    Rectangle outline = {
                        next_blocks_x + (x * block_w),
                        (i * next_blocks_h) + stats_y + (y * block_h) + 20 + next_blocks_h + 20,
                        block_w, block_h};
                    DrawRectangleLinesEx(outline, 1, BLACK);
                }
            }

        EndDrawing();

        frame_end_time = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end_time - frame_start_time).count();
    }

    // deinit game
    CloseWindow();

    return 0;
}