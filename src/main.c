#include <math.h>
#include <raylib.h>
#include <rlgl.h>
#include <time.h>

#include "stb_ds.h"

#define SCALE 0.75f
#define STEP_INTERVAL 0.1f
#define BASE_WIDTH 1920
#define BASE_HEIGHT 1080
#define WINDOW_WIDTH    ((int) BASE_WIDTH *SCALE)
#define WINDOW_HEIGHT   ((int) BASE_HEIGHT *SCALE)
#define GAME_WIDTH    ((int) (BASE_WIDTH/2 )) // 960
#define GAME_HEIGHT   ((int) (BASE_HEIGHT /2))
#define ROWS 28
#define COLUMNS 52
#define TILE_SIZE 16
#define TILE_SPACING 2

#define GRID_OFFSET_X 12
#define GRID_OFFSET_Y 35

typedef enum
{
    EMPTY_TILE,
    VISITED_TILE,
    PLAYER_TILE,
    FOOD_TILE,
    CLONE_TILE,
    CLONE_AND_PLAYER_TILE,
} TileState;

typedef struct
{
    float timer;
    float angle;
    TileState state;
    bool visited;
} Tile;

typedef struct
{
    int row;
    int column;
} Position;

typedef enum
{
    UP_DIRECTION,
    DOWN_DIRECTION,
    LEFT_DIRECTION,
    RIGHT_DIRECTION,
} Direction;

typedef struct
{
    Position* tiles;
    TileState value;
    Direction dir;
    Direction next_dir;
    Direction next_next_dir;
    bool has_next_next_dir;
} Snake;

typedef struct
{
    Snake snake;
    size_t player_path_index;
} SnakeClone;

typedef struct
{
    Position pos;
    TileState value;
} Food;

typedef struct
{
    Tile tileGrid[ROWS][COLUMNS];
    Snake player;
    Food food;
    Position* player_path;
    SnakeClone* clones;
} Game;

Game game;

void InitTileGrid(void)
{
    for (size_t row = 0; row < ROWS; row++)
    {
        for (size_t column = 0; column < COLUMNS; column++)
        {
            Tile* tile = &game.tileGrid[row][column];
            tile->state = EMPTY_TILE;
            tile->angle = 0.0f;
            tile->timer = (row + 1) * (TILE_SIZE + TILE_SPACING) * (column + 1) * (TILE_SIZE + TILE_SPACING);
            tile->visited = false;
        }
    }
}

void InitSnake(Snake* snake, const TileState value, const size_t row, const size_t column, const size_t length,
               bool is_player)
{
    snake->tiles = nullptr;
    snake->value = value;
    snake->dir = RIGHT_DIRECTION;
    snake->next_dir = RIGHT_DIRECTION;
    snake->next_next_dir = RIGHT_DIRECTION;
    snake->has_next_next_dir = false;

    const auto start_position = (Position){row, column};

    for (size_t i = 0; i < length; i++)
    {
        arrpush(snake->tiles, start_position);
    }

    if (is_player)
    {
        arrpush(game.player_path, start_position);
    }
}

void PlaceFoodRandomly(Food* food)
{
    size_t row, column;
    do
    {
        row = rand() % ROWS;
        column = rand() % COLUMNS;
    }
    while (
        game.tileGrid[row][column].state != EMPTY_TILE &&
        game.tileGrid[row][column].state != VISITED_TILE);
    food->pos.row = row;
    food->pos.column = column;
}

void InitFood(Food* food)
{
    food->value = FOOD_TILE;
    PlaceFoodRandomly(food);
}

void InitGame(void)
{
    game.player_path = nullptr;
    InitTileGrid();
    InitSnake(&game.player, PLAYER_TILE, 13, 24, 3, true);
    InitFood(&game.food);
}

Color GetTileColor(TileState state)
{
    switch (state)
    {
    case EMPTY_TILE:
        return (Color){20, 20, 20, 255};
    case VISITED_TILE:
        return (const Color){50, 50, 50, 255};
    case PLAYER_TILE:
        return (const Color){255, 255, 255, 255};
    case FOOD_TILE:
        return (const Color){40, 255, 40, 255};
    case CLONE_TILE:
        return (const Color){255, 40, 40, 255};
    case CLONE_AND_PLAYER_TILE:
        return (Color){200, 20, 160, 255};
    default: return (Color){50, 50, 50, 255};
    }
}

