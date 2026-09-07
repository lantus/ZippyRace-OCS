#include "support/gcc8_c_support.h"
#include <exec/types.h>
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include "game.h"
#include "map.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "fuel.h"
#include "sprites.h"
#include "memory.h"
#include "blitter.h"
#include "roadsystem.h"
#include "motorbike.h"
#include "pak.h"
#include "cars.h"
#include "audio.h"

/*
 * ============================================================================
 * ZIPPY RACE — TRAFFIC CAR SYSTEM
 * ============================================================================
 *
 * Based on reverse engineering of the NES Zippy Race (1983, Irem/SunSoft).
 * Adapted from 2-car NES system to 5-car Amiga system with collision map
 * pathfinding and double-buffered BOB rendering.
 *
 * ---- NES REFERENCE (from disassembly) ------------------------------------
 *
 * The NES has exactly 2 traffic car slots. Each car has:
 *   - X position (pixel)
 *   - Y position (screen-relative)
 *   - Previous Y (for pass detection)
 *   - State: $00=normal, $80=crashed/swerving
 *
 * ---- MOVEMENT: RELATIVE SPEED -------------------------------------------
 *
 * NES formula: car_y += scroll_speed - offset
 *
 * Both values are raw pixels/frame. The car doesn't have its own speed —
 * everything is relative to the bike's scroll rate:
 *
 *   scroll_speed = 0 (stopped):  car_y += -4 -> car drives AWAY from player
 *   scroll_speed = 4 (cruising): car_y += 0  -> car appears stationary
 *   scroll_speed = 6 (fast):     car_y += 2  -> car drifts DOWN past player
 *
 * NES offset values:
 *   - Default: 4 pixels/frame
 *   - Early game (<15 cars passed): 5 pixels/frame (easier)
 *   - Player crashed: offset++ (cars pull away faster)
 *
 * AMIGA IMPLEMENTATION:
 *   - Scroll values are 8.8 fixed point (256 = 1 pixel/frame)
 *   - Offset is also 8.8: 768=3px, 1024=4px, 1280=5px
 *   - Per-car variation via target_speed field:
 *       target_speed > 150: offset = 768  (fast car, hard to pass)
 *       target_speed > 120: offset = 1024 (medium, matches NES default)
 *       target_speed <= 120: offset = 1280 (slow car, easy to pass)
 *   - Car always moves FORWARD (decreasing Y) at a fixed rate
 *   - SmoothScroll changes mapposy which handles relative motion:
 *       screen_y = car.y - mapposy
 *       Bike fast -> mapposy drops fast -> screen_y increases -> car drifts down
 *       Bike slow -> mapposy drops slow -> screen_y stays/decreases -> car ahead
 *
 * ---- STEERING: 3-TILE PROBE + DECISION TREE -----------------------------
 *
 * NES method: Read 3 adjacent road tiles at the car's current X column.
 * Each tile is checked against a non-road table. The decision tree:
 *
 *   CENTER BLOCKED:
 *     Both sides blocked -> random direction (NES: 25% crash, we skip crash)
 *     Left open only     -> steer left
 *     Right open only    -> steer right
 *     Both open          -> pick side with more room
 *
 *   CENTER CLEAR, SIDE BLOCKED:
 *     Nudge away from blocked side (1px/frame)
 *
 *   ALL CLEAR:
 *     Pursue player (see below)
 *
 * NES steers 4px on a 256px screen. Amiga steers 3px (hard) or 1px (soft)
 * on a 192px screen — proportionally similar.
 *
 * AMIGA ADDITION: Lookahead probe at Y-32 (2 tiles ahead). If the road
 * curves ahead, the car starts turning early with a gentle 1px nudge.
 * The NES doesn't need this because its road is wider relative to car size.
 *
 * ROAD DEFINITION:
 *   NES: binary road/not-road tile check
 *   Ours: collision map with packed lane+surface bytes
 *     - LANE_OFFROAD (0x00): blocked — car must steer away
 *     - LANE_SHOULDER (0x10-0x20): drivable but treated as blocked for AI
 *       (bike can ride shoulders, cars avoid them)
 *     - LANE_1 to LANE_4 (0x30-0x60): proper road — car drives here
 *
 * ---- PURSUIT: BLOCKING THE PLAYER ---------------------------------------
 *
 * NES code at $A4BA: When all 3 road tiles ahead are clear, the car
 * actively steers toward the player's X position. Conditions:
 *
 *   - Player must be BEHIND the car (approaching from below)
 *   - Car must be within chase range on screen
 *   - 25% random chance to skip each frame (keeps it beatable)
 *   - Only 1px per frame (NES uses INC/DEC car_x)
 *   - Stops pursuing near top of screen
 *   - Stops pursuing on narrow road sections
 *
 * AMIGA IMPLEMENTATION — three distance zones:
 *
 *   Distance > 160px: Car drives normally — no blocking                
 *   Distance 64-160px: Car subtly drifts into bike's lane                 
 *   Distance < 64px:  Car STOPS blocking — locked in position

 * ---- SPAWNING: DIFFICULTY RAMP ------------------------------------------
 *
 * NES spawn logic at $A388 gates on cars_passed:
 *   0-1 passed: 1 car max on screen
 *   2 passed:   2nd car only if 1st is gone
 *   3+ passed:  Both slots active simultaneously
 *
 * OUR RAMP (5 slots):
 *   cars_passed < 3:   max 1 active
 *   cars_passed < 8:   max 2 active
 *   cars_passed < 15:  max 3 active
 *   cars_passed >= 15: max 4 active
 *
 * NES spawn position depends on scroll speed:
 *   scroll >= 3: spawn at screen Y=0 (top, AHEAD of player)
 *   scroll < 3:  spawn at screen Y=$E0 (bottom, BEHIND player)
 *
 * OUR IMPLEMENTATION:
 *   scroll >= 768 (3px/frame): spawn at mapposy - 32 (above screen)
 *   scroll < 768:              spawn at mapposy + SCREENHEIGHT + 32 (below)
 *
 * Cars spawned above scroll into view from the top — player approaches.
 * Cars spawned below scroll into view from the bottom — car approaches.
 * At low speed, traffic comes from behind and passes.
 *
 * NES uses rejection sampling for lane: pick random X column, check 3
 * adjacent tiles, retry if any are not road. We do the same using
 * Collision_GetLane().
 *
 * Spawn slot rotates via next_slot counter (0->1->2->3->4->0) to cycle
 * through all 5 car sprites instead of always reusing slot 0.
 *
 * ---- CAR-CAR COLLISION: SEPARATION --------------------------------------
 *
 * NES at $A2BB: AABB check (24px wide × 32px tall). The car behind
 * enters crash/swerve state ($80). With only 2 cars this is simple.
 *
 * OUR IMPLEMENTATION: With 5 cars, crashing on overlap is too harsh.
 * Instead we do gentle separation:
 *   - Y too close (<40px): car behind pushed backward (+1 Y/frame)
 *   - X too close (<32px): both cars nudged apart (±1 X/frame)
 *   - Runs once after all cars have moved (Cars_CheckAllCollisions)
 *   - N*(N-1)/2 pair checks = 10 checks for 5 cars
 *
 * ---- CRASH RECOVERY: SCATTER --------------------------------------------
 *
 * NES uses invulnerability timer ($93) and speed offset increase during
 * crash state. Cars naturally drift away because of the offset++ in the
 * movement formula.
 *
 * ADDITION: After crash recovery, Cars_ScatterAfterCrash() pushes
 * all nearby cars 200-600px ahead with staggered spacing and lane spread.
 * Cars are marked off_screen and scroll back into view naturally over
 * the next few seconds. This prevents the same car from repeatedly
 * colliding with the bike on recovery.
 *
 * ---- PASS DETECTION -----------------------------------------------------
 *
 * NES at $A342: Frame-to-frame Y comparison.
 *   - Save car_prev_y each frame
 *   - If player was behind last frame AND ahead this frame → PASSED
 *   - Awards 50 points + sound effect
 *   - Counter capped at 99
 *
 * AMIGA IMPLEMENTATION: Same logic using car_was_ahead[] boolean array.
 * Awards 500 points (10× NES for Amiga score scale) and plays
 * SFX_OVERHEADOVERTAKE. Also updates game_rank (99=last, 1=first)
 * and increments cars_passed for the difficulty ramp.
 *
 * ---- RENDERING: DOUBLE-BUFFERED BOBS ------------------------------------
 *
 * NES: Simple sprite writes to OAM
 *
 * AMIGA IMPLEMENTATION: Blitter cookie-cut BOBs on interleaved bitplanes.
 *   - 32×32 pixel BOBs, 4 bitplanes, barrel-shifted for sub-word X
 *   - Three buffers: draw_buffer, display_buffer, pristine
 *   - Position tracking: old_x/old_y and prev_old_x/prev_old_y
 *     for 2-frame-delayed restore (double buffer = each buffer needs
 *     its own restore from 2 frames ago)
 *   - needs_restore flag only set when car was actually rendered on screen
 *   - Screen bounds check before restore to prevent artifacts from
 *     off-screen spawn positions
 *   - Buffer split handling when BOB crosses the bitmap wrap boundary
 *
 * ============================================================================
 */


