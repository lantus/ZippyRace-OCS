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
#include <dos/dosextens.h>
#include <workbench/startup.h>

#include "game.h"
#include "map.h"
#include "roadsystem.h"
#include "hardware.h"
#include "bitmap.h"
#include "blitter.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "motorbike.h"
#include "hud.h"
#include "cars.h"
#include "hardware.h"
#include "title.h"
#include "timers.h"
#include "audio.h"

volatile struct Custom *custom;

enum
{
	REG = 0,
	VALUE = 1
};

// Global variables
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
 
struct GfxBase *GfxBase = NULL;
struct ViewPort *viewPort = NULL;
struct RasInfo rasInfo;
 
static UWORD SystemInts;
static UWORD SystemDMA;
static UWORD SystemADKCON;
volatile APTR VBR = 0;
APTR saved_vectors[7]; 
static APTR SystemIrq;
struct View *ActiView;

static struct WBStartup *wb_msg = NULL;
static BPTR progdir = 0;
static BPTR prevdir = 0;
static BOOL from_cli = FALSE;
 
/* write definitions for dmaconw */
#define DMAF_SETCLR  0x8000
#define DMAF_AUDIO   0x000F   /* 4 bit mask */
#define DMAF_AUD0    0x0001
#define DMAF_AUD1    0x0002
#define DMAF_AUD2    0x0004
#define DMAF_AUD3    0x0008
#define DMAF_DISK    0x0010
#define DMAF_SPRITE  0x0020
#define DMAF_BLITTER 0x0040
#define DMAF_COPPER  0x0080
#define DMAF_RASTER  0x0100
#define DMAF_MASTER  0x0200
#define DMAF_BLITHOG 0x0400
#define DMAF_ALL     0x01FF   /* all dma channels */

#define BPLCON3_BRDNBLNK (1<<5)

const UBYTE* planes[BLOCKSDEPTH];
 
WORD	bitmapheight;

LONG MapSize = 0;

BOOL	option_ntsc,option_how,option_speed;
WORD	option_fetchmode;
UBYTE *hud_buffer = NULL;
BPTR	MapHandle = 0;
char	s[256];
struct RawMap *Map;
struct FetchInfo
{
	WORD	start;
	WORD	stop;
	WORD	modulooffset;
} fetchinfo [] =
{
	{0x38,0xD0,0},	/* normal         */	
	{0x38,0xC8,0}, 	/* BPL32          */
	{0x38,0xC8,0}, 	/* BPAGEM         */
	{0x38,0xB8,0}	/* BPL32 + BPAGEM */
};

// Global variables
 
struct Interrupt vblankInt;
BOOL running = TRUE;
 
__attribute__((always_inline)) inline short MouseLeft() { return !((*(volatile UBYTE*)0xbfe001) & 64); }
__attribute__((always_inline)) inline short MouseRight() { return !((*(volatile UWORD*)0xdff016) & (1 << 10)); }
 

static void InitCopperlist(void)
{
	WORD	*wp;
	LONG l;
 
	custom->dmacon = 0x7FFF;
 
	// plane pointers
 
	for(int i=0; i < 16; i++)
	{
		Copper_SetPalette(i, ((USHORT*)intro_colors)[i]);
	}

	CopFETCHMODE[VALUE] = 0;

	CopBPLCON0[VALUE] = 0x4200;			 
	CopBPLCON1[VALUE] = 0;
	CopBPLCON3[VALUE] = (1<<6);

	// bitplane modulos
	l = (BITMAPBYTESPERROW * BLOCKSDEPTH) - (24 );

	CopBPLMODA[VALUE] = l;
	CopBPLMODB[VALUE] = l;
	
	// display window start/stop
	
	if (g_is_pal == TRUE)
	{
		CopDIWSTART[VALUE] = DIWSTARTVAL;
		CopDIWSTOP[VALUE] = DIWSTOPVAL;
	}
	else
	{
      	CopDIWSTART[VALUE] = DIWSTARTVAL_NTSC;
        CopDIWSTOP[VALUE] = DIWSTOPVAL_NTSC;
	}
	
	// display data fetch start/stop
	
	CopDDFSTART[VALUE] = 0x48;
    CopDDFSTOP[VALUE] = 0xA0;   
	
	// plane pointers
	 
	const USHORT lineSize=SCREENWIDTH/8;
	 
	for(int a=0;a<BLOCKSDEPTH;a++)
		planes[a]=(UBYTE*)screen.bitplanes + lineSize * a;

	Copper_SetBitplanePointer(BLOCKSDEPTH,planes , 0);

	custom->intena = 0x7FFF;
 	
	custom->dmacon = DMAF_SETCLR | DMAF_BLITTER | DMAF_COPPER | DMAF_RASTER | DMAF_MASTER | DMAF_SPRITE | DMAF_AUD0 | DMAF_AUD1 | DMAF_AUD2 | DMAF_AUD3;
	custom->cop2lc = (ULONG)CopperList;	
	custom->copjmp2 = 0;  /* Strobe copper to start */
 
};

 
void Cleanup(char *msg)
{
	WORD rc;
	
	if (msg)
	{
		KPrintF("Error: %s\n",msg);
		rc = RETURN_WARN;
	} 
	else 
	{
		rc = RETURN_OK;
	}
	 
	if (ScreenBitmap)
	{
		WaitBlit();
		BitMapEx_Destroy(ScreenBitmap);
	}

	if (Map) FreeMem(Map,MapSize);
	if (MapHandle) Close(MapHandle);

	
	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);
	 
	Exit(rc);
}