void DrawTileGrid(void)
{
    for (size_t row = 0; row < ROWS; row++)
    {
        for (size_t column = 0; column < COLUMNS; column++)
        {
            const Tile* tile = &game.tileGrid[row][column];

            const float drawX = (float)column * (TILE_SIZE + TILE_SPACING) + GRID_OFFSET_X;
            const float drawY = (float)row * (TILE_SIZE + TILE_SPACING) + GRID_OFFSET_Y;

            const auto pos = (Vector2){
                drawX + TILE_SPACING / 2.0f + (tile->state == EMPTY_TILE ? TILE_SIZE / 4.0f : 0),
                drawY + TILE_SPACING / 2.0f + (tile->state == EMPTY_TILE ? TILE_SIZE / 4.0f : 0)
            };

            const auto size = (Vector2){
                tile->state == EMPTY_TILE ? TILE_SIZE / 2.0f : TILE_SIZE,
                tile->state == EMPTY_TILE ? TILE_SIZE / 2.0f : TILE_SIZE,
            };

            const Color color = GetTileColor(tile->state);

            const auto center = (Vector2){
                drawX + TILE_SPACING / 2.0f + TILE_SIZE / 2.0f,
                drawY + TILE_SPACING / 2.0f + TILE_SIZE / 2.0f,
            };
            rlPushMatrix();
            rlTranslatef(center.x, center.y, 0);
            rlRotatef(tile->state == EMPTY_TILE ? (RAD2DEG * tile->angle) : 0, 0, 0, 1);
            rlTranslatef(-center.x, -center.y, 0);
            DrawRectangleV(pos, size, color);
            rlPopMatrix();
        }
    }
}

void UpdateTileGrid(const float dtm)
{
    for (size_t row = 0; row < ROWS; row++)
    {
        for (size_t column = 0; column < COLUMNS; column++)
        {
            Tile* tile = &game.tileGrid[row][column];
            tile->timer += dtm;
            tile->angle = sinf(tile->timer) * PI;
            tile->state = tile->visited ? VISITED_TILE : EMPTY_TILE;
        }
    }
}

void SnakeMarkTiles(Snake* snake)
{
    size_t len = arrlen(snake->tiles);
    for (size_t i = 0; i < len; i++)
    {
        Position* p = &snake->tiles[i];
        Tile* tile = &game.tileGrid[p->row][p->column];
        tile->visited = true;

        if ((tile->state == PLAYER_TILE && snake->value == CLONE_TILE) || (tile->state == CLONE_TILE && snake->value ==
            PLAYER_TILE))
        {
            tile->state = CLONE_AND_PLAYER_TILE;
        }
        else
        {
            tile->state = snake->value;
        }
    }
}

void ClonesMarkTiles()
{
    size_t clones_len = arrlen(game.clones);
    for (size_t i = 0; i < clones_len; i++)
    {
        SnakeMarkTiles(&game.clones[i].snake);
    }
}

void FoodMarkTile(const Food* food)
{
    Tile* tile = &game.tileGrid[food->pos.row][food->pos.column];
    tile->state = food->value;
}

void MoveClones()
{
    size_t clones_len = arrlen(game.clones);
    for (size_t i = 0; i < clones_len; i++)
    {
        SnakeClone* clone = &game.clones[i];
        Position next = game.player_path[clone->player_path_index];
        size_t len = arrlen(clone->snake.tiles);
        for (size_t j = len - 1; j > 0; j--)
        {
            clone->snake.tiles[j] = clone->snake.tiles[j - 1];
        }
        clone->snake.tiles[0] = next;
        clone->player_path_index++;
    }
}

void SnakeDoStep(Snake* snake)
{
    snake->dir = snake->next_dir;
    int dx = 0, dy = 0;
    if (snake->dir == UP_DIRECTION) dy = -1;
    else if (snake->dir == DOWN_DIRECTION) dy = 1;
    else if (snake->dir == RIGHT_DIRECTION) dx = 1;
    else if (snake->dir == LEFT_DIRECTION) dx = -1;

    Position* head = &snake->tiles[0];
    Position new_head = (Position){.row = head->row + dy, .column = head->column + dx};

    if (new_head.row < 0)
    {
        new_head.row = ROWS - 1;
    }
    else if (new_head.row >= ROWS)
    {
        new_head.row = 0;
    }
    else if (new_head.column < 0)
    {
        new_head.column = COLUMNS - 1;
    }
    else if (new_head.column >= COLUMNS)
    {
        new_head.column = 0;
    }

    size_t len = arrlen(snake->tiles);
    for (size_t i = len - 1; i > 0; i--)
    {
        snake->tiles[i] = snake->tiles[i - 1];
    }
    snake->tiles[0] = new_head;

    arrpush(game.player_path, new_head);

    if (snake->has_next_next_dir)
    {
        snake->next_dir = snake->next_next_dir;
        snake->has_next_next_dir = false;
    }
}

