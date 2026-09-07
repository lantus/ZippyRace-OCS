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
#include "sprites.h"
#include "blitter.h"
#include "timers.h"
#include "cars.h"
#include "memory.h"
#include "roadsystem.h"
#include "hardware.h"
#include "copper.h"
#include "barreltruck.h"
#include "motorbike.h"

extern volatile struct Custom *custom;

static Sprite spr_rsrc_bike_moving1;
static Sprite spr_rsrc_bike_moving2;
static Sprite spr_rsrc_bike_moving3;

 
static Sprite spr_rsrc_bike_moving2_or;
static Sprite spr_rsrc_bike_moving3_or;

static Sprite spr_rsrc_bike_turn_left1;
static Sprite spr_rsrc_bike_turn_left2;
static Sprite spr_rsrc_bike_turn_right1;
static Sprite spr_rsrc_bike_turn_right2;

static Sprite spr_rsrc_approach_bike_frame1;

static Sprite spr_rsrc_approach_bike_frame1_left1;
static Sprite spr_rsrc_approach_bike_frame1_left2;
static Sprite spr_rsrc_approach_bike_frame1_right1;
static Sprite spr_rsrc_approach_bike_frame1_right2;

static Sprite spr_rsrc_approach_bike_frame2;
static Sprite spr_rsrc_approach_bike_frame3;
static Sprite spr_rsrc_approach_bike_frame4;
static Sprite spr_rsrc_approach_bike_frame5;
static Sprite spr_rsrc_approach_bike_frame6;
static Sprite spr_rsrc_approach_bike_frame7;
static Sprite spr_rsrc_approach_bike_frame8;

static Sprite spr_rsrc_frontview_bike_crash1;
static Sprite spr_rsrc_frontview_bike_crash2;
static Sprite spr_rsrc_frontview_bike_crash3;
static Sprite spr_rsrc_frontview_bike_crash4;

static Sprite spr_rsrc_overhead_bike_crash1;
static Sprite spr_rsrc_overhead_bike_crash2;
static Sprite spr_rsrc_overhead_bike_crash3;
static Sprite spr_rsrc_overhead_bike_crash4;
static Sprite spr_rsrc_overhead_bike_crash5;
static Sprite spr_rsrc_overhead_bike_crash6;
static Sprite spr_rsrc_overhead_bike_crash7;
static Sprite spr_rsrc_overhead_bike_crash8;

static Sprite spr_rsrc_bike_wheelie1;
static Sprite spr_rsrc_bike_wheelie2;
static Sprite spr_rsrc_bike_wheelie3;

ULONG *spr_bike_moving1;
ULONG *spr_bike_moving2;
ULONG *spr_bike_moving3;
ULONG *spr_bike_moving2_or;
ULONG *spr_bike_moving3_or;
ULONG *spr_bike_turn_left1;
ULONG *spr_bike_turn_left2;
ULONG *spr_bike_turn_right1;
ULONG *spr_bike_turn_right2;

ULONG *spr_approach_bike_frame1;
ULONG *spr_approach_bike_frame1_left1;
ULONG *spr_approach_bike_frame1_left2;
ULONG *spr_approach_bike_frame1_right1;
ULONG *spr_approach_bike_frame1_right2;
ULONG *spr_approach_bike_frame2;
ULONG *spr_approach_bike_frame3;
ULONG *spr_approach_bike_frame4;
ULONG *spr_approach_bike_frame5;
ULONG *spr_approach_bike_frame6;
ULONG *spr_approach_bike_frame7;
ULONG *spr_approach_bike_frame8;

ULONG *spr_frontview_bike_crash1;
ULONG *spr_frontview_bike_crash2;
ULONG *spr_frontview_bike_crash3;
ULONG *spr_frontview_bike_crash4;

ULONG *spr_overhead_bike_crash1;
ULONG *spr_overhead_bike_crash2;
ULONG *spr_overhead_bike_crash3;
ULONG *spr_overhead_bike_crash4;
ULONG *spr_overhead_bike_crash5;
ULONG *spr_overhead_bike_crash6;
ULONG *spr_overhead_bike_crash7;
ULONG *spr_overhead_bike_crash8;

ULONG *spr_bike_wheelie1;
ULONG *spr_bike_wheelie2;
ULONG *spr_bike_wheelie3;

ULONG *current_bike_sprite;
 
UBYTE bike_state = BIKE_STATE_MOVING;

UWORD bike_position_x;
UWORD bike_position_y;
LONG  bike_world_y = 0;
ULONG bike_anim_frames = 0;

 
static ULONG bike_anim_lr_frames = 0;
static UBYTE bike_frame = 0;
static BikeFrame current_bike_frame = -1;
 

// Add these to your global variables
WORD bike_speed = 42;        // Current speed in mph (starts at idle)
WORD bike_acceleration = 0;  // Current acceleration
WORD scroll_accumulator = 0;  // Fractional scroll position (fixed point)

