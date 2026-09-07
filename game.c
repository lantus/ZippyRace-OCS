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
#include "pak.h"
#include "timers.h"
#include "hardware.h"
#include "bitmap.h"
#include "copper.h"
#include "pixel.h"
#include "sprites.h"
#include "tilesheet.h"
#include "motorbike.h"
#include "planes.h"
#include "hud.h"
#include "font.h"
#include "title.h"
#include "fuel.h"
#include "hiscore.h"
#include "hiscore_entry.h"
#include "blitter.h"
#include "city_approach.h"
#include "roadsystem.h"
#include "ranking.h"
#include "stageprogress.h"
#include "fuel.h"
#include "barreltruck.h"

#include "cars.h"
#include "audio.h"

extern volatile struct Custom *custom;
extern BlitterObject nyc_horizon;
extern BlitterObject lv_horizon;
extern BlitterObject houston_horizon;
extern BlitterObject *city_horizon;


// Precomputed scroll amounts for speeds 0-255 (8.8 fixed point)
#define MAX_SPEED_TABLE 256
const UWORD scroll_speed_table[MAX_SPEED_TABLE] = {
    0, 12, 24, 36, 48, 60, 73, 85, 97, 109, 121, 133, 146, 158, 170, 182, 194, 207, 219, 231, 243, 256,
    268, 280, 292, 304, 317, 329, 341, 353, 365, 377, 390, 402, 414, 426, 438, 451, 463, 475, 487, 499, 512,
    520, 528, 537, 545, 553, 561, 569, 577, 586, 594, 602, 610, 618, 626, 635, 643, 651, 659, 667, 675, 684,
    692, 700, 708, 716, 724, 733, 741, 749, 757, 765, 773, 782, 790, 798, 806, 814, 822, 831, 839, 847, 855,
    863, 871, 880, 888, 896, 904, 912, 920, 929, 937, 945, 953, 961, 969, 978, 986, 994, 1002, 1010, 1018, 1024,
    1028, 1033, 1038, 1042, 1047, 1052, 1057, 1061, 1066, 1071, 1076, 1080, 1085, 1090, 1095, 1099, 1104, 1109,
    1114, 1118, 1123, 1128, 1133, 1137, 1142, 1147, 1152, 1156, 1161, 1166, 1171, 1175, 1180, 1185, 1190, 1194,
    1199, 1204, 1209, 1213, 1218, 1223, 1228, 1232, 1237, 1242, 1247, 1251, 1256, 1261, 1266, 1270, 1275, 1280,
    1285, 1289, 1294, 1299, 1304, 1308, 1313, 1318, 1323, 1327, 1332, 1337, 1342, 1346, 1351, 1356, 1361, 1365,
    1370, 1375, 1380, 1384, 1389, 1394, 1399, 1403, 1408, 1413, 1418, 1422, 1427, 1432, 1437, 1441, 1446, 1451,
    1456, 1460, 1465, 1470, 1475, 1479, 1484, 1489, 1494, 1498, 1503, 1508, 1513, 1517, 1522, 1527, 1532, 1536,
    1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
    1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536, 1536,
    1536, 1536, 1536, 1536, 1536, 1536
};

// PAL (50fps) — each NTSC value * 1.2 to match game feel
const UWORD scroll_speed_table_pal[MAX_SPEED_TABLE] = {
    0, 14, 29, 43, 58, 72, 88, 102, 116, 131, 145, 160, 175, 190, 204, 218, 233, 248, 263, 277, 292, 307,
    322, 336, 350, 365, 380, 395, 409, 424, 438, 452, 468, 482, 497, 511, 526, 541, 556, 570, 584, 599, 614,
    624, 634, 644, 654, 664, 673, 683, 692, 703, 713, 722, 732, 742, 751, 762, 772, 781, 791, 800, 810, 821,
    830, 840, 850, 859, 869, 880, 889, 899, 908, 918, 928, 938, 948, 958, 967, 977, 986, 997, 1007, 1016, 1026,
    1036, 1045, 1056, 1066, 1075, 1085, 1094, 1104, 1115, 1124, 1134, 1144, 1153, 1163, 1174, 1183, 1193, 1202, 1212, 1222, 1229,
    1234, 1240, 1246, 1250, 1256, 1262, 1268, 1273, 1279, 1285, 1291, 1296, 1302, 1308, 1314, 1319, 1325, 1331,
    1337, 1342, 1348, 1354, 1360, 1365, 1370, 1376, 1382, 1387, 1393, 1399, 1405, 1410, 1416, 1422, 1428, 1433,
    1439, 1445, 1451, 1456, 1462, 1468, 1474, 1478, 1484, 1490, 1496, 1501, 1507, 1513, 1519, 1524, 1530, 1536,
    1542, 1547, 1553, 1559, 1565, 1570, 1576, 1582, 1588, 1592, 1598, 1604, 1610, 1616, 1621, 1627, 1633, 1638,
    1644, 1650, 1656, 1661, 1667, 1673, 1679, 1684, 1690, 1696, 1702, 1706, 1712, 1718, 1724, 1729, 1735, 1741,
    1747, 1752, 1758, 1764, 1770, 1775, 1781, 1787, 1793, 1798, 1804, 1810, 1816, 1821, 1827, 1833, 1838, 1843,
    1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843,
    1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843, 1843,
    1843, 1843, 1843, 1843, 1843, 1843
};

// 750cc (×1.25)
const UWORD scroll_speed_750cc[MAX_SPEED_TABLE] = {
    0, 15, 30, 45, 60, 75, 91, 106, 121, 136, 151, 166, 182, 197, 212, 227, 242, 258, 273, 288, 303, 320,
    335, 350, 365, 380, 396, 411, 426, 441, 456, 471, 487, 502, 517, 532, 547, 563, 578, 593, 608, 623, 640,
    650, 660, 671, 681, 691, 701, 711, 721, 732, 742, 752, 762, 772, 782, 793, 803, 813, 823, 833, 843, 855,
    865, 875, 885, 895, 905, 916, 926, 936, 946, 956, 966, 977, 987, 997, 1007, 1017, 1027, 1038, 1048, 1058, 1068,
    1078, 1088, 1100, 1110, 1120, 1130, 1140, 1150, 1161, 1171, 1181, 1191, 1201, 1211, 1222, 1232, 1242, 1252, 1262, 1272, 1280,
    1285, 1291, 1297, 1302, 1308, 1315, 1321, 1326, 1332, 1338, 1345, 1350, 1356, 1362, 1368, 1373, 1380, 1386,
    1392, 1397, 1403, 1410, 1416, 1421, 1427, 1433, 1440, 1445, 1451, 1457, 1463, 1468, 1475, 1481, 1487, 1492,
    1498, 1505, 1511, 1516, 1522, 1528, 1535, 1540, 1546, 1552, 1558, 1563, 1570, 1576, 1582, 1587, 1593, 1600,
    1606, 1611, 1617, 1623, 1630, 1635, 1641, 1647, 1653, 1658, 1665, 1671, 1677, 1682, 1688, 1695, 1701, 1706,
    1712, 1718, 1725, 1730, 1736, 1742, 1748, 1753, 1760, 1766, 1772, 1777, 1783, 1790, 1796, 1801, 1807, 1813,
    1820, 1825, 1831, 1837, 1843, 1848, 1855, 1861, 1867, 1872, 1878, 1885, 1891, 1896, 1902, 1908, 1915, 1920,
    1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920,
    1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920, 1920,
    1920, 1920, 1920, 1920, 1920, 1920
};

// 750cc PAL (×1.25 × 1.2)
const UWORD scroll_speed_750cc_pal[MAX_SPEED_TABLE] = {
    0, 17, 35, 53, 71, 90, 108, 127, 145, 162, 181, 198, 218, 236, 255, 272, 290, 310, 327, 346, 363, 383,
    401, 420, 437, 455, 475, 492, 511, 528, 547, 565, 585, 602, 620, 638, 656, 676, 693, 712, 730, 747, 767,
    780, 791, 805, 817, 828, 841, 852, 865, 878, 890, 902, 915, 926, 938, 952, 963, 976, 987, 1000, 1012, 1025,
    1037, 1050, 1061, 1073, 1085, 1098, 1111, 1122, 1135, 1147, 1158, 1172, 1185, 1196, 1208, 1220, 1232, 1246, 1257, 1270, 1282,
    1293, 1306, 1320, 1331, 1343, 1355, 1367, 1380, 1392, 1405, 1417, 1428, 1441, 1452, 1466, 1478, 1490, 1502, 1515, 1526, 1535,
    1541, 1548, 1556, 1562, 1570, 1577, 1585, 1591, 1598, 1606, 1613, 1620, 1627, 1635, 1642, 1647, 1655, 1662,
    1670, 1676, 1683, 1691, 1698, 1705, 1712, 1720, 1727, 1733, 1741, 1748, 1756, 1762, 1770, 1777, 1785, 1790,
    1797, 1805, 1812, 1818, 1826, 1833, 1841, 1847, 1855, 1862, 1870, 1876, 1883, 1891, 1898, 1905, 1912, 1920,
    1927, 1932, 1940, 1947, 1955, 1961, 1968, 1976, 1983, 1990, 1997, 2005, 2012, 2018, 2026, 2033, 2041, 2047,
    2055, 2062, 2070, 2075, 2082, 2090, 2097, 2103, 2111, 2118, 2126, 2132, 2140, 2147, 2155, 2161, 2168, 2176,
    2183, 2190, 2197, 2205, 2212, 2217, 2225, 2232, 2240, 2246, 2253, 2261, 2268, 2275, 2282, 2290, 2297, 2303,
    2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303,
    2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303, 2303,
    2303, 2303, 2303, 2303, 2303, 2303
};

const UWORD scroll_speed_1200cc[MAX_SPEED_TABLE] = {
    0, 16, 32, 48, 64, 81, 98, 114, 130, 147, 163, 179, 197, 213, 229, 245, 261, 279, 295, 311, 328, 345,
    361, 378, 394, 410, 427, 444, 460, 476, 492, 508, 526, 542, 558, 575, 591, 608, 625, 641, 657, 673, 691, 702,
    712, 724, 735, 746, 757, 768, 778, 791, 801, 812, 823, 834, 845, 857, 868, 878, 889, 900, 911, 923, 934, 945,
    955, 966, 977, 989, 1000, 1011, 1021, 1032, 1043, 1055, 1066, 1077, 1088, 1098, 1109, 1121, 1132, 1143, 1154, 1165, 1175, 1188,
    1198, 1209, 1220, 1231, 1242, 1254, 1264, 1275, 1286, 1297, 1308, 1320, 1331, 1341, 1352, 1363, 1374, 1382, 1387, 1394, 1401, 1406,
    1413, 1420, 1426, 1432, 1439, 1445, 1452, 1458, 1464, 1471, 1478, 1483, 1490, 1497, 1503, 1509, 1516, 1522, 1529, 1534, 1541, 1548,
    1555, 1560, 1567, 1574, 1580, 1586, 1593, 1599, 1606, 1611, 1618, 1625, 1632, 1637, 1644, 1651, 1657, 1663, 1669, 1676, 1683, 1688,
    1695, 1702, 1709, 1714, 1721, 1728, 1734, 1740, 1746, 1753, 1760, 1765, 1772, 1779, 1786, 1791, 1798, 1804, 1811, 1817, 1823, 1830,
    1837, 1842, 1849, 1856, 1863, 1868, 1875, 1881, 1888, 1894, 1900, 1907, 1914, 1919, 1926, 1933, 1939, 1945, 1952, 1958, 1965, 1971,
    1977, 1984, 1991, 1996, 2003, 2010, 2016, 2022, 2029, 2035, 2042, 2047, 2054, 2061, 2068, 2073, 2073, 2073, 2073, 2073, 2073, 2073,
    2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073,
    2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073, 2073
};