#define CAR_BG_SIZE 768
extern volatile struct Custom *custom;

static BOOL cars_spawn_disabled = FALSE;

#define CAR_SPEED_OFFSET_EASY   5   // Before 15 cars passed
#define CAR_SPEED_OFFSET_HARD   4   // After 15 cars passed
#define CAR_STEER_AMOUNT        2   // Pixels per steer  

WORD next_respawn_id = 0;     // Start with car 0
WORD respawn_timer = 60;
WORD respawn_interval = 120;
WORD respawn_distance = 400;
WORD last_respawned_id = -1;  // -1 = none respawned yet
UWORD cars_passed = 0;

BlitterObject car[MAX_CARS];

BOOL car_was_ahead[MAX_CARS];

static LONG car_last_y[MAX_CARS];
static BOOL passing_initialized = FALSE;

/* Restore pointers */
APTR restore_ptrs[MAX_CARS * 4];
APTR *restore_ptr;

void Cars_LoadSprites()
{
    car[0].data = Pak_LoadAsset(CAR0_FRAME0, MEMF_CHIP);
    car[0].data_frame2 = Pak_LoadAsset(CAR0_FRAME1, MEMF_CHIP);

    car[1].data = Pak_LoadAsset(CAR1_FRAME0, MEMF_CHIP);
    car[1].data_frame2 = Pak_LoadAsset(CAR1_FRAME1, MEMF_CHIP);

    car[2].data = Pak_LoadAsset(CAR2_FRAME0, MEMF_CHIP);
    car[2].data_frame2 = Pak_LoadAsset(CAR2_FRAME1, MEMF_CHIP);

    car[3].data = Pak_LoadAsset(CAR3_FRAME0, MEMF_CHIP);
    car[3].data_frame2 = Pak_LoadAsset(CAR3_FRAME1, MEMF_CHIP);

    car[4].data = Pak_LoadAsset(CAR4_FRAME0, MEMF_CHIP);
    car[4].data_frame2 = Pak_LoadAsset(CAR4_FRAME1, MEMF_CHIP);   

}