static UBYTE approach_frame_index = 1;  // Track which approach frame we're on
static UBYTE vibration_counter = 0;
static UBYTE current_sprite_count = 2;  // Track whether using 2 or 4 sprites

BOOL  wheelie_active = FALSE;
BOOL  wheelie_scored = FALSE;
WORD  wheelie_speed = 0;
 
UWORD wheelie_anim_frames = 0;
UWORD crash_anim_frames = 0;
UBYTE crash_spin_frame = 0;
 
GameTimer wheelie_timer;
GameTimer invuln_timer;
GameTimer jump_timer;

static const USHORT palette_750cc[8] = {
    0x000,  /* 0: black */
    0x0BF,  /* 1: light blue */
    0xFFF,  /* 2: white */
    0xD60,  /* 3: orange */
    0xDF0,  /* 4: yellow-green */
    0xD00,  /* 5: red */
    0x00F,  /* 6: blue */
    0x002,  /* 7: dark blue */
};

static const USHORT palette_1200cc[8] = {
    0x000,  /* 0: black */
    0xFFF,  /* 1: white */
    0x0F0,  /* 2: green */
    0xD60,  /* 3: orange */
    0xDF0,  /* 4: yellow-green */
    0xD00,  /* 5: red */
    0x00F,  /* 6: blue */
    0x002,  /* 7: dark blue */
};