// 1200cc PAL (×1.35 × 1.2)
const UWORD scroll_speed_1200cc_pal[MAX_SPEED_TABLE] = {
    0, 19, 38, 58, 77, 97, 118, 137, 157, 176, 196, 215, 236, 255, 275, 294, 314, 335, 354, 374, 393, 414,
    434, 453, 473, 492, 513, 532, 552, 571, 591, 610, 631, 651, 670, 690, 709, 730, 750, 769, 788, 808, 829, 842,
    855, 869, 882, 895, 908, 921, 934, 949, 962, 975, 988, 1001, 1014, 1028, 1041, 1054, 1067, 1080, 1093, 1108, 1121, 1134,
    1146, 1159, 1172, 1187, 1200, 1213, 1226, 1239, 1252, 1266, 1279, 1292, 1305, 1318, 1331, 1346, 1359, 1372, 1385, 1398, 1411, 1425,
    1438, 1451, 1464, 1477, 1490, 1504, 1517, 1530, 1543, 1556, 1569, 1584, 1597, 1610, 1623, 1636, 1649, 1658, 1665, 1673, 1681, 1688,
    1696, 1704, 1712, 1718, 1726, 1735, 1743, 1749, 1757, 1765, 1773, 1780, 1788, 1796, 1804, 1811, 1819, 1827, 1835, 1841, 1850, 1858,
    1866, 1872, 1880, 1888, 1897, 1903, 1911, 1919, 1927, 1934, 1942, 1950, 1958, 1965, 1973, 1981, 1989, 1995, 2003, 2012, 2020, 2026,
    2034, 2042, 2050, 2057, 2065, 2073, 2081, 2088, 2096, 2104, 2112, 2118, 2127, 2135, 2143, 2149, 2157, 2165, 2174, 2180, 2188, 2196,
    2204, 2211, 2219, 2227, 2235, 2242, 2250, 2258, 2266, 2272, 2280, 2289, 2297, 2303, 2311, 2319, 2327, 2334, 2342, 2350, 2358, 2365,
    2373, 2381, 2389, 2395, 2404, 2412, 2420, 2426, 2434, 2442, 2451, 2457, 2465, 2473, 2481, 2488, 2488, 2488, 2488, 2488, 2488, 2488,
    2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488,
    2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488, 2488
};

UBYTE stage_state = STAGE_BEGIN;
UBYTE stage_complete = 0;

GameTimer countdown_timer;
GameTimer hud_update_timer;
GameTimer collision_recovery_timer;

CollisionState collision_state = COLLISION_NONE;
WORD collision_car_index = -1;
UBYTE countdown_value = 5;

UBYTE game_stage = STAGE_LASVEGAS;
UBYTE game_state = TITLE_SCREEN;
UBYTE game_difficulty = FIVEHUNDREDCC;
UBYTE game_map = MAP_ATTRACT_INTRO;
UWORD max_stage_speed;
ULONG game_score;
UBYTE game_rank;
UBYTE game_best_rank; 
ULONG game_frame_count = 0;


UBYTE game_car_block_move_rate = 5;   // Frames between movements (lower = faster)
UBYTE game_car_block_move_speed = 1;  // Pixels per move (higher = faster)
UBYTE game_car_block_x_threshold = 32; // How close bike needs to be horizontally

ULONG speed_accumulator = 0;
ULONG speed_sample_count = 0;

UWORD frontview_bike_frames = 0;

WORD mapposy,videoposy;
LONG mapwidth,mapheight;

UBYTE *frontbuffer,*blocksbuffer;
UWORD *mapdata;
 
struct BitMapEx *BlocksBitmap,*ScreenBitmap;
 
static BikeFrame prev_bike_state = -1;

// Palettes
UWORD	intro_colors[BLOCKSCOLORS];
UWORD	lv_colors[BLOCKSCOLORS];
UWORD	houston_colors[BLOCKSCOLORS];
UWORD	city_colors[BLOCKSCOLORS];
UWORD	offroad_colors[BLOCKSCOLORS];
UWORD	stlouis_colors[BLOCKSCOLORS];
UWORD   black_palette[BLOCKSCOLORS] = {0};
UWORD   palette_fv_stl[BLOCKSCOLORS];
UWORD  *current_palette; 

// Used for the Countdown
static Sprite spr_countdown_timer[4];
ULONG *spr_countdown[4];
ULONG *current_countdown_spr;
UBYTE countdown_idx = 0;

// One time startup for everything

// At top with other static variables
static GameTimer stage_complete_timer;
static WORD ranking_backdrop_y = 0;   

const UWORD *scroll_speed_table_active;

// HUD stuff
static ULONG last_score = 0xFFFFFFFF;
static UBYTE last_rank = 0xFF;
static WORD  last_speed = -1;
static UBYTE hud_phase = 0;

// hiscore/gameover stuff

static GameTimer gameover_timer;
static GameTimer gameover_flash_timer;
static BOOL gameover_text_visible = TRUE;
static UBYTE gameover_hiscore_pos = 0;  /* 0 = doesn't qualify */

// puddle timer
static UBYTE puddle_timer = 0;

// "Empty" text 
static WORD fuel_empty_y = 0;

// Bike repositioning stuff
BOOL bike_invulnerable = FALSE;
 
WORD bike_wy = 0;

UBYTE game_continues = MAX_CONTINUES;
static GameTimer continue_timer;      // Ticks the 9->0 countdown
static UBYTE continue_countdown;      // 9 down to 0

 /* Keep score, fuel, rank — don't reset them */
static UBYTE stage_start_rank;
static ULONG stage_start_score;

void Game_Initialize()
{
    Timer_Init();
    Preloader_Init();

    Sprites_Initialize();
    HiScore_Initialize();
    
    Pak_Open("objects.dat");
    Pak_Open("stages.dat");
    Pak_Open("mus.dat");
    Pak_Open("images.dat");
    Pak_Open("sprites.dat");
    Pak_Open("sfx.dat");
    Pak_Open("palettes.dat");
 
	/* Load ALL assets while OS is alive */

    Preloader_LoadAll();
	MapPool_Initialize();        
    CollisionMap_Initialize();  

    Audio_Initialize();
    MotorBike_Initialize();
    Planes_Initialize();
    NYCVictory_Initialize();
    BarrelTruck_Initialize();
	Cars_Initialize();

    HUD_InitSprites();
    HUD_SetSpritePositions();
    
    /* Initialize tilesheet pool BEFORE Title and Stage */
    TilesheetPool_Initialize();
    
    Title_Initialize();
    City_Initialize();
      
    Stage_Initialize();

    Pak_CloseAll();
    
    Game_SetBackGroundColor(0x125);
  
    game_state = TITLE_SCREEN;
    game_map = MAP_ATTRACT_INTRO;
    game_best_rank = 99;  
     
    /* Load city attract tiles for title screen */
    TilesheetPool_Load(TILEPOOL_CITY_ATTRACT);
    
    Game_SetMap(game_map);
    MotorBike_Reset();
    MotorBike_ResetApproachIndex();
    Fuel_Initialize();
    StageProgress_Initialize();
    
    switch (game_difficulty)
    {
        case FIVEHUNDREDCC:
            scroll_speed_table_active = g_is_pal ? scroll_speed_table_pal : scroll_speed_table;
            break;
        case SEVENFIFTYCC:
            scroll_speed_table_active = g_is_pal ? scroll_speed_750cc_pal : scroll_speed_750cc;
            break;
        case TWELVEHUNDREDCC:
            scroll_speed_table_active = g_is_pal ? scroll_speed_1200cc_pal : scroll_speed_1200cc;
            break;
    }

    prev_bike_state = -1;
    
}

 
void Game_Reset(void)
{
    Transition_ToBlack();

    /* Reset buffer state */
    current_buffer = 0;
    draw_buffer = screen.bitplanes;
    display_buffer = screen.offscreen_bitplanes;
    videoposy = 0;
    mapposy = 0;

    Game_ResetBitplanePointer();

    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);

    Game_ApplyPalette(intro_colors, BLOCKSCOLORS);
    
    title_state = TITLE_ATTRACT_INIT;
    Title_Reset();
    title_state = TITLE_ATTRACT_START;

    Sprites_ClearLower();
    Sprites_ClearHigher();

    /* Reset copper colors back to white */
    Copper_SetScoreRestoreColors();
    HUD_Show1UP();

    /* Reset scroll speed table to default */
    scroll_speed_table_active = g_is_pal ? scroll_speed_table_pal : scroll_speed_table;
 

    /* Reset game state */
    title_state = TITLE_ATTRACT_START;
    game_state = TITLE_SCREEN;
    game_map = MAP_ATTRACT_INTRO;
    game_difficulty = FIVEHUNDREDCC;
    bike_speed = 0;
    stage_state = STAGE_BEGIN;
    fuel_alarm_active = FALSE;

    game_best_rank = 99;  

    TilesheetPool_Load(TILEPOOL_CITY_ATTRACT);
    City_ResetRoadState();
    Game_SetMap(game_map);
    Skyline_Load(SKYLINE_ATTRACT);
    city_horizon = &nyc_horizon;
    
    city_horizon->y = 0;
    city_horizon->old_x = 0;
    city_horizon->old_y = 0;
    city_horizon->prev_old_x = 0;
    city_horizon->prev_old_y = 0;
    city_horizon->needs_restore = FALSE;
    city_horizon->off_screen = FALSE;

    MotorBike_Reset();
    MotorBike_ResetApproachIndex();
    MotorBike_SetDefaultPalette();
    
    Fuel_Initialize();
    StageProgress_Initialize();

    Game_SetBackGroundColor(0x125);
    
    game_frame_count = 0;

    bike_state = BIKE_STATE_STOPPED;
    prev_bike_state = -1;
 
 
    Transition_FromBlack(intro_colors, BLOCKSCOLORS);
}
 

void Game_AdvanceStage(void)
{
    Music_Stop();

    game_stage++;
    
    if (game_stage > STAGE_NEWYORK)
    {
       /* Completed all 5 stages — loop with higher difficulty */
        game_stage = STAGE_LASVEGAS;
        game_rank = 99;  /* Reset rank for new loop */
 
        switch (game_difficulty)
        {
            case FIVEHUNDREDCC:
                game_difficulty = SEVENFIFTYCC;
                break;
            case SEVENFIFTYCC:
                game_difficulty = TWELVEHUNDREDCC;
                break;
            case TWELVEHUNDREDCC:
                /* Already max difficulty — stay here */
                break;
        }
        
        NYCVictory_Stop();
    }
    
    /* More stages — start next overhead */
    Game_StartNextOverhead();
}