void Cars_Initialize(void)
{
    car[0].background = Mem_AllocChip(CAR_BG_SIZE);
    car[1].background = Mem_AllocChip(CAR_BG_SIZE);
    car[2].background = Mem_AllocChip(CAR_BG_SIZE);
    car[3].background = Mem_AllocChip(CAR_BG_SIZE);
    car[4].background = Mem_AllocChip(CAR_BG_SIZE);
  
    Cars_LoadSprites();

}
 
void Cars_ResetPositions(void)
{
    car[0].id = 0;
    car[0].visible = TRUE;
    car[0].x = FAR_RIGHT_LANE;
    car[0].y = mapposy + 220;   
    car[0].old_x = car[0].x;
    car[0].old_y = car[0].y;
    car[0].prev_old_x  = car[0].x;
    car[0].prev_old_y = car[0].y;
    car[0].speed = 768;
    car[0].accumulator = 0;
    car[0].target_speed = 165;
    car[0].off_screen = FALSE;
    car[0].needs_restore = FALSE;
    car[0].anim_frame = 0;
    car[0].anim_counter = 0;
    car[0].has_blocked_bike = FALSE;
    car[0].block_timer = 0;

    car[1].id = 1;
    car[1].visible = TRUE;
    car[1].x = FAR_LEFT_LANE;
    car[1].y = mapposy + 220;
    car[1].speed = 1024;
    car[1].accumulator = 0;
    car[1].target_speed = 155;
    car[1].off_screen = FALSE;
    car[1].needs_restore = FALSE;
    car[1].anim_frame = 0;
    car[1].anim_counter = 0;
    car[1].has_blocked_bike = FALSE;
    car[1].block_timer = 0;
    car[1].old_x = car[1].x;
    car[1].old_y = car[1].y;
    car[1].prev_old_x  = car[1].x;
    car[1].prev_old_y = car[1].y;
   
    car[2].id = 2;
    car[2].visible = TRUE;
    car[2].x = FAR_LEFT_LANE;
    car[2].y = mapposy + 148;
    car[2].speed = 1024;
    car[2].target_speed = 165;
    car[2].accumulator = 0;
    car[2].off_screen = FALSE;
    car[2].needs_restore = FALSE;
    car[2].anim_frame = 0;
    car[2].anim_counter = 0;
    car[2].has_blocked_bike = FALSE;
    car[2].block_timer = 0;  
    car[2].old_x = car[2].x;
    car[2].old_y = car[2].y;
    car[2].prev_old_x  = car[2].x;
    car[2].prev_old_y = car[2].y;
   
    car[3].id = 3;
    car[3].visible = TRUE;
    car[3].x = FAR_RIGHT_LANE;
    car[3].y = mapposy + 148;  
    car[3].speed = 768;
    car[3].target_speed = 165;
    car[3].accumulator = 0;
    car[3].off_screen = FALSE;
    car[3].needs_restore = FALSE;
    car[3].anim_frame = 0;
    car[3].anim_counter = 0;
    car[3].has_blocked_bike = FALSE;
    car[3].block_timer = 0;  
    car[3].old_x = car[3].x;
    car[3].old_y = car[3].y;
    car[3].prev_old_x  = car[3].x;
    car[3].prev_old_y = car[3].y; 
   
    car[4].id = 4;
    car[4].visible = TRUE;
    car[4].x = CENTER_LANE;
    car[4].y = mapposy + 40;
    car[4].speed = 640;
    car[4].target_speed = 165;
    car[4].accumulator = 0;
    car[4].off_screen = FALSE;
    car[4].needs_restore = FALSE;
    car[4].anim_frame = 0;
    car[4].anim_counter = 0;
    car[4].has_blocked_bike = FALSE;
    car[4].block_timer = 0;
    car[4].old_x = car[4].x;
    car[4].old_y = car[4].y;
    car[4].prev_old_x  = car[4].x;
    car[4].prev_old_y = car[4].y;   

    // Reset passing tracking
    for (int i = 0; i < MAX_CARS; i++)
    {
        car_last_y[i] = car[i].y;
        car_was_ahead[i] = TRUE;
        car[i].spawning = FALSE;
        car[i].honking = FALSE;
        car[i].honk_timer = 0;
        car[i].pursuit_timer = 0;
        car[i].swerving = FALSE;
        car[i].lateral_accum= 0;
        car[i].swerve_timer = 0;
        car[i].in_tunnel = 0;
        
    }

    passing_initialized = TRUE;
    cars_passed = 0;
}

void Cars_ClearAll(void)
   {
       for (int i = 0; i < MAX_CARS; i++)
       {
           car[i].visible = FALSE;
           car[i].off_screen = TRUE;
           car[i].crashed = FALSE;
       }
   }

void Cars_ResetPositionsEmpty(void)
{
    for (int i = 0; i < MAX_CARS; i++)
    {
        car[i].id = i;
        car[i].visible = TRUE;
        car[i].off_screen = TRUE;
        car[i].needs_restore = FALSE;
        car[i].spawning = FALSE;
        car[i].honking = FALSE;
        car[i].honk_timer = 0;
        car[i].pursuit_timer = 0;
        car[i].swerving = FALSE;
        car[i].lateral_accum = 0;
        car[i].swerve_timer = 0;
        car[i].accumulator = 0;
        car[i].anim_frame = 0;
        car[i].anim_counter = 0;
        car[i].has_blocked_bike = FALSE;
        car[i].block_timer = 0;
        car[i].speed = 0;
        car[i].target_speed = 0;
        car[i].in_tunnel = 0; 
        
        car_last_y[i] = 0;
        car_was_ahead[i] = TRUE;
    }
    
    passing_initialized = TRUE;
    cars_passed = 0;
}
 