void MotorBike_Initialize()
{
    Sprites_LoadFromFile(BIKE_MOVING1,&spr_rsrc_bike_moving1);
    Sprites_LoadFromFile(BIKE_MOVING2,&spr_rsrc_bike_moving2);
    Sprites_LoadFromFile(BIKE_MOVING3,&spr_rsrc_bike_moving3);

    Sprites_LoadFromFile(BIKE_MOVING2_OR, &spr_rsrc_bike_moving2_or);
    Sprites_LoadFromFile(BIKE_MOVING3_OR, &spr_rsrc_bike_moving3_or);

    Sprites_LoadFromFile(BIKE_LEFT1,&spr_rsrc_bike_turn_left1);
    Sprites_LoadFromFile(BIKE_LEFT2,&spr_rsrc_bike_turn_left2);
    Sprites_LoadFromFile(BIKE_RIGHT1,&spr_rsrc_bike_turn_right1);
    Sprites_LoadFromFile(BIKE_RIGHT2,&spr_rsrc_bike_turn_right2);

    // Frontview Bike Sprites

    Sprites_LoadFromFile(APPROACH_BIKE1,&spr_rsrc_approach_bike_frame1);
    Sprites_LoadFromFile(APPROACH_BIKE2,&spr_rsrc_approach_bike_frame2);
    Sprites_LoadFromFile(APPROACH_BIKE3,&spr_rsrc_approach_bike_frame3);
    Sprites_LoadFromFile(APPROACH_BIKE4,&spr_rsrc_approach_bike_frame4);
    Sprites_LoadFromFile(APPROACH_BIKE5,&spr_rsrc_approach_bike_frame5);
    Sprites_LoadFromFile(APPROACH_BIKE6,&spr_rsrc_approach_bike_frame6);
    Sprites_LoadFromFile(APPROACH_BIKE7,&spr_rsrc_approach_bike_frame7);
    Sprites_LoadFromFile(APPROACH_BIKE8,&spr_rsrc_approach_bike_frame8); 
 
    Sprites_LoadFromFile(APPROACH_BIKE_LEFT1, &spr_rsrc_approach_bike_frame1_left1);
    Sprites_LoadFromFile(APPROACH_BIKE_LEFT2, &spr_rsrc_approach_bike_frame1_left2);
    Sprites_LoadFromFile(APPROACH_BIKE_RIGHT1, &spr_rsrc_approach_bike_frame1_right1);
    Sprites_LoadFromFile(APPROACH_BIKE_RIGHT2, &spr_rsrc_approach_bike_frame1_right2);

     // Frontview Crash
    Sprites_LoadFromFile(FRONTVIEW_BIKECRASH1,&spr_rsrc_frontview_bike_crash1);
    Sprites_LoadFromFile(FRONTVIEW_BIKECRASH2,&spr_rsrc_frontview_bike_crash2);
    Sprites_LoadFromFile(FRONTVIEW_BIKECRASH3,&spr_rsrc_frontview_bike_crash3);
    Sprites_LoadFromFile(FRONTVIEW_BIKECRASH4,&spr_rsrc_frontview_bike_crash4); 

    // Overhead Crash
    Sprites_LoadFromFile(OVERHEAD_BIKECRASH1,&spr_rsrc_overhead_bike_crash1);
    Sprites_LoadFromFile(OVERHEAD_BIKECRASH2,&spr_rsrc_overhead_bike_crash2);
    Sprites_LoadFromFile(OVERHEAD_BIKECRASH3,&spr_rsrc_overhead_bike_crash3);
    Sprites_LoadFromFile(OVERHEAD_BIKECRASH4,&spr_rsrc_overhead_bike_crash4); 
    Sprites_LoadFromFile(OVERHEAD_BIKECRASH5,&spr_rsrc_overhead_bike_crash5);
    Sprites_LoadFromFile(OVERHEAD_BIKECRASH6,&spr_rsrc_overhead_bike_crash6);
    Sprites_LoadFromFile(OVERHEAD_BIKECRASH7,&spr_rsrc_overhead_bike_crash7);
    Sprites_LoadFromFile(OVERHEAD_BIKECRASH8,&spr_rsrc_overhead_bike_crash8);    

    // Overhead Wheelie
    Sprites_LoadFromFile(WHEELIE1,&spr_rsrc_bike_wheelie1);
    Sprites_LoadFromFile(WHEELIE2,&spr_rsrc_bike_wheelie2);
    Sprites_LoadFromFile(WHEELIE3,&spr_rsrc_bike_wheelie3); 


    spr_bike_moving1 = Mem_AllocChip(32);
    spr_bike_moving2 = Mem_AllocChip(32);
    spr_bike_moving3 = Mem_AllocChip(32);
    spr_bike_moving2_or = Mem_AllocChip(32);
    spr_bike_moving3_or = Mem_AllocChip(32);
    spr_bike_turn_left1 = Mem_AllocChip(32);
    spr_bike_turn_left2 = Mem_AllocChip(32);
    spr_bike_turn_right1 = Mem_AllocChip(32);
    spr_bike_turn_right2 = Mem_AllocChip(32);

    // Wheelie 

    spr_bike_wheelie1 = Mem_AllocChip(32);
    spr_bike_wheelie2 = Mem_AllocChip(32);
    spr_bike_wheelie3 = Mem_AllocChip(32);

    // Approach City Bike Sprites Mem Alloc

    spr_approach_bike_frame1 = Mem_AllocChip(32);
    spr_approach_bike_frame1_left1  = Mem_AllocChip(32);
    spr_approach_bike_frame1_left2  = Mem_AllocChip(32);
    spr_approach_bike_frame1_right1 = Mem_AllocChip(32);
    spr_approach_bike_frame1_right2 = Mem_AllocChip(32);
    spr_approach_bike_frame2 = Mem_AllocChip(32);
    spr_approach_bike_frame3 = Mem_AllocChip(32);
    spr_approach_bike_frame4 = Mem_AllocChip(32);
    spr_approach_bike_frame5 = Mem_AllocChip(32);  
    spr_approach_bike_frame6 = Mem_AllocChip(32);  
    spr_approach_bike_frame7 = Mem_AllocChip(32);       
    spr_approach_bike_frame8 = Mem_AllocChip(32);     

    spr_frontview_bike_crash1 = Mem_AllocChip(32);  
    spr_frontview_bike_crash2 = Mem_AllocChip(32);  
    spr_frontview_bike_crash3 = Mem_AllocChip(32);       
    spr_frontview_bike_crash4 = Mem_AllocChip(32);     

    spr_overhead_bike_crash1 = Mem_AllocChip(32);  
    spr_overhead_bike_crash2 = Mem_AllocChip(32);  
    spr_overhead_bike_crash3 = Mem_AllocChip(32);       
    spr_overhead_bike_crash4 = Mem_AllocChip(32);     
    spr_overhead_bike_crash5 = Mem_AllocChip(32);  
    spr_overhead_bike_crash6 = Mem_AllocChip(32);  
    spr_overhead_bike_crash7 = Mem_AllocChip(32);       
    spr_overhead_bike_crash8 = Mem_AllocChip(32);        
    

    Sprites_BuildComposite(spr_bike_moving1,2,&spr_rsrc_bike_moving1);
    Sprites_BuildComposite(spr_bike_moving2,2,&spr_rsrc_bike_moving2);
    Sprites_BuildComposite(spr_bike_moving3,2,&spr_rsrc_bike_moving3);
    Sprites_BuildComposite(spr_bike_moving2_or,2,&spr_rsrc_bike_moving2_or);
    Sprites_BuildComposite(spr_bike_moving3_or,2,&spr_rsrc_bike_moving3_or);
    Sprites_BuildComposite(spr_bike_turn_left1,2,&spr_rsrc_bike_turn_left1);
    Sprites_BuildComposite(spr_bike_turn_left2,2,&spr_rsrc_bike_turn_left2);
    Sprites_BuildComposite(spr_bike_turn_right1,2,&spr_rsrc_bike_turn_right1);
    Sprites_BuildComposite(spr_bike_turn_right2,2,&spr_rsrc_bike_turn_right2);

    // Approach City Bike Sprites Composites

    Sprites_BuildComposite(spr_approach_bike_frame1,4,&spr_rsrc_approach_bike_frame1);
    Sprites_BuildComposite(spr_approach_bike_frame2,4,&spr_rsrc_approach_bike_frame2);
    Sprites_BuildComposite(spr_approach_bike_frame3,4,&spr_rsrc_approach_bike_frame3);
    Sprites_BuildComposite(spr_approach_bike_frame4,4,&spr_rsrc_approach_bike_frame4); 
    Sprites_BuildComposite(spr_approach_bike_frame5,4,&spr_rsrc_approach_bike_frame5);
    Sprites_BuildComposite(spr_approach_bike_frame6,4,&spr_rsrc_approach_bike_frame6);   
    Sprites_BuildComposite(spr_approach_bike_frame7,4,&spr_rsrc_approach_bike_frame7);   
    Sprites_BuildComposite(spr_approach_bike_frame8,4,&spr_rsrc_approach_bike_frame8);   

    Sprites_BuildComposite(spr_approach_bike_frame1_left1, 4, &spr_rsrc_approach_bike_frame1_left1);
    Sprites_BuildComposite(spr_approach_bike_frame1_left2, 4, &spr_rsrc_approach_bike_frame1_left2);
    Sprites_BuildComposite(spr_approach_bike_frame1_right1, 4, &spr_rsrc_approach_bike_frame1_right1);
    Sprites_BuildComposite(spr_approach_bike_frame1_right2, 4, &spr_rsrc_approach_bike_frame1_right2);

    Sprites_BuildComposite(spr_frontview_bike_crash1,4,&spr_rsrc_frontview_bike_crash1);
    Sprites_BuildComposite(spr_frontview_bike_crash2,4,&spr_rsrc_frontview_bike_crash2);   
    Sprites_BuildComposite(spr_frontview_bike_crash3,4,&spr_rsrc_frontview_bike_crash3);   
    Sprites_BuildComposite(spr_frontview_bike_crash4,4,&spr_rsrc_frontview_bike_crash4);   

    Sprites_BuildComposite(spr_overhead_bike_crash1,4,&spr_rsrc_overhead_bike_crash1);
    Sprites_BuildComposite(spr_overhead_bike_crash2,4,&spr_rsrc_overhead_bike_crash2);   
    Sprites_BuildComposite(spr_overhead_bike_crash3,4,&spr_rsrc_overhead_bike_crash3);   
    Sprites_BuildComposite(spr_overhead_bike_crash4,4,&spr_rsrc_overhead_bike_crash4);      
    Sprites_BuildComposite(spr_overhead_bike_crash5,4,&spr_rsrc_overhead_bike_crash5);
    Sprites_BuildComposite(spr_overhead_bike_crash6,4,&spr_rsrc_overhead_bike_crash6);   
    Sprites_BuildComposite(spr_overhead_bike_crash7,4,&spr_rsrc_overhead_bike_crash7);   
    Sprites_BuildComposite(spr_overhead_bike_crash8,4,&spr_rsrc_overhead_bike_crash8);      


    Sprites_BuildComposite(spr_bike_wheelie1,4,&spr_rsrc_bike_wheelie1);   
    Sprites_BuildComposite(spr_bike_wheelie2,4,&spr_rsrc_bike_wheelie2);   
    Sprites_BuildComposite(spr_bike_wheelie3,4,&spr_rsrc_bike_wheelie3);       

    Sprites_ApplyPalette(&spr_rsrc_approach_bike_frame1);
 
}
 