void Game_StartNextOverhead(void)
{
    Transition_ToBlack();

    game_state = STAGE_START;
    stage_state = STAGE_BEGIN;
    
    stage_start_rank  = game_rank;
    stage_start_score = game_score;

    /* Keep score, fuel, rank — don't reset them */
    
    collision_state = COLLISION_NONE;
    bike_speed = 0;

     /* ---- Difficulty scaling ---- */
    WORD base_speed;
    WORD max_active_cars;
    
    switch (game_difficulty)
    {
        case FIVEHUNDREDCC:
            base_speed = 210;
            max_active_cars = 2;
            break;
        case SEVENFIFTYCC:
            base_speed = 250;
            max_active_cars = 3;
            break;
        case TWELVEHUNDREDCC:
            base_speed = 300;
            max_active_cars = 4;
            break;
        default:
            base_speed = 210;
            max_active_cars = 2;
            break;
    }
    
   // max_car_count = max_active_cars;  
    
    /* Per-stage settings: speed, tilesheet, map, music */
    UBYTE stage_music;
    WORD start_offset = 0;
    switch (game_stage)
    {
        case STAGE_LASVEGAS:
            max_stage_speed = base_speed;
            TilesheetPool_Load(TILEPOOL_LEVEL1);
            game_map = STAGE1_OVERHEAD;
            stage_music = (game_continues == MAX_CONTINUES) ? MUSIC_START : MUSIC_ONROAD;
            current_palette = city_colors;
            road_tile_plain = 11;
            start_offset = -15;
            break;
        case STAGE_HOUSTON:
            max_stage_speed =   base_speed - 25;
            TilesheetPool_Load(TILEPOOL_LEVEL2);
            game_map = STAGE2_OVERHEAD;
            stage_music = MUSIC_OFFROAD;
            current_palette = offroad_colors;
            road_tile_plain = 0;
         
            break;
        case STAGE_STLOUIS:
            max_stage_speed = base_speed;
            TilesheetPool_Load(TILEPOOL_LEVEL3);
            game_map = STAGE3_OVERHEAD;
            stage_music = MUSIC_ONROAD;
            current_palette = stlouis_colors;
            road_tile_plain = 1;
       
            break;
        case STAGE_CHICAGO:
            max_stage_speed =   base_speed - 25;
            TilesheetPool_Load(TILEPOOL_LEVEL4);
            game_map = STAGE4_OVERHEAD;
            stage_music = MUSIC_OFFROAD;
            current_palette = offroad_colors;
            road_tile_plain = 0;
       
            break;
        case STAGE_NEWYORK:
            max_stage_speed = base_speed;
            TilesheetPool_Load(TILEPOOL_LEVEL5);
            game_map = STAGE5_OVERHEAD;
            stage_music = MUSIC_ONROAD;
            current_palette = city_colors;
            road_tile_plain = 1; 
      
            break;
        default:
            max_stage_speed = base_speed;
            TilesheetPool_Load(TILEPOOL_LEVEL1);
            game_map = STAGE1_OVERHEAD;
            stage_music = (game_continues == MAX_CONTINUES) ? MUSIC_START : MUSIC_ONROAD;
            break;
    }
    
    Game_SetMap(game_map);
 
    bike_position_x = 96;
    bike_position_y = SCREENHEIGHT - (g_is_pal ? 24 : 56);
    bike_state = BIKE_STATE_STOPPED;
    bike_invulnerable = FALSE;
    
    mapposy = (mapheight * BLOCKHEIGHT) - SCREENHEIGHT - BLOCKHEIGHT;
    mapposy = (mapposy / EFFECTIVE_HEIGHT) * EFFECTIVE_HEIGHT;
    mapposy -= start_offset;
    videoposy = mapposy % HALFBITMAPHEIGHT;
    
    stage_progress.mapsize = mapposy;
    speed_accumulator = 0;
    speed_sample_count = 0;
    wheelie_active = FALSE;
    wheelie_scored = FALSE;
    
    crash_anim_frames = 0;
    prev_bike_state = -1;
    
    Game_ApplyPalette(black_palette,BLOCKSCOLORS);
    
    /* Stage 1 spawns 5 cars around bike; other stages use respawn system */
    if (game_stage == STAGE_LASVEGAS)
        Cars_ResetPositions();
    else
        Cars_ResetPositionsEmpty();
 
    Game_ResetBitplanePointer();
    
    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);

     /* Reset draw buffer pointers */
    draw_buffer = screen.bitplanes;
    display_buffer = screen.offscreen_bitplanes;
    
    /* Reset bike speed for attract mode */
    bike_speed = 0;

    
    Game_FillScreen();
    Road_CacheFillVisible();
    Game_SwapBuffers();
    
    if (game_stage == STAGE_LASVEGAS && game_continues == MAX_CONTINUES)
    {
        Stage_ShowInfo();
    }
    
    StageProgress_SetStage(game_stage-1); //UGH
    StageProgress_DrawAll();
    
    MotorBike_Reset();

    HUD_SetSpritePositions();
    HUD_DrawAll();
    HUD_UpdateScore(game_score);
    Fuel_DrawAll();
    
    HUD_UpdateRank(game_rank);
    
    Music_Stop();
    Music_LoadModule(stage_music);
    
    Game_ApplyPalette(current_palette,BLOCKSCOLORS);

    Transition_FromBlack(current_palette, BLOCKSCOLORS);

    KPrintF("=== Starting stage %ld ===\n", game_stage);
}

void Game_NewGame(UBYTE difficulty)
{
    game_stage = STAGE_LASVEGAS;
    game_state = STAGE_START;
    game_map = STAGE1_OVERHEAD;
    collision_state = COLLISION_NONE;

    game_continues = MAX_CONTINUES; 

    game_difficulty = difficulty;
    game_score = 0;
    bike_speed = 0;
    game_rank = 99; // Start in 99th

    stage_start_rank  = 99;      
    stage_start_score = 0;  

    switch (difficulty)
    {
        case FIVEHUNDREDCC:
            max_stage_speed = 210;
            break;
        case SEVENFIFTYCC:
            max_stage_speed = 250;
            break;
        case TWELVEHUNDREDCC:
            max_stage_speed = 300;
            break;
        default:
            max_stage_speed = 210;
            break;
    }
    
    road_tile_plain = 11;

    /* Swap to level 1 tiles */
    TilesheetPool_Load(TILEPOOL_LEVEL1);
    Game_SetMap(game_map);

    // Position bike near bottom of screen
    bike_position_x = 96;
    bike_position_y = SCREENHEIGHT - (g_is_pal ? 24 : 56);  // Near bottom
    bike_state = BIKE_STATE_STOPPED; 
    bike_invulnerable = FALSE;

    mapposy = (mapheight * BLOCKHEIGHT) - SCREENHEIGHT - BLOCKHEIGHT;
    mapposy = (mapposy / EFFECTIVE_HEIGHT) * EFFECTIVE_HEIGHT;
    mapposy -= -15;
    videoposy = mapposy % HALFBITMAPHEIGHT ;

    //videoposy = 0;
    stage_progress.mapsize = mapposy;
 
    speed_accumulator = 0;
    speed_sample_count = 0;
    wheelie_active = FALSE;
    wheelie_scored = FALSE;
    crash_anim_frames = 0;
    prev_bike_state = -1;
    switch (game_difficulty)
    {
        case FIVEHUNDREDCC:
            scroll_speed_table_active = g_is_pal ? scroll_speed_table_pal : scroll_speed_table;
            break;
        case SEVENFIFTYCC:
            scroll_speed_table_active = g_is_pal ? scroll_speed_750cc_pal : scroll_speed_750cc;
            break;
        case TWELVEHUNDREDCC:
            scroll_speed_table_active = g_is_pal ? scroll_speed_1200cc_pal : scroll_speed_1200cc;
            break;
    }

    Fuel_Reset();
    
    Cars_ResetPositions();

    HUD_Init1UPFlash();
}

void Game_Draw()
{
    switch(game_state)
    {
        case TITLE_SCREEN:
            Title_Draw();
            break;
        case GAME_READY:
            GameReady_Draw();
            break;    
        case STAGE_START:
            Stage_Draw();
            break;
    }
}

void Game_Update()
{
    switch(game_state)
    {
        case TITLE_SCREEN:
            Title_Update();
            break;
        case GAME_READY:
            GameReady_Update();
            break;   
        case STAGE_START:
            Stage_Update();
            break;
    }
}

void Game_SetBackGroundColor(UWORD color)
{
    Copper_SetPalette(0, color);
}


void Game_SetMap(UBYTE maptype)
{
    game_map = maptype;
    switch (maptype)
    {
        case MAP_ATTRACT_INTRO:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = intro_colors;
           
            break;
        case STAGE1_OVERHEAD:
            MapPool_Load(STAGE_LASVEGAS);
            current_palette = city_colors;
            CollisionMap_SetStage(STAGE_LASVEGAS);
            break;
        case STAGE1_FRONTVIEW:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = lv_colors;
            break;
        case STAGE2_OVERHEAD:
            MapPool_Load(STAGE_HOUSTON);
            current_palette = offroad_colors;
            CollisionMap_SetStage(STAGE_HOUSTON);
            break;
        case STAGE2_FRONTVIEW:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = houston_colors;
            break;     
        case STAGE3_OVERHEAD:
            MapPool_Load(STAGE_STLOUIS);
            current_palette = stlouis_colors;
            CollisionMap_SetStage(STAGE_STLOUIS);
            break;          
        case STAGE3_FRONTVIEW:
             MapPool_Load(STAGE_ATTRACT);
            current_palette = palette_fv_stl;    
            break;    
        case STAGE4_OVERHEAD:
            MapPool_Load(STAGE_CHICAGO);
            current_palette = offroad_colors;
            CollisionMap_SetStage(STAGE_CHICAGO); 
            break;         
        case STAGE4_FRONTVIEW:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = palette_fv_stl;    
            break;      
        case STAGE5_OVERHEAD:
            MapPool_Load(STAGE_NEWYORK);
            current_palette = city_colors;
            CollisionMap_SetStage(STAGE_NEWYORK);  
            break;         
        case STAGE5_FRONTVIEW:
            MapPool_Load(STAGE_ATTRACT);
            current_palette = palette_fv_stl;    
            break;                       
    }

   
}
 
__attribute__((always_inline)) WORD GetScrollAmount(WORD speed)
{
    if (speed >= MAX_SPEED_TABLE)
        return scroll_speed_table_active[MAX_SPEED_TABLE - 1];
    
    return scroll_speed_table_active[speed];
}


static void SmoothScroll(void)
{
    WORD scroll_speed = GetScrollAmount(bike_speed);
    
    scroll_accumulator += scroll_speed;
    
    WORD pixels = scroll_accumulator >> 8;
    if (pixels == 0) return;
    scroll_accumulator &= 0xFF;
    
    if (mapposy - pixels < 1)
        pixels = mapposy - 1;
    if (pixels <= 0) return;
    
    WORD old_mapposy = mapposy;
    
    mapposy -= pixels;
   
    videoposy = mapposy;
    while (videoposy >= HALFBITMAPHEIGHT)
        videoposy -= HALFBITMAPHEIGHT;
    
    UWORD drawn_cols = 0;
    
    for (WORD p = 1; p <= pixels; p++)
    {
        WORD pos = old_mapposy - p;
        WORD col = pos & (NUMSTEPS - 1);
        
        if (col >= 12) continue;
        if (drawn_cols & (1 << col)) continue;
        drawn_cols |= (1 << col);
        
        WORD mapy = pos >> 4;
        WORD y = ROUND2BLOCKHEIGHT(pos % EFFECTIVE_HEIGHT) << 2;
        WORD x = col << 4;
        
        DrawBlock(x, y, col, mapy, screen.bitplanes);
        DrawBlock(x, y, col, mapy, screen.offscreen_bitplanes);
        DrawBlock(x, y, col, mapy, screen.pristine);
 
        UWORD yoff = y + (HALFBITMAPHEIGHT << 2);
        DrawBlock(x, yoff, col, mapy, screen.bitplanes);
        DrawBlock(x, yoff, col, mapy, screen.offscreen_bitplanes);
        DrawBlock(x, yoff, col, mapy, screen.pristine);
 
        /* ---- Cache road center for this tile row ---- */
        if (col == 0)
        {
            Road_CacheRow(pos);
        }
    }
}