void Cars_RenderBOB(BlitterObject *car)
{

    /* 32x32
    Barrel Shifting is a pain...
    bitplanes = 4
    words per scanline - 2 words (32/16)
              - 3 words with -AW padding
              
    Bytes per scanline = (48/8) = 6 bytes + 6 bytes mask
    Total scanline = 32

    BlitSize (32x4) << 6 | 3
 
    src mod = 3 words x 2 Bytes
    dst mod = 40 - 6 = 34; for 192 = 24 - 6 = 18
    */
   
    if (!car->visible) return;
    
    WORD screen_y = car->y - mapposy;
    
    // Calculate clipping
    WORD src_y_offset = 0;
    WORD clip_height = BOB_HEIGHT;
    WORD dest_y = screen_y;
 
     // Clip BOTTOM edge
    if (screen_y + clip_height > SCREENHEIGHT)
    {
        clip_height = SCREENHEIGHT - screen_y;
        
        if (clip_height <= 0)
        {
            car->off_screen = TRUE;
            return;
        }
    }
    
    car->spawning = FALSE;

    // Clip TOP edge
    if (screen_y < -BOB_HEIGHT)
    {
        src_y_offset = -screen_y;
        clip_height -= src_y_offset;
        dest_y = 0;
        
        if (clip_height <= 0)
        {
            car->off_screen = TRUE;
            return;
        }
    }
  
    // Check X bounds
    if (car->x < -16 || car->x > SCREENWIDTH + 16)
    {
        car->off_screen = TRUE;
        return;
    }
    
    car->off_screen = FALSE;
 

    WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
    
    // Handle wraparound
    if (buffer_y < 0)
        buffer_y += BITMAPHEIGHT;
    else if (buffer_y >= BITMAPHEIGHT)
        buffer_y -= BITMAPHEIGHT;
    
    WORD x = car->x;
    
    // Select animation frame
    UBYTE *source = (car->anim_frame == 0) ? car->data : car->data_frame2;
    UBYTE *mask = source + 6;
    
    // Offset source: src_y_offset * 24 = (src_y_offset << 4) + (src_y_offset << 3)
    ULONG src_offset = (src_y_offset << 4) + (src_y_offset << 3);  // * 16 + * 8 = * 24
    source += src_offset;
    mask += src_offset;
    
    UWORD source_mod = 6;
    UWORD dest_mod = 18;
    ULONG admod = ((ULONG)dest_mod << 16) | source_mod;
    
    APTR car_restore_ptrs[4];
 
    if (car->in_tunnel)
    {
        // Already inside — check car CENTER to know when fully out
        UBYTE surface = Collision_GetSurface(car->x + 16, car->y + 32);
        if (surface == SURFACE_TUNNEL)
        {
            return;  // Still inside tunnel — don't draw
        }
        else
        {
            car->in_tunnel = FALSE;  // Fully out — unlatch
        }
    }
    else
    {
        // Not in tunnel — check AHEAD to catch entry
        UBYTE surface = Collision_GetSurface(car->x + 16, car->y);
        if (surface == SURFACE_TUNNEL)
        {
            car->in_tunnel = TRUE;    
            return;                  
        }
    }

    // Check if BOB crosses buffer split
    if (buffer_y + clip_height > BITMAPHEIGHT)
    {
        // SPLIT BLIT
        WORD lines_before = BITMAPHEIGHT - buffer_y;
        WORD lines_after = clip_height - lines_before;
        
        // bltsize: lines * 4 = lines << 2
        UWORD bltsize1 = ((lines_before << 2) << 6) | 3;
        BlitBob2(SCREENWIDTH_WORDS, x, buffer_y, admod, bltsize1, 
                 BOB_WIDTH, car_restore_ptrs, source, mask, draw_buffer);
        
        // Advance source: lines_before * 24
        ULONG advance = (lines_before << 4) + (lines_before << 3);
        UBYTE *source2 = source + advance;
        UBYTE *mask2 = mask + advance;
        APTR restore_ptrs2[4];
        UWORD bltsize2 = ((lines_after << 2) << 6) | 3;
        
        BlitBob2(SCREENWIDTH_WORDS, x, 0, admod, bltsize2,
                 BOB_WIDTH, restore_ptrs2, source2, mask2, draw_buffer);
    }
    else
    {
        // Normal single blit
        UWORD bltsize = ((clip_height << 2) << 6) | 3;
        
        BlitBob2(SCREENWIDTH_WORDS, x, buffer_y, admod, bltsize,
                 BOB_WIDTH, car_restore_ptrs, source, mask, draw_buffer);
    }
}
 
void Cars_PreDraw(void)
{
    
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (car[i].visible && !car[i].off_screen)
        {
            // Use prev_old position (2 frames back)
            WORD temp_x = car[i].old_x;
            WORD temp_y = car[i].old_y;
            
            car[i].old_x = car[i].prev_old_x;
            car[i].old_y = car[i].prev_old_y;
  
            car[i].needs_restore = 1;
            
            // Only restore if prev position was on screen
            WORD prev_screen_y = car[i].prev_old_y - mapposy;
            if (prev_screen_y >= -BOB_HEIGHT && prev_screen_y < SCREENHEIGHT + BOB_HEIGHT)
            {
                Cars_CopyPristineBackground(&car[i]);
            }   
            
            car[i].old_x = temp_x;
            car[i].old_y = temp_y;

            // Save position history
            car[i].prev_old_x = car[i].old_x;
            car[i].prev_old_y = car[i].old_y;
            
            car[i].old_x = car[i].x;
            car[i].old_y = car[i].y;
            
    
            Cars_RenderBOB(&car[i]);
 
        }
    }

}