void MotorBike_Reset()
{
    bike_frame = 0; 

    if (game_map == MAP_ATTRACT_INTRO ||
        game_map == STAGE1_FRONTVIEW ||
        game_map == STAGE2_FRONTVIEW ||
        game_map == STAGE3_FRONTVIEW ||
        game_map == STAGE4_FRONTVIEW || 
        game_map == STAGE5_FRONTVIEW)
    {
        MotorBike_SetFrame(BIKE_FRAME_APPROACH1);
    }
    else
    {
        MotorBike_SetFrame(BIKE_FRAME_MOVING1);
    }

    MotorBike_SetDefaultPalette();
 
}

void MotorBike_SetDefaultPalette()
{
    const USHORT *palette;
    
    switch (game_difficulty)
    {
        case SEVENFIFTYCC:    palette = palette_750cc;  break;
        case TWELVEHUNDREDCC: palette = palette_1200cc; break;
        default:
            Sprites_ApplyPalette(&spr_rsrc_approach_bike_frame1);
            return;
    }
    
    for (int i = 0; i < 8; i++)
        Copper_SetSpritePalette(i, palette[i]);
    
    Copper_SetScoreRestoreColors();
}

void MotorBike_Draw(WORD x, UWORD y, UBYTE state)
{
    if (current_sprite_count == 2)
    {
        // 16-pixel wide sprite (2 hardware sprites)
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[0], x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[1], x, y, 32);
        Sprites_ClearHigher();
    }
    else // current_sprite_count == 4
    {
        // 32-pixel wide sprite (4 hardware sprites)
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[0], x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[1], x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[2], x+16, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[3], x+16, y, 32);
    }
}