__attribute__((always_inline)) inline void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy, UBYTE *dest)
{
	x = (x >> 3) & 0xFFFE;
	y = (y << 4) + (y << 3);
	
	UWORD block = mapdata[mapy * mapwidth + mapx];

	mapx = (block % BLOCKSPERROW) * (BLOCKWIDTH >> 3);
	mapy = (block / BLOCKSPERROW) * (BLOCKPLANELINES * BLOCKSBYTESPERROW);
 
	WaitBlit();
	
	custom->bltcon0 = 0x9F0;	// use A and D. Op: D = A
	custom->bltcon1 = 0;
	custom->bltafwm = 0xFFFF;
	custom->bltalwm = 0xFFFF;
	custom->bltamod = BLOCKSBYTESPERROW - (BLOCKWIDTH >> 3);
	custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);
	custom->bltapt  = blocksbuffer + mapy + mapx;
	custom->bltdpt	= dest + y + x;
	
	custom->bltsize = BLOCKPLANELINES * 64 + (BLOCKWIDTH >> 4);
}

__attribute__((always_inline)) inline void DrawBlocksHalf(LONG x, LONG y, LONG mapx, LONG mapy, 
    UWORD blocksperrow, UWORD blockbytessperrow, UWORD blockplanelines, 
    BOOL deltas_only, UBYTE tile_idx, UBYTE *dest)
{
    UWORD halfplanelines = blockplanelines >> 1;  /* 32 = bottom 8 pixels */
    UWORD src_skip = halfplanelines * blockbytessperrow;  /* Skip top 8px in source */
    UWORD dst_skip = halfplanelines * BITMAPBYTESPERROW;  /* Skip top 8px in dest */
    
    x = (x >> 3) & 0xFFFE;
    y = (y << 4) + (y << 3);

    UWORD block = mapdata[mapy * mapwidth + mapx];
  
    mapx = (block % blocksperrow) * (BLOCKWIDTH / 8);
    mapy = (block / blocksperrow) * (blockplanelines * blockbytessperrow);

    WaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = blockbytessperrow - (BLOCKWIDTH >> 3);
    custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);
    custom->bltapt  = blocksbuffer + mapy + mapx + src_skip;   /* Skip top half */
    custom->bltdpt  = dest + y + x + dst_skip;                 /* Draw in bottom half */
    
    custom->bltsize = halfplanelines * 64 + (BLOCKWIDTH >> 4); /* Half height */
}

__attribute__((always_inline)) inline void DrawBlockRunHalf(LONG x, LONG y, UWORD block, WORD count, 
    UWORD blocksperrow, UWORD blockbytesperrow, UWORD blockplanelines, UBYTE *dest)
{
    UWORD halfplanelines = blockplanelines >> 1;
    UWORD src_skip = halfplanelines * blockbytesperrow;
    UWORD dst_skip = halfplanelines * BITMAPBYTESPERROW;
    
    x = (x >> 3) & 0xFFFE;
    y = (y << 4) + (y << 3);

    UWORD mapx = (block & 15) << 1;
    UWORD mapy = (block >> 4) * (blockplanelines * blockbytesperrow);
    
    WaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = blockbytesperrow - (BLOCKWIDTH >> 3);
    custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);
    custom->bltapt  = blocksbuffer + mapy + mapx + src_skip;
    custom->bltdpt  = dest + y + x + dst_skip;
    
    custom->bltsize = (halfplanelines << 6) + (BLOCKWIDTH >> 4);
}
 
__attribute__((always_inline)) inline void DrawBlocks(LONG x,LONG y,LONG mapx,LONG mapy, UWORD blocksperrow, UWORD blockbytessperrow, UWORD blockplanelines, BOOL deltas_only, UBYTE tile_idx, UBYTE *dest)
{
	 
	x = (x >> 3) & 0xFFFE;
	y = (y << 4) + (y << 3);
 

	UWORD block = mapdata[mapy * mapwidth + mapx];
  
	mapx = (block % blocksperrow) * (BLOCKWIDTH / 8);
	mapy = (block / blocksperrow) * (blockplanelines * blockbytessperrow);
 
    WaitBlit();
	
	custom->bltcon0 = 0x9F0;	// use A and D. Op: D = A
	custom->bltcon1 = 0;
	custom->bltafwm = 0xFFFF;
	custom->bltalwm = 0xFFFF;
	custom->bltamod = blockbytessperrow - (BLOCKWIDTH >> 3);
	custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);
	custom->bltapt  = blocksbuffer + mapy + mapx;
	custom->bltdpt	= dest + y + x;
	
	custom->bltsize = blockplanelines * 64 + (BLOCKWIDTH >> 4);
 
}
 
__attribute__((always_inline)) inline void DrawBlockRun(LONG x, LONG y, UWORD block, WORD count, UWORD blocksperrow, UWORD blockbytesperrow, UWORD blockplanelines, UBYTE *dest)
{
   x = (x >> 3) & 0xFFFE;
    y = (y << 4) + (y << 3);  // y * 24

    // Assuming blocksperrow = 16
    UWORD mapx = (block & 15) << 1;                          // block % 16
    UWORD mapy = (block >> 4) * (blockplanelines * blockbytesperrow);  // block / 16
    
    
    WaitBlit();
    
    custom->bltcon0 = 0x9F0;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xFFFF;
    custom->bltalwm = 0xFFFF;
    custom->bltamod = blockbytesperrow - (BLOCKWIDTH >> 3);
    custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH >> 3);  // Skip over tiles we're not writing
    custom->bltapt  = blocksbuffer + mapy + mapx;
    custom->bltdpt  = dest + y + x;
    
    // Blit width is count * BLOCKWIDTH
    custom->bltsize = (blockplanelines << 6) + (BLOCKWIDTH >> 4);
}
 
void Game_FillScreen(void)
{
    WORD a, b, x, y;
    WORD start_tile_y = mapposy / BLOCKHEIGHT;
    WORD rows = EFFECTIVE_HEIGHT / BLOCKHEIGHT;

    for (b = 0; b < rows; b++)
    {
        for (a = 0; a < 12; a++)
        {
            x = a * BLOCKWIDTH;
            y = b * BLOCKPLANELINES;
            
            DrawBlock(x, y, a, start_tile_y + b, screen.bitplanes);
            DrawBlock(x, y, a, start_tile_y + b, screen.offscreen_bitplanes);
            DrawBlock(x, y, a, start_tile_y + b, screen.pristine);
            
#ifndef USE_YUNLIMITED2
            UWORD yoff = y + (HALFBITMAPHEIGHT * BLOCKSDEPTH);
            DrawBlock(x, yoff, a, start_tile_y + b, screen.bitplanes);
            DrawBlock(x, yoff, a, start_tile_y + b, screen.offscreen_bitplanes);
            DrawBlock(x, yoff, a, start_tile_y + b, screen.pristine);
#endif
        }
    }
 
}
 

void Game_SwapBuffers(void)
{
    // Toggle current buffer
 
    // Update copper bitplane pointers to show the new display buffer
 
    const UBYTE* planes_temp[BLOCKSDEPTH];
    
    planes_temp[0] = draw_buffer;
    planes_temp[1] = draw_buffer + 24;
    planes_temp[2] = draw_buffer + 48;
    planes_temp[3] = draw_buffer + 72;

    LONG planeadd = ((LONG)(videoposy + BLOCKHEIGHT)) * SCREENWIDTH_WORDS;
 
    Copper_SetBitplanePointer(BLOCKSDEPTH, planes_temp, planeadd);
}

void Game_ResetBitplanePointer(void)
{
    // Reset copper bitplane pointers  
 
    const UBYTE* planes_temp[BLOCKSDEPTH];
    
    planes_temp[0] = draw_buffer;
    planes_temp[1] = draw_buffer + 24;
    planes_temp[2] = draw_buffer + 48;
    planes_temp[3] = draw_buffer + 72;

    Copper_SetBitplanePointer(BLOCKSDEPTH, planes_temp, 0);
}


void Game_LoadPalette(const char *filename, UWORD *palette, int num_colors)
{
    // Assuming Dpaint

    ULONG file_size = Pak_GetSize((char *)filename);
    UBYTE *file_data = Pak_LoadAsset((char *)filename, MEMF_ANY);
    if (!file_data) return;
    
    UBYTE *ptr = file_data;
    
    // Check for "FORM" header
    if (ptr[0] != 'F' || ptr[1] != 'O' || ptr[2] != 'R' || ptr[3] != 'M')
    {
        FreeMem(file_data, file_size);  // Free the loaded data
        return;
    }
    
    ptr += 4;  // Skip "FORM"
    
    // Skip file size (4 bytes, big-endian)
    ptr += 4;
    
    // Skip form type (usually "ILBM") - 4 bytes
    ptr += 4;
    
    // Now search for CMAP chunk
    while (1)
    {
        // Check chunk ID
        if (ptr[0] == 'C' && ptr[1] == 'M' && ptr[2] == 'A' && ptr[3] == 'P')
        {
            ptr += 4;  // Skip "CMAP"
            
            // Get chunk size (big-endian)
            ULONG chunk_size = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4;
            
            // ptr now points to RGB data
            UBYTE *rgb_data = ptr;
            
            for (int i = 0; i < num_colors && i < chunk_size/3; i++)
            {
                UBYTE r = *rgb_data++;
                UBYTE g = *rgb_data++;
                UBYTE b = *rgb_data++;
                
                // Convert to Amiga 4-bit per channel
                UWORD r4 = (r >> 4) & 0x0F;
                UWORD g4 = (g >> 4) & 0x0F;
                UWORD b4 = (b >> 4) & 0x0F;
                
                palette[i] = (r4 << 8) | (g4 << 4) | b4;
            }
            
            break;
        }
        
        ptr += 4;  // Skip chunk ID
        
        // Get chunk size and skip over it
        ULONG chunk_size = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;
        ptr += chunk_size;
        
        // Chunks are word-aligned, skip padding byte if odd size
        if (chunk_size & 1) ptr++;
    }
    
    FreeMem(file_data, file_size);  // Free the loaded data
}

void Game_ApplyPalette(UWORD *palette, int num_colors)
{
    for (int i = 0; i < num_colors; i++)
    {
        Copper_SetPalette(i, palette[i]);
    }
}

// Game Ready Stuff

// Add these static variables with the other timers
static GameTimer ready_blink_timer;
static BOOL ready_text_visible = TRUE;

void GameReady_Initialize(void)
{
      // Turn off sprites
    Sprites_Initialize();

    // Clear entire screen including HUD area
    BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 256);
    BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 256);
 
    // Start blink timer
    ready_text_visible = TRUE;
    Timer_StartMs(&ready_blink_timer, 800);  // Blink every 800ms

    Font_DrawStringCentered(draw_buffer, "       PUSH BUTTON", 60, 17);   
    Font_DrawStringCentered(draw_buffer, "       ONLY 1 PLAYER", 100, 17);  
            
 
}

void GameReady_Draw(void)
{
    // Handle blinking "PUSH BUTTON" text
    if (Timer_HasElapsed(&ready_blink_timer))
    {
        ready_text_visible = !ready_text_visible;
        
        if (ready_text_visible)
        {
            Font_DrawStringCentered(draw_buffer, "       PUSH BUTTON", 60, 17);   
            Font_DrawStringCentered(draw_buffer, "       ONLY 1 PLAYER", 100, 17);  
            WaitBlit();
        }
        else
        {
            // Clear the "PUSH BUTTON" area
            Font_ClearArea(draw_buffer, 0, 60, SCREENWIDTH, 8);
            WaitBlit();
        }
        
        Timer_Reset(&ready_blink_timer);
    }
}