void Cars_CheckAllCollisions(void)
{
    WORD bike_cx = bike_position_x + 8;
    WORD bike_sy = bike_position_y - g_sprite_voffset;
    
    /* ---- Bike rear-end prevention (unchanged) ---- */
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (!car[i].visible || car[i].off_screen || car[i].crashed) continue;
        if (car[i].swerving) continue;

        WORD car_screen_y = car[i].y - mapposy;
        WORD car_cx = car[i].x + 16;
        WORD x_dist = ABS(bike_cx - car_cx);
        WORD gap = car_screen_y - bike_sy;
 
        if (gap > 0 && gap < 40 && x_dist < 16)
        {
            car[i].accumulator = 0;
            car[i].y += 3;
            
            if (gap < 36)
                car[i].y++;
            
            if (!car[i].honking)
            {
                car[i].honking = TRUE;
                car[i].honk_timer = 0;

                if (fuel_alarm_active == FALSE)
                {
                    SFX_Play(SFX_HORN);
                }
            }
            car[i].honk_timer++;
            if (car[i].honk_timer > 360)
            {
                car[i].honk_timer = 0;           
            }
        }
        else
        {
            car[i].honking = FALSE;
            car[i].honk_timer = 0;
        }
    }
    
    /* ---- Car vs Car: arcade swerve/spinout ---- */
    for (int i = 0; i < MAX_CARS - 1; i++)
    {
        if (!car[i].visible || car[i].off_screen) continue;
        
        for (int j = i + 1; j < MAX_CARS; j++)
        {
            if (!car[j].visible || car[j].off_screen) continue;
            
            WORD x_dist = ABS(car[i].x - car[j].x);
            LONG y_dist = ABS(car[i].y - car[j].y);
            
            if (x_dist > 28 || y_dist > 36) continue;
            
            /* Determine which car is behind (higher Y = behind) */
            BlitterObject *behind = (car[i].y > car[j].y) ? &car[i] : &car[j];
            BlitterObject *ahead  = (car[i].y > car[j].y) ? &car[j] : &car[i];
            
            if (!behind->swerving)
            {
                behind->swerving = TRUE;
                behind->swerve_timer = 45;
                
                /* No lateral push — car stops and spins in place */
                
                //SFX_Play(SFX_CRASH);
            }
            
            /* Always separate Y to prevent overlap */
            if (car[i].y > car[j].y)
                car[i].y += 2;
            else
                car[j].y += 2;
        }
    }
}


void Cars_Update(void)
{
    bike_world_y = mapposy + bike_position_y;
    Cars_CheckForRespawn();

    for (int i = 0; i < MAX_CARS; i++)
    {
        if (!car[i].visible) continue;
        
        // Just pass the car pointer
        Cars_CheckPassing(&car[i]);
        
       
        Cars_Tick(&car[i]);
    }  

     Cars_CheckAllCollisions();
}

void Cars_CopyPristineBackground(BlitterObject *car)
{
    if (!car->needs_restore) return;
    
    WORD old_screen_y = car->old_y - mapposy;

    // Skip restore if old position was off screen
    if (old_screen_y < -BOB_HEIGHT || old_screen_y >= SCREENHEIGHT + BOB_HEIGHT)
        return;
    
    // Calculate clipping
    WORD clip_height = BOB_HEIGHT;
    WORD dest_y = old_screen_y;
    
    // Clip TOP edge
    if (old_screen_y < -BOB_HEIGHT)
    {
        clip_height += old_screen_y;
        dest_y = 0;
        if (clip_height <= 0) return;
    }
    
    // Clip BOTTOM edge
    if (old_screen_y + clip_height > SCREENHEIGHT)
    {
        clip_height = SCREENHEIGHT - old_screen_y;
        if (clip_height <= 0) return;
    }
    
    WORD buffer_y = (videoposy + BLOCKHEIGHT + dest_y);
    
    // Handle wraparound
    if (buffer_y < 0)
        buffer_y += BITMAPHEIGHT;
    else if (buffer_y >= BITMAPHEIGHT)
        buffer_y -= BITMAPHEIGHT;
    
    // X position word-aligned: (car->old_x >> 4) << 1
    WORD x_word_aligned = (car->old_x >> 3) & 0xFFFE;  // Even faster
    
    // Check if restore crosses buffer split
    if (buffer_y + clip_height > BITMAPHEIGHT)
    {
        // SPLIT RESTORE
        WORD lines_before = BITMAPHEIGHT - buffer_y;
        WORD lines_after = clip_height - lines_before;
        
        // buffer_y * SCREENWIDTH_WORDS: SCREENWIDTH_WORDS = 96 = 64 + 32
        ULONG offset1 = ((ULONG)buffer_y << 6) + ((ULONG)buffer_y << 5) + x_word_aligned;
        
        WaitBlit();
        custom->bltcon0 = 0x9F0;
        custom->bltcon1 = 0;
        custom->bltafwm = 0xFFFF;
        custom->bltalwm = 0xFFFF;
        custom->bltamod = 18;
        custom->bltdmod = 18;
        custom->bltapt = screen.pristine + offset1;
        custom->bltdpt = draw_buffer + offset1;
        custom->bltsize = ((lines_before << 2) << 6) | 3;
        
        // Part 2: Y=0
        ULONG offset2 = x_word_aligned;
        
        WaitBlit();
        custom->bltapt = screen.pristine + offset2;
        custom->bltdpt = draw_buffer + offset2;
        custom->bltsize = ((lines_after << 2) << 6) | 3;
    }
    else
    {
        // Single blit
        ULONG offset = ((ULONG)buffer_y << 6) + ((ULONG)buffer_y << 5) + x_word_aligned;
        
        WaitBlit();
        custom->bltcon0 = 0x9F0;
        custom->bltcon1 = 0;
        custom->bltafwm = 0xFFFF;
        custom->bltalwm = 0xFFFF;
        custom->bltamod = 18;
        custom->bltdmod = 18;
        custom->bltapt = screen.pristine + offset;
        custom->bltdpt = draw_buffer + offset;
        custom->bltsize = ((clip_height << 2) << 6) | 3;
    }
}