void MotorBike_UpdatePosition(UWORD x, UWORD y, UBYTE state)
{
    current_sprite_count = 2;

    if (bike_invulnerable)
    {
        if (game_frame_count & 4)
        {
            // Hide sprites — move them off screen
            Sprites_ClearLower();
            Sprites_ClearHigher();
            return;
        }
    }

    if (state == BIKE_STATE_MOVING || 
        state == BIKE_STATE_ACCELERATING  || 
        state == BIKE_STATE_BRAKING)
    {
        bike_anim_lr_frames = 0;

        if (bike_anim_frames % (state == BIKE_STATE_ACCELERATING ? 5 : 10) == 0)
        {
            if (game_stage == STAGE_HOUSTON || game_stage == STAGE_CHICAGO)
            {
                /* Offroad: center, right, center, left */
                bike_frame++;
                if (bike_frame >= 4)
                    bike_frame = 0;

                switch (bike_frame)
                {
                    case 0: current_bike_sprite = spr_bike_moving1;    break;
                    case 1: current_bike_sprite = spr_bike_moving3_or; break;
                    case 2: current_bike_sprite = spr_bike_moving1;    break;
                    case 3: current_bike_sprite = spr_bike_moving2_or; break;
                }
            }
            else
            {
                /* City: toggle between 2 frames */
                bike_frame = (bike_frame + 1) & 1;

                if (bike_frame == 0)
                    current_bike_sprite = spr_bike_moving2;
                else
                    current_bike_sprite = spr_bike_moving3;
            }
        }
    }
    else if (state == BIKE_STATE_WATER_CRASH)
    {
        crash_anim_frames++;
         
        if (crash_anim_frames < 2)
            current_bike_sprite = spr_overhead_bike_crash1;
        else if (crash_anim_frames < 4)
            current_bike_sprite = spr_overhead_bike_crash2;
        else
            current_bike_sprite = spr_approach_bike_frame3;  /* Nosedive */
        current_sprite_count = 4;
    }
    else if (state == BIKE_STATE_FRONTVIEW_LEFT)
    {
        /* Flip between frame1 and frame2 every 4 game frames */
        if ((bike_anim_frames >> 2) & 1)
            current_bike_sprite = spr_approach_bike_frame1_left2;
        else
            current_bike_sprite = spr_approach_bike_frame1_left1;
        current_sprite_count = 4;

        KPrintF("pulling lef\n");
    }
    else if (state == BIKE_STATE_FRONTVIEW_RIGHT)
    {
        if ((bike_anim_frames >> 2) & 1)
            current_bike_sprite = spr_approach_bike_frame1_right2;
        else
            current_bike_sprite = spr_approach_bike_frame1_right1;

        current_sprite_count = 4;

         KPrintF("pulling right\n");
    }
    else if (state == BIKE_STATE_LEFT)
    {
         if (bike_anim_lr_frames >= 6)
         {
            current_bike_sprite = spr_bike_turn_left2;
         }
         else
         {
            current_bike_sprite = spr_bike_turn_left1;
         }

         bike_anim_lr_frames++;
    }
    else if (state == BIKE_STATE_RIGHT)
    {
         if (bike_anim_lr_frames >= 6)
         {
            current_bike_sprite = spr_bike_turn_right2;
         }
         else
         {
            current_bike_sprite = spr_bike_turn_right1;
         }

         bike_anim_lr_frames++;
    }
    else if (state == BIKE_STATE_WHEELIE || 
             state == BIKE_STATE_JUMP)
    {
        if (Timer_GetRemainingMs(&wheelie_timer) <= 500)  
        {
            current_bike_sprite = spr_bike_wheelie3;
        }
        else
        {
            if ((bike_anim_frames >> 2) & 1)   
                current_bike_sprite = spr_bike_wheelie2;
            else
                current_bike_sprite = spr_bike_wheelie1;
        }
        
        current_sprite_count = 4;
        bike_anim_lr_frames = 0;
    }
    else if (state == BIKE_STATE_CRASHED)
    {
        crash_anim_frames++;
        
        // Spin speed — starts moderate, decelerates to stop
        UBYTE spin_delay;
        
        if (crash_anim_frames < 40)
            spin_delay = 4;      // Initial spin — not too fast
        else if (crash_anim_frames < 80)
            spin_delay = 6;      // Slowing
        else if (crash_anim_frames < 110)
            spin_delay = 8;      // Slower
        else if (crash_anim_frames < 140)
            spin_delay = 12;     // Crawling
        else if (crash_anim_frames < 160)
            spin_delay = 16;     // Almost stopped
        else
            spin_delay = 0;      // Stopped
        
        if (spin_delay > 0 && (crash_anim_frames % spin_delay) == 0)
        {
            crash_spin_frame = (crash_spin_frame + 1) & 7;
        }
        
        switch (crash_spin_frame)
        {
            case 0: current_bike_sprite = spr_overhead_bike_crash1; break;
            case 1: current_bike_sprite = spr_overhead_bike_crash2; break;
            case 2: current_bike_sprite = spr_overhead_bike_crash3; break;
            case 3: current_bike_sprite = spr_overhead_bike_crash4; break;
            case 4: current_bike_sprite = spr_overhead_bike_crash5; break;
            case 5: current_bike_sprite = spr_overhead_bike_crash6; break;
            case 6: current_bike_sprite = spr_overhead_bike_crash7; break;
            case 7: current_bike_sprite = spr_overhead_bike_crash8; break;
        }
        
        current_sprite_count = 4;
    }

    // Center adjustment: 16px sprites are at x, 32px sprites need x-8
    WORD draw_x = x;
    if (current_sprite_count == 4)
    {
        draw_x = x - 8;  // Shift left 8px to center 32px over 16px position
    }
    
    Sprites_SetPointers(current_bike_sprite, current_sprite_count, SPRITEPTR_ZERO_AND_ONE);
    
    if (current_sprite_count == 2)
    {
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[0], draw_x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[1], draw_x, y, 32);
    }
    else
    {
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[0], draw_x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[1], draw_x, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[2], draw_x + 16, y, 32);
        Sprites_SetScreenPosition((UWORD *)current_bike_sprite[3], draw_x + 16, y, 32);
    }
    
    bike_anim_frames++;
}