static void OpenDisplay(void)
{
	bitmapheight = BITMAPHEIGHT + 3;

	Screen_Initialize_DoubleBuff(BITMAPWIDTH,bitmapheight,BLOCKSDEPTH, FALSE);
}

void WaitLine(USHORT line) 
{
	while (1) 
    {
		volatile ULONG vpos=*(volatile ULONG*)0xDFF004;
		if(((vpos >> 8) & 511) == line)
			break;
	}
}
APTR GetVBR(void) 
{
    APTR vbr = 0;
    UWORD getvbr[] = { 0x4e7a, 0x0801, 0x4e73 }; // MOVEC.L VBR,D0 RTE
    if (SysBase->AttnFlags & AFF_68010)
        vbr = (APTR)Supervisor((ULONG (*)())getvbr);
    return vbr;
}

void SetInterruptHandler(APTR interrupt) 
{
    *(volatile APTR*)(((UBYTE*)VBR) + 0x6c) = interrupt;
}

void SetInterruptHandlerLevel4(APTR interrupt) {
	*(volatile APTR*)(((UBYTE*)VBR)+0x6c + 4) = interrupt;
}

APTR GetInterruptHandler() {
    return *(volatile APTR*)(((UBYTE*)VBR) + 0x6c);
}

void WaitVbl() 
{
	volatile UWORD *vposr = (volatile UWORD*)0xDFF004;
    
    // Wait for LOF bit to be SET (bit 0 = 1)
    while ((*vposr & 1) == 0) {
        // Loop until bit 0 goes high
    }
    
    // Wait for LOF bit to be CLEAR (bit 0 = 0)
    while ((*vposr & 1) != 0) {
        // Loop until bit 0 goes low
    }
}
 
void FreeSystem()
{ 
	WaitVbl();
	WaitBlit();
    
	custom->intena=0x7fff;//disable all interrupts
	custom->intreq=0x7fff;//Clear any interrupts that were pending
	custom->dmacon=0x7fff;//Clear all DMA channels

	//restore interrupts
	SetInterruptHandler(SystemIrq);

	/*Restore system copper list(s). */
	custom->cop1lc=(ULONG)GfxBase->copinit;
	custom->cop2lc=(ULONG)GfxBase->LOFlist;
	custom->copjmp1=0x7fff; //start coppper

	/*Restore all interrupts and DMA settings. */
	custom->intena=SystemInts|0x8000;
	custom->dmacon=SystemDMA|0x8000;
	custom->adkcon=SystemADKCON|0x8000;

	WaitBlit();	
	DisownBlitter();
	Enable();

	/* Re-enable caches */
    if (SysBase->AttnFlags & AFF_68020)
    {
        CacheControl(CACRF_EnableI | CACRF_EnableD, 
                     CACRF_EnableI | CACRF_EnableD);
    }

	LoadView(ActiView);
	WaitTOF();
	WaitTOF();

	Permit();
} 
 