void GameReady_Update(void)
{
    if (JoyFirePressed())
    {
        // Clear screens for game start
        BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 256);
        BlitClearScreen(display_buffer, SCREENWIDTH << 6 | 256);
        
        // Transition to actual game
        game_state = STAGE_START;

        stage_state = STAGE_BEGIN;

        // Start HUD update timer (update every 96 frames)
        Timer_StartMs(&hud_update_timer, 96);
 
        Game_NewGame(FIVEHUNDREDCC);

        Game_ApplyPalette(current_palette,BLOCKSCOLORS);

        MotorBike_Reset();
 
        StageProgress_DrawAllEmpty();

        HUD_SetSpritePositions();
        HUD_DrawAll();

        Game_FillScreen();
        Road_CacheFillVisible();
        Game_SwapBuffers(); 

        Stage_ShowInfo();
        StageProgress_SetStage(0);
        Music_LoadModule((game_continues == MAX_CONTINUES) ? MUSIC_START : MUSIC_ONROAD);
    }    
 
}

void Stage_Initialize(void)
{
    Disk_LoadAsset((UBYTE *)city_colors,"palettes/lv1_tiles.PAL");

     // Used for the Countdown
    Sprites_LoadFromFile(COUNTDOWN_ZERO,&spr_countdown_timer[0]);
    Sprites_LoadFromFile(COUNTDOWN_ONE,&spr_countdown_timer[1]);
    Sprites_LoadFromFile(COUNTDOWN_TWO,&spr_countdown_timer[2]);
    Sprites_LoadFromFile(COUNTDOWN_THREE,&spr_countdown_timer[3]);

    spr_countdown[0] = Mem_AllocChip(32);
    spr_countdown[1] = Mem_AllocChip(32);
    spr_countdown[2] = Mem_AllocChip(32);
    spr_countdown[3] = Mem_AllocChip(32);

    Sprites_BuildComposite(spr_countdown[0],2,&spr_countdown_timer[0]);
    Sprites_BuildComposite(spr_countdown[1],2,&spr_countdown_timer[1]);
    Sprites_BuildComposite(spr_countdown[2],2,&spr_countdown_timer[2]);
    Sprites_BuildComposite(spr_countdown[3],2,&spr_countdown_timer[3]); 

    current_countdown_spr = spr_countdown[3];
  
}
 

void Stage_Draw()
{
    if (stage_state == STAGE_COUNTDOWN)
    {
        if (game_frame_count == 0)
        {
            Stage_ShowInfo();   
        }
        Cars_PreDraw();
        Game_SwapBuffers();  
        MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);
    }
    else if (stage_state == STAGE_PLAYING)
    {
        SmoothScroll();
        Cars_Update();
        bike_world_y = mapposy + bike_position_y;

        //Stage_RedrawTunnelTiles();

        Game_SwapBuffers();
 
        if (Timer_HasElapsed(&hud_update_timer))
        {
        switch (hud_phase)
        {
            case 0:
                if (game_score != last_score)
                {
                    HUD_UpdateScore(game_score);
                    last_score = game_score;
                }
                break;
            case 1:
                if (game_rank != last_rank)
                {
                    HUD_UpdateRank(game_rank);
                    last_rank = game_rank;
                }
                break;
            case 2:
                if (bike_speed != last_speed)
                {
                    HUD_UpdateBikeSpeed(bike_speed);
                    last_speed = bike_speed;
                }
                break;
        }
        hud_phase++;
        if (hud_phase > 2) hud_phase = 0;
        Timer_Reset(&hud_update_timer);
        }

        Fuel_Draw(); 
        StageProgress_DrawOverhead();
 
        BarrelTruck_Draw();     

        MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);

        UBYTE surf_top = Collision_GetSurface(bike_position_x + 8, bike_wy);
        UBYTE surf_bot = Collision_GetSurface(bike_position_x + 8, bike_wy + 28);
        
        BOOL in_tunnel = (surf_top == SURFACE_TUNNEL || surf_bot == SURFACE_TUNNEL);
        
        custom->bplcon2 = in_tunnel ? 0x0000 : 0x0024;

        Game_HandleCollisions();
    }
    else if (stage_state == STAGE_FRONTVIEW)
    {       
        City_RestoreOncomingCars();
        City_DrawRoad();
        City_DrawOncomingCars();
        Planes_Restore(draw_buffer);
        Planes_Draw(draw_buffer);

        WORD vibration_x = MotorBike_GetVibrationOffset();
        MotorBike_Draw(bike_position_x + vibration_x, bike_position_y, 0);
        Planes_Update();
        
        WaitBlit();
 
        Game_ResetBitplanePointer();
 
        if (Timer_HasElapsed(&hud_update_timer))
        {
            if (bike_speed != last_speed)
            {
                HUD_UpdateBikeSpeed(bike_speed);
                last_speed = bike_speed;
            }
            Timer_Reset(&hud_update_timer);
        }

        Fuel_Draw(); 
        StageProgress_DrawFrontview();
      
    }
    else if (stage_state == STAGE_CONTINUE)
    {
        char num[2];
        num[0] = '0' + continue_countdown;
        num[1] = '\0';

        Font_DrawStringCentered(draw_buffer, "CONTINUE", 90, 17);

        Font_ClearArea(draw_buffer, 0, 120, SCREENWIDTH, 8);

        if (gameover_text_visible)
            Font_DrawStringCentered(draw_buffer, num, 120, 17);
        else
            Font_ClearArea(draw_buffer, 0, 120, SCREENWIDTH, 8);

        Font_DrawStringCentered(draw_buffer, "PRESS FIRE", 150, 13);

        Game_ResetBitplanePointer();
    }
    else if (stage_state == STAGE_FUEL_EMPTY)
    {
        if (gameover_text_visible)
        {
            Font_DrawStringCentered(draw_buffer, "   EMPTY", fuel_empty_y, 9);
        }
        else
        {
            /* Restore background instead of clearing to black */
            Font_RestoreFromPristine(draw_buffer, 0, fuel_empty_y, SCREENWIDTH, 8);
        }
        
        MotorBike_UpdatePosition(bike_position_x,bike_position_y,bike_state);

        Game_SwapBuffers();
    }
    else if (stage_state == STAGE_COMPLETE)
    {
        City_DrawRoad();
        Planes_Restore(draw_buffer);
        Planes_Update();
        Planes_Draw(draw_buffer);

        NYCVictory_Update();

        if (Timer_HasElapsed(&stage_complete_timer))
        {
            Planes_Stop();

            Timer_Stop(&stage_complete_timer);
            stage_state = STAGE_RANKING;

            // Turn off Bike
            Sprites_ClearLower();
            Sprites_ClearHigher();

            Game_ApplyPalette(current_palette,BLOCKSCOLORS);

            Ranking_Initialize();

            if (game_stage != STAGE_NEWYORK)
            {
                Music_Stop();
                WaitVBL();
                custom->dmacon = DMAF_SETCLR | DMAF_AUD0 | DMAF_AUD1 | DMAF_AUD2 | DMAF_AUD2 ;
                Music_LoadModule(MUSIC_RANKING);
            }
 

        }
    }
    else if (stage_state == STAGE_RANKING)
    {
        RankingState ranking_state = Ranking_GetState();

        switch (ranking_state)
        {
            case RANKING_STATE_DRAWHORIZON:
                BlitClearScreen(draw_buffer, SCREENWIDTH << 6 | 256);
                break;
            case RANKING_STATE_SCROLLING:     
            case RANKING_STATE_FLASHING:     
            case RANKING_STATE_SOLID_RED:
                 BlitClearArea(draw_buffer, 0, 88, VIEWPORT_WIDTH, 200);
                 break;
            case RANKING_STATE_BONUS_DEPLETING: 
            case RANKING_STATE_COMPLETE:   

        }
 
        Ranking_Draw(draw_buffer);
        Game_ResetBitplanePointer();
    }
    else if (stage_state == STAGE_GAMEOVER)
    {
        if (gameover_text_visible)
        {
            Font_DrawStringCentered(draw_buffer, "GAME OVER", 120, 17); 
        }
        else
        {
            Font_ClearArea(draw_buffer, 0, 120, SCREENWIDTH, 8);
        }
        
        Game_ResetBitplanePointer();
    }
    else if (stage_state == STAGE_GAMEOVER_ENTRY)
    {
        NameEntry_Draw(draw_buffer);
        Game_ResetBitplanePointer();
    }   
}

static void CheckBonusPickups(void)
{
    UBYTE surface = Collision_GetSurface(bike_position_x + 8, bike_wy);
    UBYTE surface_l = Collision_GetSurface(bike_position_x, bike_wy);
    UBYTE surface_r = Collision_GetSurface(bike_position_x + 16, bike_wy);

    UWORD pickup_points = 0;
    UBYTE pickup_surface = SURFACE_NORMAL;
    
    if (surface_l >= SURFACE_PTS100 && surface_l <= SURFACE_PTS1000)
        pickup_surface = surface_l;
    else if (surface >= SURFACE_PTS100 && surface <= SURFACE_PTS1000)
        pickup_surface = surface;
    else if (surface_r >= SURFACE_PTS100 && surface_r <= SURFACE_PTS1000)
        pickup_surface = surface_r;
    
    if (pickup_surface != SURFACE_NORMAL)
    {
        switch (pickup_surface)
        {
            case SURFACE_PTS100:  pickup_points = 100;  break;
            case SURFACE_PTS200:  pickup_points = 200;  break;
            case SURFACE_PTS500:  pickup_points = 500;  break;
            case SURFACE_PTS700:  pickup_points = 700;  break;
            case SURFACE_PTS1000: pickup_points = 1000; break;
        }
        
        WORD hit_x = bike_position_x + 8;
        if (surface_l == pickup_surface) hit_x = bike_position_x;
        else if (surface_r == pickup_surface) hit_x = bike_position_x + 16;
        
        WORD map_x = hit_x >> 4;
        WORD map_y = bike_wy >> 4;
        
        game_score += pickup_points;
        HUD_UpdateScore(game_score);
        
        mapdata[map_y * mapwidth + map_x] = road_tile_plain;
        
        UBYTE old_col = Collision_Get(hit_x, bike_wy);
        Collision_Set(hit_x, bike_wy, (old_col & 0xF0) | SURFACE_NORMAL);
        
        WORD buf_y = ROUND2BLOCKHEIGHT((map_y << 4) % EFFECTIVE_HEIGHT) << 2;
        WORD buf_x = map_x << 4;
        
        DrawBlock(buf_x, buf_y, map_x, map_y, draw_buffer);
        DrawBlock(buf_x, buf_y, map_x, map_y, display_buffer);
        DrawBlock(buf_x, buf_y, map_x, map_y, screen.pristine);
        
        UWORD yoff = buf_y + (HALFBITMAPHEIGHT << 2);
        DrawBlock(buf_x, yoff, map_x, map_y, draw_buffer);
        DrawBlock(buf_x, yoff, map_x, map_y, display_buffer);
        DrawBlock(buf_x, yoff, map_x, map_y, screen.pristine);
    }
}

