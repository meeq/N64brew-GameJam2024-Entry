#include "board.h"
#include "global.h"
#include "player.h"

typedef struct
{
  int col;
  int row;
} AiMove;

static AiMove ai_moves[BOARD_SIZE];
static size_t ai_move_count;

static int ai_pieces[PIECE_COUNT];
static size_t ai_piece_count;

void
ai_init (void)
{
}

void
ai_cleanup (void)
{
}

static void
shuffle (int *array, size_t n)
{
  if (n > 1)
    {
      for (size_t i = 0; i < n; i++)
        {
          size_t j = i + rand () / (RAND_MAX / (n - i) + 1);
          int temp = array[j];
          array[j] = array[i];
          array[i] = temp;
        }
    }
}

/**
 * Shuffle the AI pieces for Medium difficulty.
 *
 * Buckets the pieces into two groups: 5-2 and 1.
 * Within each group, the pieces will be randomized;
 * Assumes the pieces are currently sorted by value.
 */
static void
ai_shuffle_pieces_easy (void)
{
  if (ai_piece_count > 1)
    {
      size_t bucket_start = 0;
      size_t bucket_size = 0;
      int bucket_value = PIECES[ai_pieces[0]].value;
      for (size_t i = 0; i < ai_piece_count; i++)
        {
          int piece_value = PIECES[ai_pieces[i]].value;
          if (piece_value == 1 && bucket_value > 1)
            {
              shuffle (&ai_pieces[bucket_start], bucket_size);
              bucket_start = i;
              bucket_size = 0;
              bucket_value = piece_value;
            }
          bucket_size++;
        }
      shuffle (&ai_pieces[bucket_start], bucket_size);
    }
}

/**
 * Shuffle the AI pieces for Medium difficulty.
 *
 * Buckets the pieces into three groups: 5-4, 3-2, and 1.
 * Within each group, the pieces will be randomized;
 * Assumes the pieces are currently sorted by value descending.
 *
 */
static void
ai_shuffle_pieces_medium (void)
{
  if (ai_piece_count > 1)
    {
      size_t bucket_start = 0;
      size_t bucket_size = 0;
      int bucket_value = PIECES[ai_pieces[0]].value;
      for (size_t i = 0; i < ai_piece_count; i++)
        {
          int piece_value = PIECES[ai_pieces[i]].value;
          if ((piece_value <= 3 && bucket_value > 3)
              || (piece_value == 1 && bucket_value > 1))
            {
              shuffle (&ai_pieces[bucket_start], bucket_size);
              bucket_start = i;
              bucket_size = 0;
              bucket_value = piece_value;
            }
          bucket_size++;
        }
      shuffle (&ai_pieces[bucket_start], bucket_size);
    }
}

/**
 * Shuffle the AI pieces for Hard difficulty.
 *
 * Randomize the pieces but keep their values in-order.
 * Assumes the pieces are currently sorted by value descending.
 */
static void
ai_shuffle_pieces_hard (void)
{
  if (ai_piece_count > 1)
    {
      size_t bucket_start = 0;
      size_t bucket_size = 0;
      int bucket_value = PIECES[ai_pieces[0]].value;
      for (size_t i = 0; i < ai_piece_count; i++)
        {
          int piece_value = PIECES[ai_pieces[i]].value;
          if (piece_value < bucket_value)
            {
              shuffle (&ai_pieces[bucket_start], bucket_size);
              bucket_start = i;
              bucket_size = 0;
              bucket_value = piece_value;
            }
          bucket_size++;
        }
      shuffle (&ai_pieces[bucket_start], bucket_size);
    }
}

static void
ai_shuffle_moves (void)
{
  if (ai_move_count > 1)
    {
      for (size_t i = 0; i < ai_move_count; i++)
        {
          size_t j = i + rand () / (RAND_MAX / (ai_move_count - i) + 1);
          AiMove temp = ai_moves[i];
          ai_moves[i] = ai_moves[j];
          ai_moves[j] = temp;
        }
    }
}

static void
ai_locate_initial_moves (PlyNum p)
{
  int index = 0;
  const AiMove corners[] = {
    { 0, 0 },
    { BOARD_COLS - 1, 0 },
    { 0, BOARD_ROWS - 1 },
    { BOARD_COLS - 1, BOARD_ROWS - 1 },
  };

  for (size_t j = 0; j < ARRAY_SIZE (corners); j++)
    {
      if (j == p && board_is_tile_unclaimed (corners[j].col, corners[j].row))
        {
          ai_moves[index].col = corners[j].col;
          ai_moves[index].row = corners[j].row;
          index++;
        }
    }

  for (size_t j = 0; j < ARRAY_SIZE (corners); j++)
    {
      if (j != p && board_is_tile_unclaimed (corners[j].col, corners[j].row))
        {
          ai_moves[index].col = corners[j].col;
          ai_moves[index].row = corners[j].row;
          index++;
        }
    }

  ai_move_count = index;
}

