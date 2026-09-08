#ifndef _GAME_
#define _GAME_


#include <clib/exec_protos.h>
#include "disk.h"
#include "screen.h"

#include "memory.h"
#include "bitplanes.h"
 
#ifdef DEBUG
#define GamePrintF(...) KPrintF(__VA_ARGS__)
#else
#define GamePrintF(...) ((void)0)
#endif

// Math
#define ABS(x) ((x)<0 ? -(x) : (x))
#define SGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
// #define SGN(x) ((x > 0) - (x < 0)) // Branchless, subtracting is slower?
#define MIN(x,y) ((x)<(y)? (x): (y))
#define MAX(x,y) ((x)>(y)? (x): (y))
#define CLAMP(x, min, max) ((x) < (min)? (min) : ((x) > (max) ? (max) : (x)))
#define CEIL_TO_FACTOR(x, m) ((((x) + (m) - 1) / (m)) * (m))
#define FLOOR_TO_FACTOR(x, m) (((x) / (m)) * (m))
#define ROUND_TO_FACTOR(x, m) ((((x) + (x) / 2) / (m)) * (m))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define SCREENWIDTH  192
#define SCREENHEIGHT 256
#define BLOCKHEIGHT 16
#define EXTRAHEIGHT 32
#define SCREENBYTESPERROW (SCREENWIDTH / 8)

#define SCREENWIDTH_WORDS 96

#define BITMAPWIDTH SCREENWIDTH
#define BITMAPBYTESPERROW (BITMAPWIDTH / 8)


#ifdef USE_YUNLIMITED2
#define BITMAPHEIGHT       (SCREENHEIGHT + EXTRAHEIGHT)         // 288
#define EFFECTIVE_HEIGHT   BITMAPHEIGHT
#else
#define BITMAPHEIGHT       ((SCREENHEIGHT + EXTRAHEIGHT) * 2)   // 576
#define HALFBITMAPHEIGHT   (BITMAPHEIGHT / 2)                   // 288
#define EFFECTIVE_HEIGHT   HALFBITMAPHEIGHT                     // 288
#endif

#define BLOCKSWIDTH 320
#define LV1_BLOCKSHEIGHT 352
#define LV2_BLOCKSHEIGHT 304
#define LV3_BLOCKSHEIGHT 384
#define LV4_BLOCKSHEIGHT 368
#define LV5_BLOCKSHEIGHT 464

#define BLOCKSDEPTH 4
#define BLOCKSCOLORS (1L << BLOCKSDEPTH)
#define BLOCKWIDTH 16
#define BLOCKHEIGHT 16
#define BLOCKSBYTESPERROW (BLOCKSWIDTH / 8)
#define BLOCKSPERROW (BLOCKSWIDTH / BLOCKWIDTH)

#define NUMSTEPS BLOCKHEIGHT

#define BITMAPBLOCKSPERROW (BITMAPWIDTH / BLOCKWIDTH)
#define BITMAPBLOCKSPERCOL (BITMAPHEIGHT / BLOCKHEIGHT)
#define HALFBITMAPBLOCKSPERCOL (BITMAPBLOCKSPERCOL / 2)

#define VISIBLEBLOCKSX (SCREENWIDTH / BLOCKWIDTH)
#define VISIBLEBLOCKSY (SCREENHEIGHT / BLOCKHEIGHT)

#define BITMAPPLANELINES (BITMAPHEIGHT * BLOCKSDEPTH)
#define BLOCKPLANELINES  (BLOCKHEIGHT * BLOCKSDEPTH)
 
#define PALSIZE (BLOCKSCOLORS * 2)
 
#define TWOBLOCKS (BITMAPBLOCKSPERROW - NUMSTEPS)
#define TWOBLOCKSTEP (NUMSTEPS - TWOBLOCKS)

#define ROUND2BLOCKWIDTH(x)  ((x) & ~(BLOCKWIDTH - 1))
#define ROUND2BLOCKHEIGHT(x) ((x) & ~(BLOCKHEIGHT - 1))
 
enum GameState
{
	TITLE_SCREEN = 0,
	ROLLING_DEMO = 1,
    GAME_READY = 2,
    STAGE_START = 3,
    GAME_OVER = 4,
    HIGH_SCORE = 5
};

enum GameStages
{
    STAGE_ATTRACT = 0,      // Attract/FrontView
    STAGE_LASVEGAS = 1,
    STAGE_HOUSTON = 2,
    STAGE_STLOUIS = 3,
    STAGE_CHICAGO = 4,
    STAGE_NEWYORK = 5,
    STAGE_COUNT = 6
    
};

enum GameDifficulty
{
    FIVEHUNDREDCC = 0,
    SEVENFIFTYCC = 1,
    TWELVEHUNDREDCC = 2
};

