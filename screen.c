#include "support/gcc8_c_support.h"
#include "screen.h"
#include "bitplanes.h"
#include "bitmap.h"
#include "blitter.h"
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>

extern struct Custom custom;
 
struct AmigaScreen screen;
struct AmigaScreen off_screen;
 
UBYTE current_buffer = 0;
UBYTE *draw_buffer;
UBYTE *display_buffer; 

void Screen_Initialize(UWORD width,
    UWORD   height,
    UBYTE   depth,
    BOOL    dual_playfield) 
{
    
    screen.framebuffer_size = (width/8)*height*depth;

    screen.bitplanes = Bitplanes_Initialize(screen.framebuffer_size);
    screen.depth = depth;
    screen.left_margin_bytes = 0;
    screen.right_margin_bytes = 0;
    screen.display_start_offset_bytes = 0;
    screen.bytes_width = width/8;
    screen.row_size = (width/8);
    screen.dual_playfield = dual_playfield;
    screen.height = height;
 
}

void Screen_Initialize_DoubleBuff(UWORD width,
    UWORD   height,
    UBYTE   depth,
    BOOL    dual_playfield) 
{
    
    screen.framebuffer_size = (width/8)*height*depth;

    screen.bitplanes = Bitplanes_Initialize(screen.framebuffer_size);
    screen.offscreen_bitplanes = Bitplanes_Initialize(screen.framebuffer_size);
    screen.pristine = Bitplanes_Initialize(screen.framebuffer_size);
    screen.depth = depth;
    screen.left_margin_bytes = 0;
    screen.right_margin_bytes = 0;
    screen.display_start_offset_bytes = 0;
    screen.bytes_width = width/8;
    screen.row_size = (width/8);
    screen.dual_playfield = dual_playfield;
    screen.height = height;
 
    BlitClearScreen(screen.bitplanes, width << 6 | 16 );
    BlitClearScreen(screen.offscreen_bitplanes, width << 6 | 16 );

    draw_buffer = screen.bitplanes;
    display_buffer = screen.offscreen_bitplanes;
  
    //debug_register_bitmap(screen.bitplanes, "screen.bitplanes", width, height, 4, 1 << 0);
    //debug_register_bitmap(screen.offscreen_bitplanes, "screen.offscreen_bitplanes", width, height, 4, 1 << 0);
    //debug_register_bitmap(screen.pristine, "screen.pristine", width, height, 4, 1 << 0);

}
 
static UWORD getFadedValueOCS(UWORD colorvalue, USHORT frame) 
{
    short output = 0;

    short red = (colorvalue & 0x0F00);
    red *= frame;
    red >>= 4;
    red &= 0x0F00;
    output |= red;

    short green = (colorvalue & 0x00F0);
    green *= frame;
    green >>= 4;
    green &= 0x00F0;
    output |= green;

    short blue = (colorvalue & 0x000F);
    blue *= frame;
    blue >>= 4;
    blue &= 0x000F;
    output |= blue;

    return output;
}
 
void Screen_FadePalette(UWORD* rawPalette,UWORD* copPalette, USHORT frame,USHORT colors) 
{
    for (int i = 0; i < colors; i++) 
    {
        *copPalette = getFadedValueOCS(*rawPalette,frame);
        rawPalette++;
        copPalette+=2;
    }
}
 