void SnakeHandleInput(Snake* snake)
{
    if (snake->dir == snake->next_dir)
    {
        if (IsKeyPressed(KEY_LEFT) && snake->dir != RIGHT_DIRECTION)
        {
            snake->next_dir = LEFT_DIRECTION;
        }
        if (IsKeyPressed(KEY_RIGHT) && snake->dir != LEFT_DIRECTION)
        {
            snake->next_dir = RIGHT_DIRECTION;
        }
        if (IsKeyPressed(KEY_UP) && snake->dir != DOWN_DIRECTION)
        {
            snake->next_dir = UP_DIRECTION;
        }
        if (IsKeyPressed(KEY_DOWN) && snake->dir != UP_DIRECTION)
        {
            snake->next_dir = DOWN_DIRECTION;
        }
    }
    else
    {
        if (IsKeyPressed(KEY_LEFT) && snake->next_dir != RIGHT_DIRECTION)
        {
            snake->has_next_next_dir = true;
            snake->next_next_dir = LEFT_DIRECTION;
        }
        if (IsKeyPressed(KEY_RIGHT) && snake->next_dir != LEFT_DIRECTION)
        {
            snake->has_next_next_dir = true;
            snake->next_next_dir = RIGHT_DIRECTION;
        }
        if (IsKeyPressed(KEY_UP) && snake->next_dir != DOWN_DIRECTION)
        {
            snake->has_next_next_dir = true;
            snake->next_next_dir = UP_DIRECTION;
        }
        if (IsKeyPressed(KEY_DOWN) && snake->next_dir != UP_DIRECTION)
        {
            snake->has_next_next_dir = true;
            snake->next_next_dir = DOWN_DIRECTION;
        }
    }
}

void SnakeGrow(Snake* snake)
{
    size_t len = arrlen(snake->tiles);
    Position last = snake->tiles[len - 1];
    arrpush(snake->tiles, last);
}

void SpawnClone(Snake* player)
{
    Snake snake;
    size_t row = game.player_path[0].row;
    size_t column = game.player_path[0].column;
    size_t length = arrlen(player->tiles);
    InitSnake(&snake, CLONE_TILE, row, column, length, false);
    SnakeClone clone = (SnakeClone){snake, 0};
    arrpush(game.clones, clone);
}

int main(void)
{
    srand(time(nullptr));
    InitWindow(WINDOW_WIDTH,WINDOW_HEIGHT, "Snake Rewind");
    SetTargetFPS(60);

    const RenderTexture2D target = LoadRenderTexture(GAME_WIDTH,GAME_HEIGHT);

    InitGame();

    bool foodWasEaten = false;
    float stepTimer = 0.0f;
    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        stepTimer += dt;

        SnakeHandleInput(&game.player);
        UpdateTileGrid(dt);

        if (stepTimer >= STEP_INTERVAL)
        {
            MoveClones();
            SnakeDoStep(&game.player);
            Position* head = &game.player.tiles[0];
            if (head->row == game.food.pos.row && head->column == game.food.pos.column)
            {
                SpawnClone(&game.player);
                SnakeGrow(&game.player);
                foodWasEaten = true;
            }
            stepTimer = 0.0f;
        }

        SnakeMarkTiles(&game.player);
        ClonesMarkTiles();
        if (foodWasEaten)
        {
            foodWasEaten = false;
            PlaceFoodRandomly(&game.food);
        }
        FoodMarkTile(&game.food);
        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawTileGrid();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            target.texture,
            (Rectangle){0, 0,GAME_WIDTH, -GAME_HEIGHT},
            (Rectangle){0, 0,WINDOW_WIDTH,WINDOW_HEIGHT},
            (Vector2){0},
            0,
            WHITE
        );
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