void Cars_DisableSpawning(void)
{
    cars_spawn_disabled = TRUE;
}

void Cars_EnableSpawning(void)
{
    cars_spawn_disabled = FALSE;
}

BOOL Cars_AnyOnScreen(void)
{
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (!car[i].visible) continue;
        
        WORD screen_y = car[i].y - mapposy;
        if (screen_y > -BOB_HEIGHT && screen_y < SCREENHEIGHT)
            return TRUE;
    }
    return FALSE;
}
 

void Cars_HandleSpinout(UBYTE car_index)
{
    if (car_index >= MAX_CARS) return;
    
    BlitterObject *crashed_car = &car[car_index];
    
    // Mark car as crashed
    crashed_car->crashed = TRUE;
    crashed_car->crash_timer = 120;  // Stopped for 60 frames (1 second)
    
    // Decelerate car extremely quickly
    crashed_car->speed = 0;
    crashed_car->accumulator = 0;
}

#define CAR_STEER_HARD  3    // Emergency — center blocked
#define CAR_STEER_SOFT  1    // Gentle — side blocked or pursuit

void Cars_CheckLaneAndSteer(BlitterObject *car)
{
    if (!car->visible || car->crashed) return;
    if (car->swerving) return;
 
    WORD cx = car->x + 16;
    WORD check_y = car->y;
    
    UBYTE left   = Collision_GetLane(cx - 16, check_y);
    UBYTE center = Collision_GetLane(cx, check_y);
    UBYTE right  = Collision_GetLane(cx + 16, check_y);
    
    BOOL ok_l = (left >= LANE_1 && left <= LANE_4);
    BOOL ok_c = (center >= LANE_1 && center <= LANE_4);
    BOOL ok_r = (right >= LANE_1 && right <= LANE_4);
 
    if (!ok_c)
    {
        if (ok_r && !ok_l)
            car->x += CAR_STEER_HARD;
        else if (ok_l && !ok_r)
            car->x -= CAR_STEER_HARD;
        else if (ok_l && ok_r)
        {
            UBYTE far_l = Collision_GetLane(cx - 32, check_y);
            UBYTE far_r = Collision_GetLane(cx + 32, check_y);
            if (far_r >= LANE_1 && far_r <= LANE_4)
                car->x += CAR_STEER_HARD;
            else if (far_l >= LANE_1 && far_l <= LANE_4)
                car->x -= CAR_STEER_HARD;
            else
                car->x += ((game_frame_count + car->id) & 2) ? CAR_STEER_HARD : -CAR_STEER_HARD;
        }
        else
        {
            car->x += ((game_frame_count + car->id) & 1) ? CAR_STEER_HARD : -CAR_STEER_HARD;
        }
        car->lateral_accum = 0;
        return;
    }
 
    if (car->pursuit_timer > 0 && car->pursuit_timer < 94)
    {
        /* Side safety */
        if (!ok_l)
            car->x += 2;
        else if (!ok_r)
            car->x -= 2;
        
        /* Lookahead safety — don't drive into a curve during pursuit */
        UBYTE ahead_c = Collision_GetLane(cx, check_y - 32);
        if (ahead_c < LANE_1 || ahead_c > LANE_4)
        {
            UBYTE ahead_l = Collision_GetLane(cx - 16, check_y - 32);
            UBYTE ahead_r = Collision_GetLane(cx + 16, check_y - 32);
            if (ahead_r >= LANE_1 && ahead_r <= LANE_4)
                car->x += 2;
            else if (ahead_l >= LANE_1 && ahead_l <= LANE_4)
                car->x -= 2;
        }
        
        car->lateral_accum = 0;
        return;
    }
    
    /* ---- NORMAL DRIVING: full steering with accumulator ---- */
    BYTE steer = 0;
    
    if (!ok_l || !ok_r)
    {
        if (!ok_l)
            steer = 2;
        else
            steer = -2;
    }
    else
    {
        UBYTE ahead_l = Collision_GetLane(cx - 16, check_y - 32);
        UBYTE ahead_c = Collision_GetLane(cx, check_y - 32);
        UBYTE ahead_r = Collision_GetLane(cx + 16, check_y - 32);
        
        BOOL ahead_ok_l = (ahead_l >= LANE_1 && ahead_l <= LANE_4);
        BOOL ahead_ok_c = (ahead_c >= LANE_1 && ahead_c <= LANE_4);
        BOOL ahead_ok_r = (ahead_r >= LANE_1 && ahead_r <= LANE_4);
        
        if (!ahead_ok_c)
        {
            if (ahead_ok_r && !ahead_ok_l)
                steer = 2;
            else if (ahead_ok_l && !ahead_ok_r)
                steer = -2;
        }
        else if (!ahead_ok_l)
        {
            steer = 1;
        }
        else if (!ahead_ok_r)
        {
            steer = -1;
        }
        else
        {
            WORD car_row = (car->y >> 4) & 0xFF;
            WORD ahead_row = ((car->y - 32) >> 4) & 0xFF;
            WORD center_now = road_center_cache[car_row];
            WORD center_ahead = road_center_cache[ahead_row];
            WORD curve = center_ahead - center_now;
            
            if (curve < -4)
                steer = -1;
            else if (curve > 4)
                steer = 1;
            else
            {
                if (car->lateral_accum > 0)
                    steer = -1;
                else if (car->lateral_accum < 0)
                    steer = 1;
                else
                    steer = 0;
            }
        }
    }
    
    WORD new_accum = car->lateral_accum + steer;
    if (new_accum < -1) new_accum = -1;   
    if (new_accum > 1) new_accum = 1;
    car->lateral_accum = (BYTE)new_accum;
    car->x += car->lateral_accum;
}