void AccelerateMotorBike()
{
    bike_state = BIKE_STATE_ACCELERATING;
    bike_speed += ACCEL_RATE;
    if (bike_speed > max_stage_speed)
    {
        bike_speed = max_stage_speed;
    }
}

void BrakeMotorBike()
{
    if (bike_speed > MIN_SPEED)
    {
        bike_speed -= DECEL_RATE;
    }

    if (bike_speed < MIN_SPEED)
    {
        bike_speed = MIN_SPEED;
    }
}
 
void MotorBike_SetFrame(BikeFrame frame)
{
    UBYTE num_sprites;

    if (current_bike_frame == frame) return;
    
    current_bike_frame = frame;
    
    switch(frame)
    {
        case BIKE_FRAME_MOVING1:
            current_bike_sprite = spr_bike_moving1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_MOVING2:
            current_bike_sprite = spr_bike_moving2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_MOVING3:
            current_bike_sprite = spr_bike_moving3;
            num_sprites = 4;
            break;
        case BIKE_FRAME_LEFT1:
            current_bike_sprite = spr_bike_turn_left1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_LEFT2:
            current_bike_sprite = spr_bike_turn_left2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_RIGHT1:
            current_bike_sprite = spr_bike_turn_right1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_RIGHT2:
            current_bike_sprite = spr_bike_turn_right2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH1:
            current_bike_sprite = spr_approach_bike_frame1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH2:
            current_bike_sprite = spr_approach_bike_frame2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH3:
            current_bike_sprite = spr_approach_bike_frame3;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH4:
            current_bike_sprite = spr_approach_bike_frame4;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH5:
            current_bike_sprite = spr_approach_bike_frame5;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH6:
            current_bike_sprite = spr_approach_bike_frame6;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH7:
            current_bike_sprite = spr_approach_bike_frame7;
            num_sprites = 4;
            break;          
        case BIKE_FRAME_APPROACH8:
            current_bike_sprite = spr_approach_bike_frame8;
            num_sprites = 4;
            break;                                                                        
        case BIKE_FRAME_APPROACH1_LEFT:
            current_bike_sprite = spr_approach_bike_frame1_left1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH2_LEFT:
            current_bike_sprite = spr_approach_bike_frame1_left2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH1_RIGHT:
            current_bike_sprite = spr_approach_bike_frame1_right1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_APPROACH2_RIGHT:
            current_bike_sprite = spr_approach_bike_frame1_right2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_CRASH1:
            current_bike_sprite = spr_frontview_bike_crash1;
            num_sprites = 4;
            break;
        case BIKE_FRAME_CRASH2:
            current_bike_sprite = spr_frontview_bike_crash2;
            num_sprites = 4;
            break;
        case BIKE_FRAME_CRASH3:
            current_bike_sprite = spr_frontview_bike_crash3;
            num_sprites = 4;
            break;
        case BIKE_FRAME_CRASH4:
            current_bike_sprite = spr_frontview_bike_crash4;
            num_sprites = 4;
            break;            
        default:
            return;
    }
    
    current_sprite_count = num_sprites;  // Store for MotorBike_Draw

    Sprites_SetPointers(current_bike_sprite, num_sprites, SPRITEPTR_ZERO_AND_ONE);
}

