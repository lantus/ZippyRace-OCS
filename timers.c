#include "timers.h"
#include "hardware.h"
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

// Private globals
static volatile ULONG timer_vblank_ticks = 0;
static UWORD timer_refresh_rate = 50;  // Hz (PAL or NTSC)

// Global display offsets (set once at init)
BOOL  g_is_pal = TRUE;
UWORD g_sprite_hoffset = 129 + 32;  // Horizontal offset (usually same for both)
UWORD g_sprite_voffset = 44;   // Vertical offset (differs PAL/NTSC)
UWORD g_screen_height = 256;   // Screen height

// Detect PAL/NTSC by counting scanlines
void Timer_Init(void)
{
    struct ExecBase *SysBase = *(struct ExecBase **)4L;
    
    if (SysBase->VBlankFrequency == 50) 
    {
        // PAL system
        g_sprite_voffset = 44;   // or 0x2C
        g_screen_height = 256;

        timer_refresh_rate = 50;
    } else {
        // NTSC system
        g_sprite_voffset = 44;   // Same, but shorter display
        g_screen_height = 200;

        timer_refresh_rate = 60;
    }

    timer_vblank_ticks = 0;
 
    KPrintF("Timer: Detected %s (%ld Hz)\n", 
            timer_refresh_rate == 60 ? "NTSC" : "PAL",
            timer_refresh_rate);

    
    
    Write(Output(), (APTR)"Loading Zippy Race V1.1\n", 25);
    Write(Output(), (APTR)" \n", 3);
    
    g_is_pal = timer_refresh_rate == 60 ? FALSE : TRUE;     
    
    if (g_is_pal == TRUE)
    {
        Write(Output(), (APTR)"50hz PAL Detected\n", 19);
    }
    else
    {
        Write(Output(), (APTR)"60hz NTSC Detected\n", 20);
    }
    Write(Output(), (APTR)" \n", 3);
}

// Call this in your VBlank interrupt handler
void Timer_VBlankUpdate(void)
{
    timer_vblank_ticks++;
}

// Get current tick count
ULONG Timer_GetTicks(void)
{
    return timer_vblank_ticks;
}

// Get refresh rate
UWORD Timer_GetRefreshRate(void)
{
    return timer_refresh_rate;
}

// Start timer with seconds
void Timer_Start(GameTimer *timer, UWORD seconds)
{
    timer->start_tick = timer_vblank_ticks;
    timer->duration_ticks = seconds * timer_refresh_rate;
    timer->active = TRUE;
    timer->paused = FALSE;
}

// Start timer with milliseconds
void Timer_StartMs(GameTimer *timer, UWORD milliseconds)
{
    timer->start_tick = timer_vblank_ticks;
    timer->duration_ticks = (milliseconds * timer_refresh_rate) / 1000;
    if (timer->duration_ticks == 0) timer->duration_ticks = 1;  // Minimum 1 frame
    timer->active = TRUE;
    timer->paused = FALSE;
}

// Start timer with exact frame count
void Timer_StartFrames(GameTimer *timer, UWORD frames)
{
    timer->start_tick = timer_vblank_ticks;
    timer->duration_ticks = frames;
    timer->active = TRUE;
    timer->paused = FALSE;
}

// Stop timer
void Timer_Stop(GameTimer *timer)
{
    timer->active = FALSE;
    timer->paused = FALSE;
}

// Reset timer (restart from beginning)
void Timer_Reset(GameTimer *timer)
{
    if (timer->active)
    {
        timer->start_tick = timer_vblank_ticks;
        timer->paused = FALSE;
    }
}

// Pause timer
void Timer_Pause(GameTimer *timer)
{
    if (timer->active && !timer->paused)
    {
        timer->pause_tick = timer_vblank_ticks;
        timer->paused = TRUE;
    }
}

// Resume paused timer
void Timer_Resume(GameTimer *timer)
{
    if (timer->active && timer->paused)
    {
        ULONG pause_duration = timer_vblank_ticks - timer->pause_tick;
        timer->start_tick += pause_duration;  // Adjust start time
        timer->paused = FALSE;
    }
}

// Check if timer has elapsed
BOOL Timer_HasElapsed(GameTimer *timer)
{
    if (!timer->active || timer->paused) return FALSE;
    
    ULONG elapsed = timer_vblank_ticks - timer->start_tick;
    return (elapsed >= timer->duration_ticks);
}

// Get elapsed time in milliseconds
ULONG Timer_GetElapsedMs(GameTimer *timer)
{
    if (!timer->active) return 0;
    
    ULONG current_tick = timer->paused ? timer->pause_tick : timer_vblank_ticks;
    ULONG elapsed_ticks = current_tick - timer->start_tick;
    
    return (elapsed_ticks * 1000) / timer_refresh_rate;
}

// Get elapsed time in seconds
ULONG Timer_GetElapsedSeconds(GameTimer *timer)
{
    if (!timer->active) return 0;
    
    ULONG current_tick = timer->paused ? timer->pause_tick : timer_vblank_ticks;
    ULONG elapsed_ticks = current_tick - timer->start_tick;
    
    return elapsed_ticks / timer_refresh_rate;
}

// Get elapsed frames
ULONG Timer_GetElapsedFrames(GameTimer *timer)
{
    if (!timer->active) return 0;
    
    ULONG current_tick = timer->paused ? timer->pause_tick : timer_vblank_ticks;
    return current_tick - timer->start_tick;
}

// Get remaining time in milliseconds
ULONG Timer_GetRemainingMs(GameTimer *timer)
{
    if (!timer->active) return 0;
    
    ULONG current_tick = timer->paused ? timer->pause_tick : timer_vblank_ticks;
    ULONG elapsed_ticks = current_tick - timer->start_tick;
    
    if (elapsed_ticks >= timer->duration_ticks) return 0;
    
    ULONG remaining_ticks = timer->duration_ticks - elapsed_ticks;
    return (remaining_ticks * 1000) / timer_refresh_rate;
}

// Get remaining time in seconds
ULONG Timer_GetRemainingSeconds(GameTimer *timer)
{
    if (!timer->active) return 0;
    
    ULONG current_tick = timer->paused ? timer->pause_tick : timer_vblank_ticks;
    ULONG elapsed_ticks = current_tick - timer->start_tick;
    
    if (elapsed_ticks >= timer->duration_ticks) return 0;
    
    ULONG remaining_ticks = timer->duration_ticks - elapsed_ticks;
    return remaining_ticks / timer_refresh_rate;
}

// Check if timer is active
BOOL Timer_IsActive(GameTimer *timer)
{
    return timer->active;
}

// Check if timer is paused
BOOL Timer_IsPaused(GameTimer *timer)
{
    return timer->paused;
}

// Blocking delay (frames)
void Timer_DelayFrames(UWORD frames)
{
    ULONG start = timer_vblank_ticks;
    while ((timer_vblank_ticks - start) < frames)
    {
        // Wait
    }
}

// Blocking delay (milliseconds)
void Timer_DelayMs(UWORD milliseconds)
{
    UWORD frames = (milliseconds * timer_refresh_rate) / 1000;
    if (frames == 0) frames = 1;
    Timer_DelayFrames(frames);
}