void Cars_PursuePlayer(BlitterObject *car)
{
 
    if (bike_speed < 60) { car->pursuit_timer = 0; return; }
    if (cars_passed < 4) { car->pursuit_timer = 0; return; }
    
    WORD cx = car->x + 16;
    WORD car_screen_y = car->y - mapposy;
    
    if (car_screen_y >= bike_position_y) { car->pursuit_timer = 0; return; }
    
    WORD y_gap = bike_position_y - car_screen_y;
    
    if (y_gap < 64) { car->pursuit_timer = 0; return; }
    if (y_gap > 160) { car->pursuit_timer = 0; return; }
    
    WORD x_diff = bike_position_x - cx;
    WORD abs_diff = ABS(x_diff);
    
    if (abs_diff < 26) { car->pursuit_timer = 0; return; }
    if (abs_diff > 48) { car->pursuit_timer = 0; return; }
    
    if (car->pursuit_timer >= 94)
        return;
    
    car->pursuit_timer++;
    
    if (car->pursuit_timer == 1)
        car->lateral_accum = 0;
    
    if ((game_frame_count + car->id) & 3) return;
    
    if (x_diff > 0)
        car->x += 1;
    else
        car->x -= 1;
}
 
void Cars_ScatterAfterCrash(void)
{
    LONG bike_y = mapposy + bike_position_y;
    
    for (int i = 0; i < MAX_CARS; i++)
    {
        /* Park all cars far off screen, inactive */
        car[i].y = bike_y - SCREENHEIGHT - 200;
        car[i].x = FAR_LEFT_LANE;
        car[i].off_screen = TRUE;
        car[i].speed = 0;  
        car[i].crashed = FALSE;
        car[i].has_blocked_bike = FALSE;
        car[i].block_timer = 0;
        car[i].accumulator = 0;
        car[i].needs_restore = FALSE;
        
        car[i].old_x = car[i].x;
        car[i].old_y = car[i].y;
        car[i].prev_old_x = car[i].x;
        car[i].prev_old_y = car[i].y;

        car[i].lateral_accum = 0;
        car[i].pursuit_timer = 0;
        car[i].swerving = FALSE;
        car[i].swerve_timer = 0;
        car[i].honking = FALSE;
        car[i].honk_timer = 0;
        
        car_was_ahead[i] = TRUE;
        car[i].spawning = FALSE;
    }
    
    /* Short pause, then normal spawn cadence resumes — 1 car at a time */
    respawn_timer = 60;   /* 1 second */
}

void Cars_RemoveAllFromScreen(void)
{
    for (int i = 0; i < MAX_CARS; i++)
    {
 
        // Mark as completely off screen and inactive
        car[i].off_screen = TRUE;
        car[i].needs_restore = FALSE;
    }

    Cars_PreDraw();
}

void Cars_UpdatePosition(BlitterObject *c)
{
    if (!c->visible) return;
    
    if (c->crashed)
    {
        if (c->crash_timer > 0) { c->crash_timer--; return; }
        else { c->crashed = FALSE; }
    }
    
   c->moved = TRUE;
    
 
    if (c->swerving)
    {
        if (c->speed > 0)
        {
            c->speed -= 20;
            if (c->speed < 0) c->speed = 0;
        }

        c->swerve_timer--;
        if (c->swerve_timer == 0)
        {
            c->swerving = FALSE;
        }
    }
    else
    {
        /* Normal speed adjustment — only when NOT swerving */
        WORD target;

        if (c->target_speed > 150)
            target = 1280;
        else if (c->target_speed > 120)
            target = 1024;
        else
            target = 768;

        if (cars_passed < 15)
            target -= 128;

        if (collision_state == COLLISION_TRAFFIC)
            target += 512;

        if (c->speed < target)
        {
            c->speed += 14;
            if (c->speed > target) c->speed = target;
        }
        else if (c->speed > target)
        {
            c->speed -= 14;
            if (c->speed < target) c->speed = target;
        }
    }   
    
    c->accumulator += c->speed;
    
    if (c->accumulator >= 256)
    {
        WORD pixels = c->accumulator >> 8;
        c->y -= pixels;
        c->accumulator &= 0xFF;
    }
    
    /* Wheel animation */
    c->anim_counter++;
    WORD threshold = (bike_speed < 84) ? 8 : (bike_speed < 126) ? 4 : 2;
    if (c->anim_counter >= threshold)
    {
        c->anim_counter = 0;
        c->anim_frame ^= 1;
    }
}
 