enum GameMapType
{
    MAP_ATTRACT_INTRO = 0,
    STAGE1_OVERHEAD = 1,
    STAGE1_FRONTVIEW = 2,
    STAGE2_OVERHEAD = 3,
    STAGE2_FRONTVIEW = 4,
    STAGE3_OVERHEAD = 5,
    STAGE3_FRONTVIEW = 6,
    STAGE4_OVERHEAD = 7,
    STAGE4_FRONTVIEW = 8,
    STAGE5_OVERHEAD = 9,
    STAGE5_FRONTVIEW = 10
};

enum StageState
{
    STAGE_BEGIN = 0,
    STAGE_COUNTDOWN = 1,
    STAGE_PLAYING = 2,
    STAGE_FRONTVIEW = 3,
    STAGE_COMPLETE = 4,
    STAGE_RANKING = 5,   
    STAGE_CONTINUE = 6,
    STAGE_GAMEOVER = 7,
    STAGE_GAMEOVER_ENTRY = 8,
    STAGE_FUEL_EMPTY = 9
};

typedef enum {
    COLLISION_NONE = 0,
    COLLISION_TRAFFIC = 1,
    COLLISION_OFFROAD = 2,
    COLLISION_WATER = 3
} CollisionState;
 
#define MAX_CONTINUES 3

extern UBYTE game_stage;
extern UBYTE game_state;
extern UBYTE game_difficulty;
extern UBYTE game_map;
extern UBYTE game_best_rank;
extern UBYTE game_continues;

extern UWORD max_stage_speed;

extern UBYTE stage_state;
extern UBYTE stage_complete;

extern WORD mapposy,videoposy;
extern LONG	mapwidth,mapheight;

extern UBYTE *frontbuffer,*blocksbuffer;
extern UWORD *mapdata;

extern ULONG game_score;
extern UBYTE game_rank;
extern UBYTE game_car_block_move_rate;   
extern UBYTE game_car_block_move_speed;  
extern UBYTE game_car_block_x_threshold;
extern ULONG game_frame_count;
extern UWORD frontview_bike_frames;
extern CollisionState collision_state;
extern WORD collision_car_index;  

extern ULONG speed_accumulator;
extern ULONG speed_sample_count;

extern BOOL bike_invulnerable;


// Palettes
extern UWORD	intro_colors[BLOCKSCOLORS];
extern UWORD	city_colors[BLOCKSCOLORS];
extern UWORD	offroad_colors[BLOCKSCOLORS];
extern UWORD    lv_colors[BLOCKSCOLORS];
extern UWORD    stlouis_colors[BLOCKSCOLORS];
extern UWORD    houston_colors[BLOCKSCOLORS];
extern UWORD    black_palette[BLOCKSCOLORS];
extern UWORD    palette_fv_stl[BLOCKSCOLORS];
extern UWORD   *current_palette;

 
extern struct BitMapEx *BlocksBitmap,*ScreenBitmap;
extern void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy, UBYTE *dest);
extern void DrawBlocks(LONG x,LONG y,
                        LONG mapx,LONG mapy, 
                        UWORD blocksperrow, UWORD blockbytessperrow, 
                        UWORD blockplanelines, BOOL deltas_only,    // deltas_only = replace updated tiles only
                        UBYTE tile_idx, UBYTE *dest);               // tile_idx = tileset we want to pull from

extern void DrawBlockRun(LONG x, LONG y, UWORD block, WORD count, UWORD blocksperrow, UWORD blockbytesperrow, UWORD blockplanelines, UBYTE *dest);
extern void DrawBlocksHalf(LONG x, LONG y, LONG mapx, LONG mapy, 
    UWORD blocksperrow, UWORD blockbytessperrow, UWORD blockplanelines, 
    BOOL deltas_only, UBYTE tile_idx, UBYTE *dest);
extern void DrawBlockRunHalf(LONG x, LONG y, UWORD block, WORD count, 
    UWORD blocksperrow, UWORD blockbytesperrow, UWORD blockplanelines, UBYTE *dest);

void Game_Initialize(void);
void Game_NewGame(UBYTE difficulty);
void Game_CheckState();
void Game_SetBackGroundColor(UWORD color);
void Game_FillScreen(void);
void Game_CheckJoyScroll(void);
void Game_SwapBuffers(void);
void Game_RenderBackgroundToDrawBuffer(void);
void Game_ResetBitplanePointer(void);
void Game_Update(void);
void Game_Draw(void);
void Game_LoadPalette(const char *filename, UWORD *palette, int num_colors);
void Game_ApplyPalette(UWORD *palette, int num_colors);
void Game_SetMap(UBYTE maptype);
void Game_HandleCollisions(void);
void Game_Reset(void);
void Game_AdvanceStage(void);
void Game_StartNextOverhead(void);

void GameReady_Initialize(void);
void GameReady_Draw(void);
void GameReady_Update(void);

void Game_RecordBestRank(void);
UBYTE Game_GetTodaysBestRank(void);

void Stage_Initialize(void);
void Stage_Draw(void);
void Stage_Update(void);
void Stage_ShowInfo(void);
void Stage_CheckCompletion(void);
void Stage_InitializeFrontView(void);
void Stage_RedrawTunnelTiles(void);

#endif