// Add these constants at the top with your other defines
#define GAMEAREA_WIDTH 64          // 24 blocks × 8 pixels
#define GAMEAREA_BLOCKS 28          // blocks across the game area
 
 
__attribute__((interrupt)) void interruptHandler() 
{
	custom->intreq=(1<<INTB_VERTB); custom->intreq=(1<<INTB_VERTB); //reset vbl req. twice for a4000 bug.

	Timer_VBlankUpdate();
 
}
static BOOL paused = FALSE;
static UBYTE prev_key = 0;
 
 
int main(void)
{
	LONG x = 0;
    SysBase = *((struct ExecBase**)4UL);
	custom = (struct Custom*)0xdff000;
 
	Startup_Init();

	// We will use the graphics library only to locate and restore the system copper list once we are through.

	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library",0);
	if (!GfxBase)
		Exit(0);

    // used for printing
	DOSBase = (struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library", 0);
	if (!DOSBase)
		Exit(0);
 
	option_how = FALSE;
   	option_speed = TRUE;
 	option_fetchmode = 0;
	
	WaitVbl();
    Delay(10);

	Startup_SetProgramDir();

    if (!wb_msg)
    {
        Write(Output(), (APTR)"\n", 2);
        Write(Output(), (APTR)"Starting up...\n", 16);
    }
 
	Game_Initialize();
 
	OpenDisplay();
 
	volatile UBYTE *ciabprb = (volatile UBYTE *)0xBFD100;
    *ciabprb = 0xF7;   /* 1111 0111 — motor off, select DF0 */
    *ciabprb = 0xFF;   /* 1111 1111 — all deselected */
 
	KillSystem();
 
	InitCopperlist();
 
	Copper_SetPalette(0, 0x003);
 
	// Enable VBlank first
	custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB
#ifdef WHDLOAD
	               | INTF_PORTS		/* let WHDLoad's keyboard hook breathe */
#endif
	;
	custom->intreq = (1<<INTB_VERTB);  // Clear pending
	
	SetInterruptHandler((APTR)interruptHandler);
	Audio_InstallPlayer();
  
	HUD_DrawAll();
 
	while(1) 
    {		 
		WaitLine(0x13);

#ifndef WHDLOAD
		KeyRead();
#endif
		 
		Joy_ReadAll();

		Game_Update();
		Game_Draw();

		// Flip Buffers
		if (stage_state == STAGE_PLAYING || 
			stage_state == STAGE_FRONTVIEW ||
			stage_state == STAGE_COUNTDOWN ||
			stage_state == STAGE_RANKING ||
		 	title_state == TITLE_ATTRACT_LOGO_DROP)
		{
			current_buffer = 1 - current_buffer;
		}

		//custom->color[0] = 0x00F;  // BLUE = game work done, waiting

		// Swap pointers
		draw_buffer = current_buffer == 0 ? screen.bitplanes : screen.offscreen_bitplanes;
		display_buffer = current_buffer == 0 ? screen.offscreen_bitplanes : screen.bitplanes;
	}

    ActivateSystem();

	Cleanup("");

	CloseLibrary((struct Library*)DOSBase);
	CloseLibrary((struct Library*)GfxBase);

    return 0;
}

void Startup_Init(void)
{
    struct Process *proc = (struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli = (struct CommandLineInterface *)(proc->pr_CLI * 4);

    if (!cli)
    {
        /* Workbench launch */
        WaitPort(&proc->pr_MsgPort);
        wb_msg = (struct WBStartup *)GetMsg(&proc->pr_MsgPort);
        progdir = wb_msg->sm_ArgList[0].wa_Lock;
    }
    else
    {
        from_cli = TRUE;
    }
}

void Startup_SetProgramDir(void)
{
    if (from_cli)
    {
        struct Process *proc = (struct Process *)FindTask(NULL);
        struct CommandLineInterface *cli = (struct CommandLineInterface *)(proc->pr_CLI * 4);

        /* BCPL string: first byte = length */
        unsigned char *p = (unsigned char *)(cli->cli_CommandName * 4);
        char cmd[200];
        memcpy(cmd, p + 1, p[0]);
        cmd[p[0]] = 0;

        BPTR prglock = Lock(cmd, SHARED_LOCK);
        progdir = ParentDir(prglock);
        UnLock(prglock);
    }

    prevdir = CurrentDir(progdir);
}

void Startup_Cleanup(void)
{
    CurrentDir(prevdir);

    if (from_cli)
        UnLock(progdir);

    if (wb_msg)
    {
        Forbid();
        ReplyMsg((struct Message *)wb_msg);
    }
}