void Stage_Update()
{
    game_frame_count++;

    if (stage_state == STAGE_BEGIN)
    {
        if (game_stage == STAGE_LASVEGAS && game_continues == MAX_CONTINUES)
        {
            countdown_value = 4;
            Timer_Start(&countdown_timer, 1);
            stage_state = STAGE_COUNTDOWN;
        }
        else
        {
            /* Skip countdown — go straight to playing */
            stage_state = STAGE_PLAYING;
            bike_speed = MIN_CRUISING_SPEED;
            bike_state = BIKE_STATE_MOVING;
        }
    }
    else if (stage_state == STAGE_COUNTDOWN)
    {
        // Handle countdown timer
        if (Timer_HasElapsed(&countdown_timer))
        {
            if (countdown_value > 0)
            {
                countdown_value--;
                Timer_Reset(&countdown_timer);  // Reset for next second
                current_countdown_spr = spr_countdown[countdown_value];
                WaitVBL();
             
                custom->intena = INTF_INTEN;

                Sprites_SetPointers(current_countdown_spr, 2, SPRITEPTR_TWO_AND_THREE);
                Sprites_SetScreenPosition((UWORD *)current_countdown_spr[0],96,120,32);
                Sprites_SetScreenPosition((UWORD *)current_countdown_spr[1],96,120,32);

                custom->intena = INTF_SETCLR | INTF_INTEN;
            }
            else
            {
                WORD difficulty_y = videoposy + BLOCKHEIGHT + 72;
                WORD stage_y = videoposy + BLOCKHEIGHT + 128;

                if (difficulty_y >= BITMAPHEIGHT)
                    difficulty_y -= BITMAPHEIGHT;
                if (stage_y >= BITMAPHEIGHT)
                    stage_y -= BITMAPHEIGHT;
    

                Font_RestoreFromPristine(screen.bitplanes, 48, difficulty_y, 192, CHAR_HEIGHT);
                Font_RestoreFromPristine(screen.offscreen_bitplanes, 48, difficulty_y, 192, CHAR_HEIGHT);
                Font_RestoreFromPristine(screen.bitplanes, 48, stage_y, 192, CHAR_HEIGHT);
                Font_RestoreFromPristine(screen.offscreen_bitplanes, 48, stage_y, 192, CHAR_HEIGHT);
                // Countdown complete - start gameplay
                stage_state = STAGE_PLAYING;
                Timer_Stop(&countdown_timer);

                Sprites_ClearLower();
                Sprites_ClearHigher();

                Music_LoadModule(MUSIC_ONROAD);
            }
        }
    }
    else if (stage_state == STAGE_PLAYING)
    {

        HUD_Update1UPFlash();

        speed_accumulator += bike_speed;
        speed_sample_count++;

        // Update fuel gauge
        Fuel_Update();

        BarrelTruck_Update(); 
        
        // Check if fuel is empty
        if (Fuel_IsEmpty())
        {
            bike_state = BIKE_STATE_CRASHED;
            bike_speed = 0;
            prev_bike_state = -1;
            stage_state = STAGE_FUEL_EMPTY;
            
            fuel_empty_y = videoposy + BLOCKHEIGHT + 128;
           
            Timer_Start(&gameover_timer, 4);
            Timer_StartMs(&gameover_flash_timer, 200);
            gameover_text_visible = TRUE;
            Music_Stop();
            return;
        }
        

        if (bike_invulnerable && Timer_HasElapsed(&invuln_timer))
        {
            bike_invulnerable = FALSE;
            Timer_Stop(&invuln_timer);
        }

        if (wheelie_active)
        {
            //bike_speed = wheelie_speed;
            //bike_state = BIKE_STATE_WHEELIE;

            // No skid if offroad over jump
            if (bike_state == BIKE_STATE_WHEELIE )
            {
                if (fuel_alarm_active == FALSE)
                {
                    if ((game_frame_count & 7) == 0)
                        SFX_Play(SFX_SKID);
                }
            }
            
            // Clear the landing path — push cars out of the way
          
            WORD bike_cx = bike_position_x + 8;
            
            for (UBYTE i = 0; i < MAX_CARS; i++)
            {
                if (!car[i].visible || car[i].off_screen || car[i].crashed) continue;
                
                WORD car_screen_y = car[i].y - mapposy;
                WORD car_cx = car[i].x + 16;
                WORD x_dist = ABS(bike_cx - car_cx);
                
                // Car is ahead of bike and in the flight path
                if (car_screen_y < bike_position_y && 
                    car_screen_y > bike_position_y - 80 && 
                    x_dist < 24)
                {
                    // Nudge car sideways out of the path
                    if (car_cx < bike_cx)
                        car[i].x -= 3;
                    else
                        car[i].x += 3;
                }
            }
            
            if (Timer_HasElapsed(&wheelie_timer))
            {
                wheelie_active = FALSE;
                wheelie_scored = FALSE;
                Timer_Stop(&wheelie_timer);
                bike_state = BIKE_STATE_MOVING;
                wheelie_anim_frames = 0;
                Sprites_ClearHigher();
            }

            speed_accumulator += bike_speed;
            speed_sample_count++;
            StageProgress_UpdateOverhead(mapposy);
            Stage_CheckCompletion();

            if (bike_state == BIKE_STATE_JUMP )
            {
                bike_wy = mapposy + bike_position_y - g_sprite_voffset;
                CheckBonusPickups();
            }

            return;
        }
 
        
    if (collision_state == COLLISION_TRAFFIC || 
        collision_state == COLLISION_OFFROAD ||
        collision_state == COLLISION_WATER)
    {
        if (collision_state == COLLISION_WATER)
        {
            /* No sliding — immediate stop */
            bike_speed = 0;
            bike_state = BIKE_STATE_WATER_CRASH;
        }
        else
        {
            if (bike_speed > 0)
            {
                bike_speed -= 4;
                if (bike_speed < 0) bike_speed = 0;
            }
            bike_state = BIKE_STATE_CRASHED;
        }
            
        StageProgress_UpdateOverhead(mapposy);
        Stage_CheckCompletion();
        return;
    }
 
        bike_wy = mapposy + bike_position_y - g_sprite_voffset;

        UBYTE surface = Collision_GetSurface(bike_position_x + 8, bike_wy);
        UBYTE surface_l = Collision_GetSurface(bike_position_x, bike_wy);
        UBYTE surface_r = Collision_GetSurface(bike_position_x + 16, bike_wy);
    
        if ((surface == SURFACE_WHEELIE  || 
            surface == SURFACE_JUMP ) && !wheelie_active)
        {
            wheelie_active = TRUE;
            wheelie_speed = bike_speed;

            Timer_Start(&wheelie_timer,  surface == SURFACE_JUMP ? 1 : 3);
            
            bike_state = surface == SURFACE_JUMP  ? BIKE_STATE_JUMP : BIKE_STATE_WHEELIE;
            wheelie_anim_frames = 0;  // Reset animation counter
            
            if (!wheelie_scored)
            {
                game_score += 700;
                wheelie_scored = TRUE;
            }
            
            speed_accumulator += bike_speed;
            speed_sample_count++;
            StageProgress_UpdateOverhead(mapposy);
            Stage_CheckCompletion();
            return;
        }
 

        UWORD pickup_points = 0;
        UBYTE pickup_surface = SURFACE_NORMAL;
        
        if (surface_l >= SURFACE_PTS100 && surface_l <= SURFACE_PTS1000)
            pickup_surface = surface_l;
        else if (surface >= SURFACE_PTS100 && surface <= SURFACE_PTS1000)
            pickup_surface = surface;
        else if (surface_r >= SURFACE_PTS100 && surface_r <= SURFACE_PTS1000)
            pickup_surface = surface_r;
        
        if (pickup_surface != SURFACE_NORMAL)
        {
            switch (pickup_surface)
            {
                case SURFACE_PTS100:  pickup_points = 100;  break;
                case SURFACE_PTS200:  pickup_points = 200;  break;
                case SURFACE_PTS500:  pickup_points = 500;  break;
                case SURFACE_PTS700:  pickup_points = 700;  break;
                case SURFACE_PTS1000: pickup_points = 1000; break;
            }
            
            /* Find which point hit */
            WORD hit_x = bike_position_x + 8;
            if (surface_l == pickup_surface) hit_x = bike_position_x;
            else if (surface_r == pickup_surface) hit_x = bike_position_x + 16;
            
            WORD map_x = hit_x >> 4;
            WORD map_y = bike_wy >> 4;
            /* Add score */
            game_score += pickup_points;
            HUD_UpdateScore(game_score);
            
            /* Replace tile with plain road */
            mapdata[map_y * mapwidth + map_x] = road_tile_plain;
            
            /* Preserve lane, clear surface */
            UBYTE old_col = Collision_Get(hit_x, bike_wy);
            Collision_Set(hit_x, bike_wy, (old_col & 0xF0) | SURFACE_NORMAL);
            
            /* Redraw on all buffers */
            WORD buf_y = ROUND2BLOCKHEIGHT((map_y * BLOCKHEIGHT) % EFFECTIVE_HEIGHT) << 2;
            WORD buf_x = map_x << 4;
            
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.bitplanes);
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.offscreen_bitplanes);
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.pristine);
            
            UWORD yoff = buf_y + (HALFBITMAPHEIGHT << 2);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.bitplanes);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.offscreen_bitplanes);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.pristine);
        }

        if (surface_l == SURFACE_GASCAN || surface == SURFACE_GASCAN || surface_r == SURFACE_GASCAN)
        {
            WORD hit_x = bike_position_x + 8;
            if (surface_l == SURFACE_GASCAN) hit_x = bike_position_x;
            else if (surface_r == SURFACE_GASCAN) hit_x = bike_position_x + 16;
            
            WORD map_x = hit_x >> 4;
            WORD map_y = bike_wy >> 4;
            
            Fuel_Add(1);
            Fuel_DrawAll();
            
            game_score += 500;
            HUD_UpdateScore(game_score);
            
            mapdata[map_y * mapwidth + map_x] = road_tile_plain;
            
            UBYTE old_col = Collision_Get(hit_x, bike_wy);
            Collision_Set(hit_x, bike_wy, (old_col & 0xF0) | SURFACE_NORMAL);
            
            WORD buf_y = ROUND2BLOCKHEIGHT((map_y << 4) % EFFECTIVE_HEIGHT) << 2;
            WORD buf_x = map_x << 4;
            
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.bitplanes);
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.offscreen_bitplanes);
            DrawBlock(buf_x, buf_y, map_x, map_y, screen.pristine);
            
            UWORD yoff = buf_y + (HALFBITMAPHEIGHT << 2);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.bitplanes);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.offscreen_bitplanes);
            DrawBlock(buf_x, yoff, map_x, map_y, screen.pristine);

            return;
        }

        /* In the surface check area */
        if (surface == SURFACE_PUDDLE)
        {
            if (puddle_timer == 0)
            {
                if (fuel_alarm_active == FALSE)
                {
                    puddle_timer = 18;
                    SFX_Play(SFX_SKID);
                }
            }
        }

        if (surface == SURFACE_BARRELTRUCK && !BarrelTruck_IsActive())
        {
            BarrelTruck_RequestSpawn();
        }

        /* Before the input handling section */
        if (puddle_timer > 0)
        {
            puddle_timer--;
            bike_state = BIKE_STATE_MOVING;  /* No steering animation */
            
            if (fuel_alarm_active == FALSE)
            {
                if ((puddle_timer & 3) == 0)
                    SFX_Play(SFX_SKID);
            }
            
            /* Skip all input — bike just cruises forward */
            StageProgress_UpdateOverhead(mapposy);
            Stage_CheckCompletion();
            return;
        }

        // === BRAKE — button 2 or pull down ===
        if (JoyButton2() || JoyDown())
        {
            bike_speed -= 8;  // Hard braking
            if (bike_speed < MIN_CRUISING_SPEED)
                bike_speed = MIN_CRUISING_SPEED;
            
            bike_state = BIKE_STATE_BRAKING;

            if ((game_frame_count & 7) == 0)
            {
                if (fuel_alarm_active == FALSE)
                {
                    SFX_Play(SFX_BRAKE);
                }
            }
        }

        // === ACCELERATION LOGIC ===

        if (JoyFireHeld())
        {
            // Fire button held - accelerate to max speed
            bike_speed += ACCEL_RATE;   
            if (bike_speed > max_stage_speed)
            {
                bike_speed = max_stage_speed;
            }
            bike_state = BIKE_STATE_ACCELERATING;
        }
        else
        {
            // Fire button not held - auto-adjust to cruising speed
            if (bike_speed < MIN_CRUISING_SPEED)
            {
                // Auto-accelerate to cruising speed SLOWLY  
                bike_speed += 1;   
                if (bike_speed > MIN_CRUISING_SPEED)
                {
                    bike_speed = MIN_CRUISING_SPEED;
                }
                bike_state = BIKE_STATE_ACCELERATING;
            }
            else if (bike_speed > MIN_CRUISING_SPEED)
            {
                // Decelerate back to cruising speed
                bike_speed -= DECEL_RATE;
                if (bike_speed < MIN_CRUISING_SPEED)
                {
                    bike_speed = MIN_CRUISING_SPEED;
                }
                bike_state = BIKE_STATE_BRAKING;
            }
            else
            {
                // Maintain cruising speed
                bike_state = BIKE_STATE_MOVING;
            }
 
        }        
 
        // === LEFT/RIGHT MOVEMENT ===
        if (JoyLeft())
        {
            bike_position_x -= 2;
            // Preserve acceleration state but show turning
            if (bike_state == BIKE_STATE_MOVING || bike_state == BIKE_STATE_ACCELERATING)
            {
                bike_state = BIKE_STATE_LEFT;
            }
        }

        if (JoyRight())
        {
            bike_position_x += 2;
            if (bike_state == BIKE_STATE_MOVING || bike_state == BIKE_STATE_ACCELERATING)
            {
                bike_state = BIKE_STATE_RIGHT;
            }
        }

         /* Clamp bike to road viewport */
        if (bike_position_x < 2)
            bike_position_x = 2;
        if (bike_position_x > SCREENWIDTH - 16)
            bike_position_x = SCREENWIDTH - 16;

        StageProgress_UpdateOverhead(mapposy);

        Stage_CheckCompletion();
    }
    else if (stage_state == STAGE_FUEL_EMPTY)
    {
        /* Flash "EMPTY" */
        if (Timer_HasElapsed(&gameover_flash_timer))
        {
            gameover_text_visible = !gameover_text_visible;
            Timer_Reset(&gameover_flash_timer);
        }
        
        /* After 2 seconds, proceed to game over */
        if (Timer_HasElapsed(&gameover_timer))
        {
            Timer_Stop(&gameover_timer);
            Timer_Stop(&gameover_flash_timer);
            
            stage_state = STAGE_GAMEOVER;
            
            Sprites_ClearLower();
            Sprites_ClearHigher();
            
            Game_ApplyPalette((UWORD *)black_palette, BLOCKSCOLORS);
            WaitVBL();
            
            BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
            BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
            BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);
            
            if (game_continues > 0)
            {
                fuel_alarm_active = FALSE;    
                stage_state = STAGE_CONTINUE;   // offer a continue
                Music_Stop();
            }
            else
            {
        
                fuel_alarm_active = FALSE;
                stage_state = STAGE_GAMEOVER;   // out of continues  
                Music_LoadModule(MUSIC_GAMEOVER);
            }
        }
    }
    else if (stage_state == STAGE_FRONTVIEW)
    {
        HUD_Update1UPFlash();

         // Update fuel gauge
        Fuel_Update();
        
        // Check if fuel is empty
        if (Fuel_IsEmpty())
        {
            // Slow bike to a stop
            bike_speed = 0;
   
            // Turn off Bike
            Sprites_ClearLower();
            Sprites_ClearHigher();

            Game_ApplyPalette((UWORD *)black_palette, BLOCKSCOLORS);
            WaitVBL();
  
            BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
            BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
            BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);
 
            if (game_continues > 0)
            {
                fuel_alarm_active = FALSE;    
                stage_state = STAGE_CONTINUE;   // offer a continue
                Music_Stop();
            }
            else
            {
                fuel_alarm_active = FALSE;
                stage_state = STAGE_GAMEOVER;   // out of continues  
                Music_LoadModule(MUSIC_GAMEOVER);
            }
            
            return;
        }

        if (City_OncomingCarsIsComplete())
        {
            stage_state = STAGE_COMPLETE;

            if (game_stage == STAGE_NEWYORK)
            {
                NYCVictory_Start();
                Timer_Start(&stage_complete_timer, 6);
            }
            else
            {
                Timer_Start(&stage_complete_timer, 2);
            }

            KPrintF("=== Stage %ld Complete! ===\n", game_stage);
            return;
        }

        CityApproachState approach_state = City_GetApproachState();
 
          
        if (frontview_bike_crashed == FALSE && approach_state < CITY_STATE_INTO_HORIZON )
        {
            static UBYTE frontview_bike_anim_frame = 0;   
   
            UWORD anim_speed = 15 - (bike_speed >> 4);  // Faster bike = smaller number
            if (anim_speed < 2) anim_speed = 2;  
 
            if (game_frame_count % anim_speed == 0)
            {
                frontview_bike_anim_frame ^= 1;
            }

            bike_state = frontview_bike_anim_frame ? BIKE_FRAME_APPROACH2 : BIKE_FRAME_APPROACH1;  

            if (JoyFireHeld())
            {
                // Fire button held - accelerate to max speed
                bike_speed += ACCEL_RATE;   
                if (bike_speed > max_stage_speed)
                {
                    bike_speed = max_stage_speed;
                }
                
            }
            else
            {
                // Fire button not held - auto-adjust to cruising speed
                if (bike_speed < MIN_CRUISING_SPEED)
                {
                    // Auto-accelerate to cruising speed SLOWLY  
                    bike_speed += 1;   
                    if (bike_speed > MIN_CRUISING_SPEED)
                    {
                        bike_speed = MIN_CRUISING_SPEED;
                    }
                }
                else if (bike_speed > MIN_CRUISING_SPEED)
                {
                    // Decelerate back to cruising speed
                    bike_speed -= DECEL_RATE;
                    if (bike_speed < MIN_CRUISING_SPEED)
                    {
                        bike_speed = MIN_CRUISING_SPEED;
                    }
                   
                }
            }        
    
            // === LEFT/RIGHT MOVEMENT ===
            if (JoyLeft())
            {
                bike_position_x -= 2;
                bike_state = BIKE_STATE_FRONTVIEW_LEFT;
            }
            else if (JoyRight())
            {
                bike_position_x += 2;
                bike_state = BIKE_STATE_FRONTVIEW_RIGHT;
            }

            /* Clamp bike to road viewport */
            if (bike_position_x < 8)
                bike_position_x = 8;
            if (bike_position_x > SCREENWIDTH - 40)
                bike_position_x = SCREENWIDTH - 40;
            
        }

        if (approach_state == CITY_STATE_INTO_HORIZON)
        {
            if (game_stage == STAGE_HOUSTON && !Planes_IsActive() && bike_position_y < 210)
            {
                Planes_Start();
            }
            else if (game_stage == STAGE_NEWYORK && !Planes_IsActive() && 
                !Planes_IsComplete() && bike_position_y < 210)
            {
                Planes_StartNYC();
            }

            City_UpdateHorizonTransition(&bike_position_y, &bike_speed, game_frame_count);

            bike_position_x = City_CalculateBikePerspectiveX(bike_position_y, 
                                                        City_GetBikeHorizonStartX());

            MotorBike_UpdateApproachFrame(bike_position_y);
        }
        else
        {
           
            if (bike_state == BIKE_STATE_FRONTVIEW_LEFT)
            {
                BikeFrame f = ((bike_anim_frames >> 3) & 1) ? 
                            BIKE_FRAME_APPROACH2_LEFT : BIKE_FRAME_APPROACH1_LEFT;
                MotorBike_SetFrame(f);
                prev_bike_state = bike_state;
                bike_anim_frames++;
            }
            else if (bike_state == BIKE_STATE_FRONTVIEW_RIGHT)
            {
                BikeFrame f = ((bike_anim_frames >> 3) & 1) ? 
                            BIKE_FRAME_APPROACH2_RIGHT : BIKE_FRAME_APPROACH1_RIGHT;
                MotorBike_SetFrame(f);
                prev_bike_state = bike_state;
                bike_anim_frames++;
            }
            else if (bike_state != prev_bike_state)
            {
                MotorBike_SetFrame(bike_state);
                prev_bike_state = bike_state;
            }
        }

        // Update road scrolling
        if (approach_state == CITY_STATE_WAITING_NAME || approach_state == CITY_STATE_ACTIVE)
        {
            UpdateRoadScroll(bike_speed, 0);
        }
        else if (approach_state == CITY_STATE_INTO_HORIZON)
        {
            UpdateRoadScroll(bike_speed, game_frame_count);
        }
    }
    else if (stage_state == STAGE_CONTINUE)
    {
        HUD_Show1UP();
 
        if (!Timer_IsActive(&continue_timer))
        {
            continue_countdown = 9;
            Timer_Start(&continue_timer, 1);          /* one tick per second */
            Timer_StartMs(&gameover_flash_timer, 300);
            gameover_text_visible = TRUE;
            Game_ApplyPalette(intro_colors, BLOCKSCOLORS);
        }

        /* Player accepts the continue */
        if (JoyFirePressed())
        {
            game_continues--;
            Timer_Stop(&continue_timer);
            Timer_Stop(&gameover_flash_timer);

            game_rank  = stage_start_rank;

            Fuel_Reset();               /* full tank again */
            Game_StartNextOverhead();   /* restarts the CURRENT stage from the top */
            return;
        }

        /* Flash the number */
        if (Timer_HasElapsed(&gameover_flash_timer))
        {
            gameover_text_visible = !gameover_text_visible;
            Timer_Reset(&gameover_flash_timer);
        }

        /* Tick 9 -> 0; hitting 0 is real game over */
        if (Timer_HasElapsed(&continue_timer))
        {
            Timer_Reset(&continue_timer);

            if (continue_countdown > 0)
            {
                continue_countdown--;
                SFX_Play(SFX_OVERHEADOVERTAKE);
            }
            else
            {
                Timer_Stop(&continue_timer);
                Timer_Stop(&gameover_flash_timer);
       
                Sprites_ClearLower();
                Sprites_ClearHigher();

                Game_ApplyPalette((UWORD *)black_palette, BLOCKSCOLORS);
                WaitVBL();

                BlitClearScreen(screen.bitplanes,          SCREENWIDTH << 6 | 256);
                BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
                BlitClearScreen(screen.pristine,            SCREENWIDTH << 6 | 256);

                Timer_Stop(&gameover_timer);         

                Music_LoadModule(MUSIC_GAMEOVER);

                stage_state = STAGE_GAMEOVER;
            }
        }
    }
    else if (stage_state == STAGE_RANKING)
    {
        HUD_Show1UP();

        Ranking_Update();
    }
    else if (stage_state == STAGE_GAMEOVER)
    {
        HUD_Show1UP();

        /* First frame — set up */
        if (!Timer_IsActive(&gameover_timer))
        {
           
             /* Record best rank at moment of game over */
            Game_RecordBestRank();

            Timer_Start(&gameover_timer, 6);
            Timer_StartMs(&gameover_flash_timer, 300);
            gameover_text_visible = TRUE;
            
            /* Check high score qualification now */
            gameover_hiscore_pos = HiScore_CheckQualifies(game_score);

            Game_ApplyPalette(intro_colors,BLOCKSCOLORS);

        }
        
        /* Flash "GAME OVER" text */
        if (Timer_HasElapsed(&gameover_flash_timer))
        {
            gameover_text_visible = !gameover_text_visible;
            Timer_Reset(&gameover_flash_timer);
        }
        
        /* After 5 seconds */
        if (Timer_HasElapsed(&gameover_timer))
        {
            Timer_Stop(&gameover_timer);
            Timer_Stop(&gameover_flash_timer);
            
            if (gameover_hiscore_pos > 0)
            {
                /* Qualifies for high score — show name entry */
                stage_state = STAGE_GAMEOVER_ENTRY;
                
                Sprites_ClearLower();
                Sprites_ClearHigher();

                Game_ApplyPalette((UWORD *)black_palette, BLOCKSCOLORS);
                WaitVBL();

                BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
                BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
                BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);
                
                NameEntry_Init(gameover_hiscore_pos);

            }
            else
            {
                /* Doesn't qualify — back to attract */
                Game_Reset();
            }
        }
    }
    else if (stage_state == STAGE_GAMEOVER_ENTRY)
    {
        Game_ApplyPalette((UWORD *)intro_colors, BLOCKSCOLORS);

        NameEntry_Update();
        
        UBYTE joy = 0;
        if (JoyUp()) joy |= 0x01;
        if (JoyDown()) joy |= 0x02;
        if (JoyLeft()) joy |= 0x04;
        if (JoyRight()) joy |= 0x08;
        NameEntry_HandleInput(joy, JoyFirePressed());
        
        if (NameEntry_IsComplete())
        {
            /* Insert into high score table */
            const char *name = NameEntry_GetName();
            HiScore_Insert(game_score, game_best_rank, name);
            
            /* Return to attract */
            Game_Reset();
        }
    }
}
 