static void
ai_locate_next_moves (PlyNum p)
{
  int index = 0;
  for (int board_row = 0; board_row < BOARD_ROWS; board_row++)
    {
      for (int board_col = 0; board_col < BOARD_COLS; board_col++)
        {
          if (board_is_tile_claimed (board_col, board_row, p))
            {
              // Is the cell to the top-left of this cell viable?
              if (board_is_tile_valid (board_col - 1, board_row - 1)
                  && !board_is_tile_claimed (board_col - 1, board_row, p)
                  && !board_is_tile_claimed (board_col, board_row - 1, p)
                  && board_is_tile_unclaimed (board_col - 1, board_row - 1))
                {
                  ai_moves[index].col = board_col - 1;
                  ai_moves[index].row = board_row - 1;
                  index++;
                }
              // Is the cell to the top-right of this cell viable?
              if (board_is_tile_valid (board_col + 1, board_row - 1)
                  && !board_is_tile_claimed (board_col + 1, board_row, p)
                  && !board_is_tile_claimed (board_col, board_row - 1, p)
                  && board_is_tile_unclaimed (board_col + 1, board_row - 1))
                {
                  ai_moves[index].col = board_col + 1;
                  ai_moves[index].row = board_row - 1;
                  index++;
                }
              // Is the cell to the bottom-left of this cell viable?
              if (board_is_tile_valid (board_col - 1, board_row + 1)
                  && !board_is_tile_claimed (board_col - 1, board_row, p)
                  && !board_is_tile_claimed (board_col, board_row + 1, p)
                  && board_is_tile_unclaimed (board_col - 1, board_row + 1))
                {
                  ai_moves[index].col = board_col - 1;
                  ai_moves[index].row = board_row + 1;
                  index++;
                }
              // Is the cell to the bottom-right of this cell viable?
              if (board_is_tile_valid (board_col + 1, board_row + 1)
                  && !board_is_tile_claimed (board_col + 1, board_row, p)
                  && !board_is_tile_claimed (board_col, board_row + 1, p)
                  && board_is_tile_unclaimed (board_col + 1, board_row + 1))
                {
                  ai_moves[index].col = board_col + 1;
                  ai_moves[index].row = board_row + 1;
                  index++;
                }
            }
        }
    }
  ai_move_count = index;
}

static void
ai_gather_pieces (Player *player)
{
  ai_piece_count = 0;
  for (size_t i = 0; i < PIECE_COUNT; i++)
    {
      if (!player->pieces_used[i])
        {
          ai_pieces[ai_piece_count] = i;
          ai_piece_count++;
        }
    }
}

static bool
ai_try_move (Player *player, const AiMove *loc)
{
  CheckPieceResult check_result;
  for (size_t piece_index = 0; piece_index < ai_piece_count; piece_index++)
    {
      player_change_piece (player, ai_pieces[piece_index]);
      for (int orientation = 0; orientation < 4; orientation++)
        {
          if (orientation == 1)
            {
              player_mirror_piece (player);
            }
          if (orientation == 2)
            {
              player_flip_piece (player);
            }
          if (orientation == 3)
            {
              player_mirror_piece (player);
            }

          for (int offset_y = -(PIECE_ROWS / 2); offset_y <= (PIECE_ROWS / 2);
               offset_y++)
            {
              for (int offset_x = -(PIECE_ROWS / 2);
                   offset_x <= (PIECE_ROWS / 2); offset_x++)
                {
                  player_set_cursor (player, loc->col + offset_x,
                                     loc->row + offset_y);
                  check_result = board_check_piece (player);
                  if (check_result.is_valid)
                    {
                      player_place_piece (player);
                      return true;
                    }
                }
            }
        }
    }
  return false;
}

bool
ai_try (Player *player)
{
  if (player->pieces_left == 0)
    {
      return false;
    }
  else if (player->pieces_left == PIECE_COUNT)
    {
      ai_locate_initial_moves (player->plynum);
    }
  else
    {
      ai_locate_next_moves (player->plynum);
      ai_shuffle_moves ();
    }

  ai_gather_pieces (player);

  if (core_get_aidifficulty () == DIFF_EASY)
    {
      ai_shuffle_pieces_easy ();
    }
  else if (core_get_aidifficulty () == DIFF_MEDIUM)
    {
      ai_shuffle_pieces_medium ();
    }
  else if (core_get_aidifficulty () == DIFF_HARD)
    {
      ai_shuffle_pieces_hard ();
    }

  for (size_t i = 0; i < ai_move_count; i++)
    {
      if (ai_try_move (player, &ai_moves[i]))
        {
          return true;
        }
    }
  return false;
}