void MotorBike_UpdateApproachFrame(UWORD y)
{
    BikeFrame new_frame;
    UBYTE new_index = 1;  // Default to frame 1
    
    // Determine frame based on Y position (perspective scaling)
    if (y >= 176)
    {
        new_frame = BIKE_FRAME_APPROACH1;
        new_index = 1;
    }
    else if (y >= 144)
    {
        new_frame = BIKE_FRAME_APPROACH3;
        new_index = 3;
    }
    else if (y >= 128)
    {
        new_frame = BIKE_FRAME_APPROACH4;
        new_index = 4;
    }
    else if (y >= 112)
    {
        new_frame = BIKE_FRAME_APPROACH5;
        new_index = 5;
    }
    else if (y >= 102)
    {
        new_frame = BIKE_FRAME_APPROACH6;
        new_index = 6;
    }
    else if (y >= 96)
    {
        new_frame = BIKE_FRAME_APPROACH7;
        new_index = 7;
    }    
    else if (y >= 88)
    {
        new_frame = BIKE_FRAME_APPROACH8;
        new_index = 8;
    }       
    else   
    {
        // turn off sprites
        Sprites_ClearLower();
        Sprites_ClearHigher();
        new_frame = -1;
        new_index = -1;
    }
    
    approach_frame_index = new_index;
    MotorBike_SetFrame(new_frame);
}

WORD MotorBike_GetVibrationOffset(void)
{
    // Only vibrate for frames 2 and beyond (smaller/distant sprites)
    if (approach_frame_index < 2)
    {
        return 0;
    }
    
    // Alternate between -1, 0, +1 for subtle vibration
    vibration_counter++;
    
    switch (vibration_counter % 2)
    {
        case 0: return -1;
        case 1: return 0;
        case 2: return 1;
        case 3: return 0;
        default: return 0;
    }
}

void MotorBike_AutoAccelerate(void)
{
    // Auto-accelerate to minimum cruising speed (42 mph)
    if (bike_speed < MIN_CRUISING_SPEED)
    {
        bike_speed += ACCEL_RATE;
        if (bike_speed > MIN_CRUISING_SPEED)
        {
            bike_speed = MIN_CRUISING_SPEED;
        }
        bike_state = BIKE_STATE_ACCELERATING;
    }
    else
    {
        bike_state = BIKE_STATE_MOVING;
    }
}

void MotorBike_DecelerateToCruising(void)
{
    // Decelerate back to cruising speed (42 mph)
    if (bike_speed > MIN_CRUISING_SPEED)
    {
        bike_speed -= DECEL_RATE;
        if (bike_speed < MIN_CRUISING_SPEED)
        {
            bike_speed = MIN_CRUISING_SPEED;
        }
        bike_state = BIKE_STATE_BRAKING;
    }
}

CollisionState MotorBike_CheckCollision(UWORD *hit_car_index)
{
    WORD bike_cx = bike_position_x + 8;
    WORD bike_top = bike_position_y - g_sprite_voffset;
    WORD bike_bottom = bike_top + 28;
    WORD bike_left = bike_position_x + 2;
    WORD bike_right = bike_position_x + 14;
    
    WORD bike_world_top = mapposy + bike_top;
    WORD bike_world_mid = bike_world_top + 20;
    
    if (wheelie_active && bike_state == BIKE_STATE_JUMP)
    {
        *hit_car_index = -1;
        return COLLISION_NONE;
    }

    if (bike_invulnerable)
    {
        *hit_car_index = -1;
        return COLLISION_NONE;
    }
    
    for (int i = 0; i < MAX_CARS; i++)
    {
        if (!car[i].visible || car[i].off_screen || car[i].crashed) continue;
        
        WORD car_screen_y = car[i].y - mapposy;
        WORD car_screen_x = car[i].x;
        
        WORD x_dist = ABS(bike_cx - (car_screen_x + 16));
        WORD y_dist = ABS(bike_top + 14 - (car_screen_y + 16));
        
        if (x_dist < 12 && y_dist < 24)
        {
            *hit_car_index = i;
            return COLLISION_TRAFFIC;
        }
    }

    /* ---- Barrel truck + dropped barrels ---- */
    if (BarrelTruck_CheckCollision(bike_cx, bike_top))
    {
        *hit_car_index = -1;
        return COLLISION_TRAFFIC;
    }
    
    UBYTE col_center = Collision_Get(bike_cx, bike_world_top);
    UBYTE lane_center = (col_center >> 4) & 0x0F;
    UBYTE surface_center = col_center & 0x0F;

    UBYTE lane_left  = Collision_GetLane(bike_left, bike_world_top);
    UBYTE lane_right = Collision_GetLane(bike_right, bike_world_top);
 
    
    if (lane_center == LANE_OFFROAD)
    {
        if (game_stage == STAGE_HOUSTON || game_stage == STAGE_CHICAGO)
        {
            if (surface_center == SURFACE_WATER)
            {
                UBYTE col_mid = Collision_Get(bike_cx, bike_world_mid);
                UBYTE surface_mid = col_mid & 0x0F;
                   
                if (surface_mid == SURFACE_WATER)
                {
                    *hit_car_index = -1;
                    return COLLISION_WATER;
                }
  
                *hit_car_index = -1;
                return COLLISION_NONE;
            }
        }
 
        *hit_car_index = -1;
        return COLLISION_OFFROAD;
    }
    
    if (lane_left == LANE_OFFROAD && lane_right == LANE_OFFROAD)
    {
        *hit_car_index = -1;
        return COLLISION_OFFROAD;
    }
    
    *hit_car_index = -1;
    return COLLISION_NONE;
}