void Stage_ShowInfo(void)
{
    char* difficulty_text;
    char* stage_text;
    
    // Determine difficulty text
    switch(game_difficulty)
    {
        case FIVEHUNDREDCC:
            difficulty_text = "500CC    READY";
            game_car_block_move_rate = 4;   // Slow (every 10 frames)
            game_car_block_move_speed = 1;   // 1 pixel at a time
            game_car_block_x_threshold = 48; // Wide threshold 
            break;
        case SEVENFIFTYCC:
            difficulty_text = "750CC    READY";
            break;
        case TWELVEHUNDREDCC:
            difficulty_text = "1200CC   READY";
            break;
        default:
            difficulty_text = "500CC    READY";
            break;
    }
    
    // Determine stage text
    stage_text = " LOS ANGELES";
    
    WORD difficulty_y = videoposy + BLOCKHEIGHT + 72;
    WORD stage_y = videoposy + BLOCKHEIGHT + 128;

    if (difficulty_y >= BITMAPHEIGHT)
        difficulty_y -= BITMAPHEIGHT;
    if (stage_y >= BITMAPHEIGHT)
        stage_y -= BITMAPHEIGHT;
 
    Font_DrawString(screen.bitplanes, difficulty_text, 48, difficulty_y, 13);
    Font_DrawStringCentered(screen.bitplanes, stage_text, stage_y, 13);
    Font_DrawString(screen.offscreen_bitplanes, difficulty_text, 48, difficulty_y, 13);
    Font_DrawStringCentered(screen.offscreen_bitplanes, stage_text, stage_y, 13);
}