void Cars_Tick(BlitterObject *car)
{
    WORD temp_x = car->old_x;
    WORD temp_y = car->old_y;
    
    car->old_x = car->prev_old_x;
    car->old_y = car->prev_old_y;
    
    // Only restore if prev_old position was on screen
    WORD prev_screen_y = car->prev_old_y - mapposy;
    if (prev_screen_y >= -BOB_HEIGHT && prev_screen_y < SCREENHEIGHT + BOB_HEIGHT)
    {
        Cars_CopyPristineBackground(car);
    }
    
    car->old_x = temp_x;
    car->old_y = temp_y;
 
    Cars_UpdatePosition(car);
    Cars_CheckLaneAndSteer(car);
  
    if (!car->swerving && !car->crashed)
        Cars_PursuePlayer(car);

    car->prev_old_x = car->old_x;
    car->prev_old_y = car->old_y;
    car->old_x = car->x;
    car->old_y = car->y;

    Cars_RenderBOB(car);
    car->needs_restore = TRUE;
}

 
void Cars_CheckForRespawn(void)
{
    if (cars_spawn_disabled)
    {
        static int dc = 0;
        if ((dc++ & 127) == 0)
            KPrintF("Respawn blocked (disabled)\n");
        return;
    }
    if (collision_state != COLLISION_NONE) return;
    if (respawn_timer > 0) { respawn_timer--; return; }
    
    WORD active_count = 0;
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (car[i].visible && (!car[i].off_screen ))
            active_count++;
    }
    
    WORD max_active;
    WORD base_max;
    
    switch (game_difficulty)
    {
        case FIVEHUNDREDCC:     base_max = 2; break;
        case SEVENFIFTYCC:      base_max = 3; break;
        case TWELVEHUNDREDCC:   base_max = 4; break;
        default:                base_max = 2; break;
    }
    
    if (cars_passed < 3)
        max_active = 1;
    else if (cars_passed < 8)
        max_active = base_max - 1;
    else
        max_active = base_max;
    
    if (active_count >= max_active) return;
    
    static UBYTE next_slot = 0;
    
    for (int attempt = 0; attempt < MAX_CARS; attempt++)
    {
        int i = (next_slot + attempt) % MAX_CARS;
        
        // Skip cars that are active or freshly spawned
        if (!car[i].off_screen) continue;   // On screen — active
        if (car[i].spawning) continue;       // Just spawned — waiting to appear
        
        WORD spawn_y;
        WORD spawn_x;
        WORD attempts = 0;
        
        WORD scroll = GetScrollAmount(bike_speed);
 
        if (game_rank >= 99)
        {
            // Last place — only spawn ahead (top of screen)
            // No cars coming from behind to overtake
            spawn_y = mapposy - 32;
        }
        else if (game_rank <= 1)
        {
            // First place — only spawn behind (bottom of screen)
            // No cars ahead to pass
            spawn_y = mapposy + SCREENHEIGHT + 32;
        }
        else
        {
            // Normal — speed determines direction
            if (scroll >= 768)
                spawn_y = mapposy - 32;
            else
                spawn_y = mapposy + SCREENHEIGHT + 32;
        }
        
        do {
            spawn_x = 16 + ((game_frame_count * 7 + i * 31 + attempts * 13) & 127);
            UBYTE lane = Collision_GetLane(spawn_x + 16, spawn_y);
            if (lane >= LANE_1 && lane <= LANE_4) break;
            attempts++;
        } while (attempts < 10);
        
        if (attempts >= 10) continue;
        
        if (car[i].target_speed > 150)
            car[i].speed = 1280;
        else if (car[i].target_speed > 120)
            car[i].speed = 1024;
        else
            car[i].speed = 768;

        car[i].x = spawn_x;
        car[i].y = spawn_y;
        car[i].off_screen = TRUE;
        car[i].spawning = TRUE;      // Mark as freshly spawned
        car[i].crashed = FALSE;
        car[i].has_blocked_bike = FALSE;
        car[i].accumulator = 0;
        car[i].needs_restore = 0;
        car[i].old_x = car[i].x;
        car[i].old_y = car[i].y;
        car[i].prev_old_x = car[i].x;
        car[i].prev_old_y = car[i].y;

        car[i].lateral_accum= 0;
        car[i].swerving = FALSE;
        car[i].pursuit_timer = 0; 
        car[i].swerve_timer = 0;
        
        car_was_ahead[i] = (car[i].y < bike_world_y);
        
        next_slot = (i + 1) % MAX_CARS;

        if (cars_passed < 3)
            respawn_timer = 180;    // 3 seconds — plenty of breathing room
        else  
            respawn_timer = 120;    // 2 seconds
 
        return;
    }
}

#define PASS_DEADZONE 48  // Must be 48px apart before re-triggering

void Cars_CheckPassing(BlitterObject *c)
{
    if (stage_state != STAGE_PLAYING) return;
    
    LONG car_world_y = c->y;
    LONG distance = car_world_y - bike_world_y;
    
    // Positive = car is behind bike, negative = car is ahead
    BOOL car_is_ahead = (distance < -PASS_DEADZONE);
    BOOL car_is_behind = (distance > PASS_DEADZONE);
    
    // In the dead zone — don't change state
    if (!car_is_ahead && !car_is_behind) return;
    
    if (car_was_ahead[c->id] && car_is_behind)
    {
        // Car was ahead, now clearly behind — bike passed it
        if (fuel_alarm_active == FALSE)
        {
            SFX_Play(SFX_OVERHEADOVERTAKE);
        }
        game_score += 500;
        cars_passed++;
        
        if (game_rank > 1)
            game_rank--;

        if (game_rank < game_best_rank)
            game_best_rank = game_rank;     
 
        car_was_ahead[c->id] = FALSE;
    }
    else if (!car_was_ahead[c->id] && car_is_ahead)
    {
        // Car was behind, now clearly ahead — car passed bike
        if (game_rank < 99)
            game_rank++;
        
        car_was_ahead[c->id] = TRUE;
    }
}