void MotorBike_CrashAndReposition(void)
{
    
    Game_ApplyPalette((UWORD *)black_palette, BLOCKSCOLORS);
    WaitVBL();
    
    // === STEP 1: Remove ALL car BOBs from BOTH buffers ===
    for (int pass = 0; pass < 2; pass++)
    {
        UBYTE *buffer = (pass == 0) ? screen.bitplanes : screen.offscreen_bitplanes;
        UBYTE *save_draw = draw_buffer;
        draw_buffer = buffer;
        
        for (int i = 0; i < MAX_CARS; i++)
        {
            if (!car[i].visible) continue;
            if (car[i].needs_restore)
            {
                Cars_CopyPristineBackground(&car[i]);
            }
        }
        
        draw_buffer = save_draw;
    }
    
    // === STEP 2: Deactivate all cars — let respawn system handle them ===
    for (int i = 0; i < MAX_CARS; i++)
    {
        car[i].off_screen = TRUE;
        car[i].spawning = FALSE;       // NOT spawning — available for respawn
        car[i].needs_restore = FALSE;
        car[i].honking = FALSE;
        car[i].honk_timer = 0;
        car[i].crashed = FALSE;
        car[i].swerving = FALSE;
        car[i].pursuit_timer = 0;
        car[i].swerve_timer = 0;
        car[i].accumulator = 0;
        car[i].lateral_accum = 0;
        car[i].speed = 0;
        
        car_was_ahead[i] = TRUE;
    }
    
    // === Long respawn delay — 3 seconds of clear road ===
    respawn_timer = 150;
    
    // === STEP 3: Move map back ===
    WORD rewind_amount = SCREENHEIGHT * 2;
    LONG max_mapposy = (mapheight * BLOCKHEIGHT) - SCREENHEIGHT - BLOCKHEIGHT - 1;

    mapposy += rewind_amount;
    if (mapposy > max_mapposy)
        mapposy = max_mapposy;

    mapposy = (mapposy / EFFECTIVE_HEIGHT) * EFFECTIVE_HEIGHT;
    videoposy = 0;
    
    // === STEP 4: Refill ALL three buffers with new map position ===
    Game_FillScreen();
    WaitBlit();
    Road_CacheFillVisible();

    WORD bike_wy = mapposy + (SCREENHEIGHT - 48);
    WORD safe_x = 80;
    
    WORD road_start = -1;
    WORD road_end = -1;
    
    for (WORD scan = 0; scan < 192; scan += 8)
    {
        UBYTE lane = Collision_GetLane(scan, bike_wy);
        if (lane >= LANE_1 && lane <= LANE_4)
        {
            if (road_start < 0) road_start = scan;
            road_end = scan;
        }
        else if (lane == LANE_OFFROAD && road_start >= 0)
        {
            break;
        }
    }
    
    if (road_start >= 0)
        safe_x = (road_start + road_end) / 2;
    
    bike_position_x = safe_x;
    bike_position_y = SCREENHEIGHT - 48;
    bike_world_y = mapposy + bike_position_y;
    bike_speed = MIN_CRUISING_SPEED;
    bike_state = BIKE_STATE_MOVING;
  

    // === STEP 5: Update copper for new scroll position ===
    const UBYTE* planes_temp[BLOCKSDEPTH];
    planes_temp[0] = display_buffer;
    planes_temp[1] = display_buffer + BITMAPBYTESPERROW;
    planes_temp[2] = display_buffer + BITMAPBYTESPERROW * 2;
    planes_temp[3] = display_buffer + BITMAPBYTESPERROW * 3;
    
    WORD yoffset = (videoposy + BLOCKHEIGHT);
    if (yoffset >= BITMAPHEIGHT) yoffset -= BITMAPHEIGHT;
    LONG planeadd = ((LONG)yoffset) * BITMAPBYTESPERROW * BLOCKSDEPTH;
    
    Copper_SetBitplanePointer(BLOCKSDEPTH, planes_temp, planeadd);
   
    Game_ApplyPalette(current_palette, BLOCKSCOLORS);
   
    WaitVBL();
     
    bike_position_x = safe_x;
    bike_position_y = SCREENHEIGHT - 48;
    bike_world_y = mapposy + bike_position_y;
    bike_speed = MIN_CRUISING_SPEED;
    bike_state = BIKE_STATE_MOVING;
    
    // 2 seconds of invulnerability
    bike_invulnerable = TRUE;
    Timer_Start(&invuln_timer, 2);
    Sprites_ClearHigher();
    MotorBike_SetFrame(BIKE_FRAME_MOVING1);
}
 
void MotorBike_ResetApproachIndex(void)
{
    approach_frame_index = 1;
    vibration_counter = 0;
}