void Game_HandleCollisions(void)
{
    if (collision_state == COLLISION_NONE)
    {
        WORD hit_car = -1;
        collision_state = MotorBike_CheckCollision(&hit_car);

        if (collision_state == COLLISION_TRAFFIC)
        {
            collision_car_index = hit_car;
            Timer_Start(&collision_recovery_timer, 3);
            Cars_HandleSpinout(hit_car);
            Music_Stop();
            crash_anim_frames = 0;
            crash_spin_frame = 0;
            SFX_Play(SFX_CRASHSKID);
        }
        else if (collision_state == COLLISION_WATER)
        {
            collision_car_index = -1;
            Timer_Start(&collision_recovery_timer, 3);
            crash_anim_frames = 0;
            crash_spin_frame = 0;
            bike_speed = 0;
            SFX_Play(SFX_CRASHSKID);
            Music_Stop();
        }
        else if (collision_state == COLLISION_OFFROAD)
        {
            Timer_Start(&collision_recovery_timer, 2);
            Music_Stop();
            crash_anim_frames = 0;
            crash_spin_frame = 0;
            SFX_Play(SFX_CRASHSKID);
        }
    }
    
    if (collision_state != COLLISION_NONE)
    {

        if (collision_state == COLLISION_TRAFFIC || 
            collision_state == COLLISION_OFFROAD)
        {
            bike_state = BIKE_STATE_CRASHED;
        }
        else if (collision_state == COLLISION_WATER)
        {
            bike_state = BIKE_STATE_WATER_CRASH;
        }
 
        if (Timer_HasElapsed(&collision_recovery_timer))
        {
            MotorBike_CrashAndReposition();
        
            //if (collision_state != COLLISION_WATER)
            //    Cars_ScatterAfterCrash();

            BarrelTruck_Stop();  
            
            collision_state = COLLISION_NONE;
            collision_car_index = -1;
            Timer_Stop(&collision_recovery_timer);
         
            switch (game_stage)
            {
                case STAGE_LASVEGAS:  Music_LoadModule(MUSIC_ONROAD); break;
                case STAGE_HOUSTON:   Music_LoadModule(MUSIC_OFFROAD); break;
                case STAGE_STLOUIS:   Music_LoadModule(MUSIC_ONROAD); break;
                case STAGE_CHICAGO:   Music_LoadModule(MUSIC_OFFROAD); break;
                case STAGE_NEWYORK:   Music_LoadModule(MUSIC_ONROAD); break;
                default:              Music_LoadModule(MUSIC_ONROAD); break;
            }

            Fuel_Decrease(1);
            Fuel_DrawAll();

            bike_state = BIKE_STATE_MOVING;
            bike_speed = MIN_CRUISING_SPEED;
        }
    }
}

void Stage_CheckCompletion(void)
{
    /* Complete when bike reaches near the top of the map */
    /* mapsize is the starting mapposy (bottom of map) */
    /* current_map_pos counts up from 0 to mapsize */
    LONG completion_threshold = stage_progress.mapsize - (BLOCKHEIGHT << 1);
    
    if (stage_progress.current_map_pos >= completion_threshold)
    {

        stage_state = STAGE_FRONTVIEW;

        Sprites_ClearLower();
        Sprites_ClearHigher();
        
        Stage_InitializeFrontView();
        HUD_UpdateBikeSpeed(bike_speed);
        HUD_UpdateScore(game_score);
    }
}

void Stage_InitializeFrontView(void)
{
    Game_ResetBitplanePointer();

    current_buffer = 0;
    draw_buffer = screen.bitplanes;
    display_buffer = screen.offscreen_bitplanes;

    Sprites_ClearLower();
    Sprites_ClearHigher();

    Game_ApplyPalette(black_palette, BLOCKSCOLORS);

    BlitClearScreen(screen.bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.offscreen_bitplanes, SCREENWIDTH << 6 | 256);
    BlitClearScreen(screen.pristine, SCREENWIDTH << 6 | 256);
 
    bike_position_x = 80;
    bike_position_y = g_is_pal ? 200 : 184;
 
    /* Swap to city attract tiles for front view */
    TilesheetPool_Load(TILEPOOL_CITY_ATTRACT);

    City_ResetRoadState();

    /* Per-stage: map, skyline, city name */
    switch (game_stage)
    {
        case STAGE_LASVEGAS:
            Game_SetMap(STAGE1_FRONTVIEW);
            Skyline_Load(SKYLINE_VEGAS);
           
            break;
        case STAGE_HOUSTON:
            Game_SetMap(STAGE2_FRONTVIEW);
            Skyline_Load(SKYLINE_HOUSTON);
         
            break;
        case STAGE_STLOUIS:
            Game_SetMap(STAGE3_FRONTVIEW);
            Skyline_Load(SKYLINE_STLOUIS);
           
            break;
        case STAGE_CHICAGO:
            Game_SetMap(STAGE4_FRONTVIEW);
            Skyline_Load(SKYLINE_CHICAGO);
        
            break;
        case STAGE_NEWYORK:
            Game_SetMap(STAGE5_FRONTVIEW);
            Skyline_Load(SKYLINE_NYC);
          
            break;
        default:
            Game_SetMap(STAGE1_FRONTVIEW);
            Skyline_Load(SKYLINE_NYC);
           
            break;
    }
 
    Title_Reset();
 
    /* Reset horizon — clear stale restore state from previous stage */
    city_horizon->y = 0;
    city_horizon->old_x = 0;
    city_horizon->old_y = 0;
    city_horizon->prev_old_x = 0;
    city_horizon->prev_old_y = 0;
    city_horizon->needs_restore = FALSE;
    city_horizon->off_screen = FALSE;
    
    City_PreDrawRoad();

    City_OncomingCarsReset();
     
    Planes_ResetDone();

    switch (game_stage)
    {
        case STAGE_LASVEGAS:  City_ShowCityName("LAS VEGAS");  break;
        case STAGE_HOUSTON:   City_ShowCityName("HOUSTON");    break;
        case STAGE_STLOUIS:   City_ShowCityName("ST. LOUIS");  break;
        case STAGE_CHICAGO:   City_ShowCityName("CHICAGO");    break;
        case STAGE_NEWYORK:   City_ShowCityName("NEW YORK");   break;
        default:              City_ShowCityName("LAS VEGAS");  break;
    }

    prev_bike_state = -1;
    bike_state = BIKE_STATE_MOVING;
    Game_ApplyPalette(current_palette, BLOCKSCOLORS);
     
    MotorBike_Reset();
    MotorBike_ResetApproachIndex();

    HUD_DrawAll();
    HUD_UpdateScore(game_score);
 
    Music_Stop();
    Music_LoadModule(MUSIC_FRONTVIEW);

    Game_SetBackGroundColor(0x00);
}

void Stage_RedrawTunnelTiles(void)
{
    WORD start_row = mapposy >> 4;          /* / BLOCKHEIGHT (16) */
    WORD end_row = (mapposy + SCREENHEIGHT) >> 4;
    
    if (start_row < 0) start_row = 0;
    if (++end_row > col_map_height) end_row = col_map_height;
    
    /* Pointer to collision map row */
    UBYTE *col_row = collision_map + (start_row * col_map_width);
    WORD row_y = start_row << 4;            /* row * BLOCKHEIGHT */
    
    for (WORD row = start_row; row < end_row; row++)
    {
        /* Quick scan: any tunnel tiles in this row? */
        BOOL has_tunnel = FALSE;
        for (WORD col = 0; col < col_map_width; col++)
        {
            if ((col_row[col] & 0x0F) == SURFACE_TUNNEL)
            {
                has_tunnel = TRUE;
                break;
            }
        }
        
        if (has_tunnel)
        {
            WORD buf_y = ROUND2BLOCKHEIGHT(row_y % EFFECTIVE_HEIGHT) << 2;
            WORD buf_x = 0;
            
            for (WORD col = 0; col < col_map_width; col++)
            {
                if ((col_row[col] & 0x0F) == SURFACE_TUNNEL)
                {
                    DrawBlock(buf_x, buf_y, col, row, draw_buffer);
                }
                buf_x += 16;
            }
        }
        
        col_row += col_map_width;
        row_y += 16;
    }
}

void Game_RecordBestRank(void)
{
    if (game_rank < todays_best_rank)
        todays_best_rank = game_rank;
}

UBYTE Game_GetTodaysBestRank(void)
{
    return todays_best_rank;
}