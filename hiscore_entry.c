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
#include <string.h>
#include "game.h"
#include "map.h"
#include "timers.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "motorbike.h"
#include "hud.h"
#include "title.h"
#include "hiscore_entry.h"
#include "hiscore.h"
#include "font.h"
 

static NameEntryState entry_state;
static GameTimer cursor_blink_timer;
static BOOL cursor_visible = TRUE;

void NameEntry_Init(UBYTE position)
{
    entry_state.name[0] = 'A';
    entry_state.name[1] = 'A';
    entry_state.name[2] = 'A';
    entry_state.name[3] = '\0';
    entry_state.cursor_pos = 0;
    entry_state.active = TRUE;
    entry_state.table_position = position;
    
    Timer_StartMs(&cursor_blink_timer, 250);
}

void NameEntry_Update(void)
{
    if (!entry_state.active) return;
    
    // Blink cursor
    if (Timer_HasElapsed(&cursor_blink_timer))
    {
        cursor_visible = !cursor_visible;
        Timer_Reset(&cursor_blink_timer);
    }
}

void NameEntry_Draw(UBYTE *buffer)
{
    if (!entry_state.active) return;
    
    char line_buffer[32];
    UWORD y = 10;
    UBYTE normal_color = 12;
    UBYTE entry_color = 2;
    
    WaitBlit();
    Font_DrawStringCentered(buffer, " BEST 10 PLAYERS", y, normal_color);
    y += 20;
    
    WaitBlit();
    Font_DrawString(buffer, "NO", COL_NO, y, normal_color);
    Font_DrawString(buffer, "SCORE", COL_SCORE, y, normal_color);
    Font_DrawString(buffer, "RANK", COL_RANK, y, normal_color);
    Font_DrawString(buffer, "NAME", COL_NAME, y, normal_color);
    y += 16;
    
    WaitBlit();
    
    HiScoreTable *table = HiScore_GetTable();
    UBYTE insert_idx = entry_state.table_position - 1;
    
    for (int i = 0; i < MAX_HISCORE_ENTRIES; i++)
    {
        BOOL is_entry_row = (i == insert_idx);
        UBYTE color = is_entry_row ? entry_color : normal_color;
        
        ULongToString(i + 1, line_buffer, 2, ' ');
        Font_DrawString(buffer, line_buffer, COL_NO, y, color);
        
        if (is_entry_row)
        {
            /* Draw player's pending entry */
            ULongToString(game_score, line_buffer, 6, ' ');
            Font_DrawString(buffer, line_buffer, COL_SCORE, y, color);
            
            char rank_str[8];
            HiScore_FormatRank(game_best_rank, rank_str);
            Font_DrawString(buffer, rank_str, COL_RANK, y, color);
            
            for (int c = 0; c < 3; c++)
            {
                WORD char_x = COL_NAME + (c << 3);
                Font_ClearArea(buffer, char_x, y, 8, 8);
                char ch[2] = { entry_state.name[c], '\0' };
                
                if (c == entry_state.cursor_pos && !cursor_visible)
                    Font_DrawString(buffer, ch, char_x, y, 0);
                else
                    Font_DrawString(buffer, ch, char_x, y, entry_color);
            }
        }
        else
        {
            /* Rows below insertion read one entry higher (shifted down) */
            UBYTE table_idx = (i >= insert_idx) ? i - 1 : i;
            
            /* Don't read past table end */
            if (table_idx >= MAX_HISCORE_ENTRIES)
            {
                y += 15;
                continue;
            }
            
            HiScoreEntry *entry = &table->entries[table_idx];
            
            ULongToString(entry->score, line_buffer, 6, ' ');
            Font_DrawString(buffer, line_buffer, COL_SCORE, y, color);
            
            char rank_str[8];
            HiScore_FormatRank(entry->rank, rank_str);
            Font_DrawString(buffer, rank_str, COL_RANK, y, color);
            
            Font_DrawString(buffer, entry->name, COL_NAME, y, color);
        }
        
        y += 15;
    }
}

void NameEntry_HandleInput(UBYTE joystick, BOOL fire)
{
    if (!entry_state.active) return;
    
    static UBYTE prev_joy = 0;
    static BOOL prev_fire = FALSE;
    
    /* Up — next letter */
    if ((joystick & 0x01) && !(prev_joy & 0x01))
    {
        entry_state.name[entry_state.cursor_pos]++;
        if (entry_state.name[entry_state.cursor_pos] > 'Z')
            entry_state.name[entry_state.cursor_pos] = 'A';
    }
    
    /* Down — previous letter */
    if ((joystick & 0x02) && !(prev_joy & 0x02))
    {
        entry_state.name[entry_state.cursor_pos]--;
        if (entry_state.name[entry_state.cursor_pos] < 'A')
            entry_state.name[entry_state.cursor_pos] = 'Z';
    }
    
    /* Left — go back to previous character */
    if ((joystick & 0x04) && !(prev_joy & 0x04))
    {
        if (entry_state.cursor_pos > 0)
            entry_state.cursor_pos--;
    }
    
    /* Right or Fire — advance to next character or finish */
    BOOL advance = fire && !prev_fire;
    if ((joystick & 0x08) && !(prev_joy & 0x08))
        advance = TRUE;
    
    if (advance)
    {
        entry_state.cursor_pos++;
        if (entry_state.cursor_pos >= 3)
        {
            entry_state.active = FALSE;
        }
    }
    
    prev_joy = joystick;
    prev_fire = fire;
}

BOOL NameEntry_IsComplete(void)
{
    return !entry_state.active;
}

const char* NameEntry_GetName(void)
{
    return entry_state.name;
}