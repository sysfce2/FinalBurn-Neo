// FB Neo Haunted Castle / Akuma-Jou Dracula driver module
// Based on MAME driver by Bryan McPhail

#include "tiles_generic.h"
#include "z80_intf.h"
#include "konami_intf.h"
#include "burn_ym3812.h"
#include "k051649.h"
#include "k007232.h"
#include "k007121.h"

static UINT8 *AllMem;
static UINT8 *MemEnd;
static UINT8 *AllRam;
static UINT8 *RamEnd;
static UINT8 *DrvKonROM;
static UINT8 *nDrvKonBank;
static UINT8 *DrvZ80ROM;
static UINT8 *DrvGfxROM[2];
static UINT8 *DrvPalROM;
static UINT8 *DrvSndROM;
static UINT8 *DrvKonRAM0;
static UINT8 *DrvKonRAM1;
static UINT8 *DrvPalRAM;
static UINT8 *DrvPfRAM[2];
static UINT8 *DrvSprRAM[2];
static UINT8 *DrvZ80RAM;
static UINT8 *Palette;
static UINT32 *DrvPalette;
static UINT8 DrvRecalc;

static UINT8 DrvJoy1[8];
static UINT8 DrvJoy2[8];
static UINT8 DrvJoy3[8];
static UINT8 DrvDips[3];
static UINT8 DrvInputs[3];
static UINT8 DrvReset;

static UINT8 *soundlatch;
static UINT8 *gfxbank;
static UINT8 *sound_bank;

static INT32 watchdog;

static INT32 nExtraCycles;

static struct BurnInputInfo HcastleInputList[] = {
	{"P1 Coin",			BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy1 + 3,	"p1 start"	},
	{"P1 Up",			BIT_DIGITAL,	DrvJoy2 + 2,	"p1 up"		},
	{"P1 Down",			BIT_DIGITAL,	DrvJoy2 + 3,	"p1 down"	},
	{"P1 Left",			BIT_DIGITAL,	DrvJoy2 + 0,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy2 + 1,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy2 + 4,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy2 + 5,	"p1 fire 2"	},

	{"P2 Coin",			BIT_DIGITAL,	DrvJoy1 + 1,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy1 + 4,	"p2 start"	},
	{"P2 Up",			BIT_DIGITAL,	DrvJoy3 + 2,	"p2 up"		},
	{"P2 Down",			BIT_DIGITAL,	DrvJoy3 + 3,	"p2 down"	},
	{"P2 Left",			BIT_DIGITAL,	DrvJoy3 + 0,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy3 + 1,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy3 + 4,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy3 + 5,	"p2 fire 2"	},

	{"Reset",			BIT_DIGITAL,	&DrvReset,		"reset"		},
	{"Service",			BIT_DIGITAL,	DrvJoy1 + 2,	"service"	},
	{"Dip A",			BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
	{"Dip B",			BIT_DIPSWITCH,	DrvDips + 1,	"dip"		},
	{"Dip C",			BIT_DIPSWITCH,	DrvDips + 2,	"dip"		},
};

STDINPUTINFO(Hcastle)

static struct BurnDIPInfo HcastleDIPList[]=
{
	DIP_OFFSET(0x12)
	{0x00, 0xff, 0xff, 0x53, NULL			},
	{0x01, 0xff, 0xff, 0xff, NULL			},
	{0x02, 0xff, 0xff, 0xff, NULL			},

	{0   , 0xfe, 0   ,    2, "Cabinet"		},
	{0x00, 0x01, 0x04, 0x00, "Upright"		},
	{0x00, 0x01, 0x04, 0x04, "Cocktail"		},

	{0   , 0xfe, 0   ,    4, "Difficulty"		},
	{0x00, 0x01, 0x18, 0x18, "Easy"			},
	{0x00, 0x01, 0x18, 0x10, "Normal"		},
	{0x00, 0x01, 0x18, 0x08, "Hard"			},
	{0x00, 0x01, 0x18, 0x00, "Hardest"		},

	{0   , 0xfe, 0   ,    4, "Strength of Player"		},
	{0x00, 0x01, 0x60, 0x00, "Very Weak"		},
	{0x00, 0x01, 0x60, 0x20, "Weak"		},
	{0x00, 0x01, 0x60, 0x40, "Normal"			},
	{0x00, 0x01, 0x60, 0x60, "Strong"		},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"		},
	{0x00, 0x01, 0x80, 0x80, "Off"			},
	{0x00, 0x01, 0x80, 0x00, "On"			},

	{0   , 0xfe, 0   ,   16, "Coin A"		},
	{0x01, 0x01, 0x0f, 0x02, "4 Coins 1 Credit"	},
	{0x01, 0x01, 0x0f, 0x05, "3 Coins 1 Credit"	},
	{0x01, 0x01, 0x0f, 0x08, "2 Coins 1 Credit"	},
	{0x01, 0x01, 0x0f, 0x04, "3 Coins 2 Credits"	},
	{0x01, 0x01, 0x0f, 0x01, "4 Coins 3 Credits"	},
	{0x01, 0x01, 0x0f, 0x0f, "1 Coin  1 Credit"	},
	{0x01, 0x01, 0x0f, 0x03, "3 Coins 4 Credits"	},
	{0x01, 0x01, 0x0f, 0x07, "2 Coins 3 Credits"	},
	{0x01, 0x01, 0x0f, 0x0e, "1 Coin  2 Credits"	},
	{0x01, 0x01, 0x0f, 0x06, "2 Coins 5 Credits"	},
	{0x01, 0x01, 0x0f, 0x0d, "1 Coin  3 Credits"	},
	{0x01, 0x01, 0x0f, 0x0c, "1 Coin  4 Credits"	},
	{0x01, 0x01, 0x0f, 0x0b, "1 Coin  5 Credits"	},
	{0x01, 0x01, 0x0f, 0x0a, "1 Coin  6 Credits"	},
	{0x01, 0x01, 0x0f, 0x09, "1 Coin  7 Credits"	},
	{0x01, 0x01, 0x0f, 0x00, "Free Play"		},

	{0   , 0xfe, 0   ,   15, "Coin B"		},
	{0x01, 0x01, 0xf0, 0x20, "4 Coins 1 Credit"	},
	{0x01, 0x01, 0xf0, 0x50, "3 Coins 1 Credit"	},
	{0x01, 0x01, 0xf0, 0x80, "2 Coins 1 Credit"	},
	{0x01, 0x01, 0xf0, 0x40, "3 Coins 2 Credits"	},
	{0x01, 0x01, 0xf0, 0x10, "4 Coins 3 Credits"	},
	{0x01, 0x01, 0xf0, 0xf0, "1 Coin  1 Credit"	},
	{0x01, 0x01, 0xf0, 0x30, "3 Coins 4 Credits"	},
	{0x01, 0x01, 0xf0, 0x70, "2 Coins 3 Credits"	},
	{0x01, 0x01, 0xf0, 0xe0, "1 Coin  2 Credits"	},
	{0x01, 0x01, 0xf0, 0x60, "2 Coins 5 Credits"	},
	{0x01, 0x01, 0xf0, 0xd0, "1 Coin  3 Credits"	},
	{0x01, 0x01, 0xf0, 0xc0, "1 Coin  4 Credits"	},
	{0x01, 0x01, 0xf0, 0xb0, "1 Coin  5 Credits"	},
	{0x01, 0x01, 0xf0, 0xa0, "1 Coin  6 Credits"	},
	{0x01, 0x01, 0xf0, 0x90, "1 Coin  7 Credits"	},

	{0   , 0xfe, 0   ,    2, "Flip Screen"		},
	{0x02, 0x01, 0x01, 0x01, "Off"			},
	{0x02, 0x01, 0x01, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Upright Controls"	},
	{0x02, 0x01, 0x02, 0x02, "Single"		},
	{0x02, 0x01, 0x02, 0x00, "Dual"			},

	{0   , 0xfe, 0   ,    2, "Service Mode"		},
	{0x02, 0x01, 0x04, 0x04, "Off"			},
	{0x02, 0x01, 0x04, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Allow Continue"	},
	{0x02, 0x01, 0x08, 0x08, "Yes"			},
	{0x02, 0x01, 0x08, 0x00, "No"			},
};

STDDIPINFO(Hcastle)

static void bankswitch(INT32 data)
{
	*nDrvKonBank = data & 0x0f;
	INT32 bankaddress = *nDrvKonBank * 0x2000;

	konamiMapMemory(DrvKonROM + 0x10000 + bankaddress, 0x6000, 0x7fff, MAP_ROM);
}

static void hcastle_write(UINT16 address, UINT8 data)
{
	if ((address & 0xfff8) == 0x0000) {
		k007121_ctrl_write(0, address & 7, data);
		return;
	}

	if ((address & 0xfff8) == 0x0200) {
		k007121_ctrl_write(1, address & 7, data);
		return;
	}

	if ((address & 0xff00) == 0x0000) {
		DrvKonRAM0[address & 0xff] = data;
		return;
	}

	if ((address & 0xff00) == 0x0200) {
		DrvKonRAM1[address & 0xff] = data;
		return;
	}

	switch (address)
	{
		case 0x0400:
			bankswitch(data);
		return;

		case 0x0404:
			*soundlatch = data;
		return;

		case 0x0408:
			ZetSetIRQLine(0, CPU_IRQSTATUS_ACK);
		return;

		case 0x040c:
			watchdog = 0;
		return;

		case 0x0410:

		return;

		case 0x0418:
			*gfxbank = data;
		return;
	}
}

static UINT8 hcastle_read(UINT16 address)
{
	switch (address)
	{
		case 0x0410:
		case 0x0411:
		case 0x0412:
			return DrvInputs[address & 3];

		case 0x0413:
			return DrvDips[2];

		case 0x0414:
			return DrvDips[1];

		case 0x0415:
			return DrvDips[0];

		case 0x0418:
			return *gfxbank;
	}

	return 0;
}

static void sound_bankswitch(INT32 data)
{
	*sound_bank = data;

	INT32 bank_A=(data&0x3);
	INT32 bank_B=((data>>2)&0x3);

	k007232_set_bank(0, bank_A, bank_B );
}

static void __fastcall hcastle_sound_write(UINT16 address, UINT8 data)
{
	if ((address & 0xff00) == 0x9800) {
		K051649Write(address & 0xff, data);
		return;
	}

	if (address >= 0xb000 && address <= 0xb00d) {
		K007232WriteReg(0, address & 0x0f, data);
		return;
	}

	switch (address)
	{
		case 0xa000:
		case 0xa001:
			BurnYM3812Write(0, address & 1, data);
		return;

		case 0xc000:
			sound_bankswitch(data);
		return;
	}
}

static UINT8 __fastcall hcastle_sound_read(UINT16 address)
{
	if (address >= 0xb000 && address <= 0xb00d) {
		return K007232ReadReg(0, address & 0x0f);
	}

	switch (address)
	{
		case 0xa000:
		case 0xa001:
			return BurnYM3812Read(0, address & 1);

		case 0xd000:
			ZetSetIRQLine(0, CPU_IRQSTATUS_NONE);
			return *soundlatch;
	}

	return 0;
}

template <int chip>
static tilemap_callback( tile )
{
	UINT8 ctrl_3 = k007121_ctrl_read(chip, 3);
	UINT8 ctrl_5 = k007121_ctrl_read(chip, 5);
	UINT8 ctrl_6 = k007121_ctrl_read(chip, 6);
	INT32 bit0 = (ctrl_5 >> 0) & 0x03;
	INT32 bit1 = (ctrl_5 >> 2) & 0x03;
	INT32 bit2 = (ctrl_5 >> 4) & 0x03;
	INT32 bit3 = (ctrl_5 >> 6) & 0x03;
	INT32 attr = DrvPfRAM[chip][offs];
	INT32 tile = DrvPfRAM[chip][offs + 0x400];
	INT32 color = attr & 0x7;
	INT32 bank = ((attr & 0x80) >> 7) |
			((attr >> (bit0 + 2)) & 0x02) |
			((attr >> (bit1 + 1)) & 0x04) |
			((attr >> (bit2    )) & 0x08) |
			((attr >> (bit3 - 1)) & 0x10);

	INT32 bank_base = (chip == 0 ? 0 : 0x4000 * ((*gfxbank & 2) >> 1));
	if (ctrl_3 & 0x01) bank_base += 0x2000;

	TILE_SET_INFO(chip, tile + bank * 0x100 + bank_base, ((ctrl_6 & 0x30) * 2 + 16) + color, 0);
}

static tilemap_scan( hcastle )
{
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 6);
}

static void DrvK007232VolCallback(INT32 v)
{
	K007232SetVolume(0, 0, (v >> 0x4) * 0x11, 0);
	K007232SetVolume(0, 1, 0, (v & 0x0f) * 0x11);
}

inline static INT32 DrvSynchroniseStream(INT32 nSoundRate)
{
	return (INT64)ZetTotalCycles() * nSoundRate / 3579545;
}

static INT32 DrvDoReset()
{
	memset (AllRam, 0, RamEnd - AllRam);

	konamiOpen(0);
	konamiReset();
	konamiClose();

	ZetOpen(0);
	ZetReset();
	ZetClose();

	k007121_reset();
	K007232Reset(0);
	K051649Reset();
	BurnYM3812Reset();

	watchdog = 0;
	nExtraCycles = 0;

	HiscoreReset();

	return 0;
}

static INT32 MemIndex()
{
	UINT8 *Next; Next = AllMem;

	DrvKonROM		= Next; Next += 0x030000;
	DrvZ80ROM		= Next; Next += 0x010000;

	DrvGfxROM[0]	= Next; Next += 0x200000;
	DrvGfxROM[1]	= Next; Next += 0x200000;

	DrvPalROM		= Next; Next += 0x000400;

	DrvSndROM		= Next; Next += 0x080000;

	Palette			= Next; Next += 0x001000;
	DrvPalette		= (UINT32*)Next; Next += 0x1000 * sizeof(UINT32);

	AllRam			= Next;

	DrvKonRAM0		= Next; Next += 0x000100;
	DrvKonRAM1		= Next; Next += 0x000100;
	DrvPalRAM		= Next; Next += 0x002000;
	DrvPfRAM[0]		= Next; Next += 0x001000;
	DrvPfRAM[1]		= Next; Next += 0x001000;
	DrvSprRAM[0]	= Next; Next += 0x001000;
	DrvSprRAM[1]	= Next; Next += 0x001000;

	DrvZ80RAM		= Next; Next += 0x000800;

	nDrvKonBank		= Next; Next += 0x000001;
	soundlatch		= Next; Next += 0x000001;
	gfxbank			= Next; Next += 0x000001;
	sound_bank      = Next; Next += 0x000001;

	RamEnd			= Next;
	MemEnd			= Next;

	return 0;
}

static void DrvGfxExpand(UINT8 *src, INT32 len)
{
	for (INT32 i = (len - 1) * 2; i >= 0; i-=2) {
		src[i+0] = src[i/2] >> 4;
		src[i+1] = src[i/2] & 0xf;
	}
}

static void DrvPaletteInit()
{
	for (INT32 chip = 0; chip < 2; chip++)
	{
		for (INT32 pal = 0; pal < 8; pal++)
		{
			INT32 clut = (chip << 1) | (pal & 1);

			for (INT32 i = 0; i < 0x100; i++)
			{
				UINT8 ctabentry;

				if (((pal & 0x01) == 0) && (DrvPalROM[(clut << 8) | i] == 0))
					ctabentry = 0;
				else
					ctabentry = (pal << 4) | (DrvPalROM[(clut << 8) | i] & 0x0f);

				Palette[(chip << 11) | (pal << 8) | i] = ctabentry;
			}
		}
	}
}

static INT32 DrvInit()
{
	BurnSetRefreshRate(59);

	BurnAllocMemIndex();

	{
		if (BurnLoadRom(DrvKonROM  + 0x00000,  0, 1)) return 1;
		if (BurnLoadRom(DrvKonROM  + 0x10000,  1, 1)) return 1;

		if (BurnLoadRom(DrvZ80ROM  + 0x00000,  2, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM[0] + 0x00000,  3, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM[0] + 0x80000,  4, 1)) return 1;
		BurnByteswap(DrvGfxROM[0], 0x100000);

		if (BurnLoadRom(DrvGfxROM[1] + 0x00000,  5, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM[1] + 0x80000,  6, 1)) return 1;
		BurnByteswap(DrvGfxROM[1], 0x100000);

		if (BurnLoadRom(DrvSndROM  + 0x00000,  7, 1)) return 1;

		if (BurnLoadRom(DrvPalROM  + 0x00000,  8, 1)) return 1;
		if (BurnLoadRom(DrvPalROM  + 0x00100,  9, 1)) return 1;
		if (BurnLoadRom(DrvPalROM  + 0x00200, 10, 1)) return 1;
		if (BurnLoadRom(DrvPalROM  + 0x00300, 11, 1)) return 1;

		DrvPaletteInit();
		DrvGfxExpand(DrvGfxROM[0], 0x100000);
		DrvGfxExpand(DrvGfxROM[1], 0x100000);
	}

	konamiInit(0);
	konamiOpen(0);
	konamiMapMemory(DrvKonRAM0,		0x0000, 0x00ff, MAP_ROM); //020-03f
	konamiMapMemory(DrvKonRAM1,		0x0200, 0x02ff, MAP_ROM); //220-23f
	konamiMapMemory(DrvPalRAM,		0x0600, 0x1fff, MAP_RAM);
	konamiMapMemory(DrvPfRAM[0],	0x2000, 0x2fff, MAP_RAM);
	konamiMapMemory(DrvSprRAM[0],	0x3000, 0x3fff, MAP_RAM);
	konamiMapMemory(DrvPfRAM[1],	0x4000, 0x4fff, MAP_RAM);
	konamiMapMemory(DrvSprRAM[1],	0x5000, 0x5fff, MAP_RAM);
	konamiMapMemory(DrvKonROM + 0x10000,	0x6000, 0x7fff, MAP_ROM);
	konamiMapMemory(DrvKonROM,		0x8000, 0xffff, MAP_ROM);
	konamiSetWriteHandler(hcastle_write);
	konamiSetReadHandler(hcastle_read);
	konamiClose();

	ZetInit(0);
	ZetOpen(0);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80ROM);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80ROM);
	ZetMapArea(0x8000, 0x87ff, 0, DrvZ80RAM);
	ZetMapArea(0x8000, 0x87ff, 1, DrvZ80RAM);
	ZetMapArea(0x8000, 0x87ff, 2, DrvZ80RAM);
	ZetSetWriteHandler(hcastle_sound_write);
	ZetSetReadHandler(hcastle_sound_read);
	ZetClose();

	BurnYM3812Init(1, 3579545, NULL, DrvSynchroniseStream, 0);
	BurnTimerAttach(&ZetConfig, 3579545);
	BurnYM3812SetRoute(0, BURN_SND_YM3812_ROUTE, 0.70, BURN_SND_ROUTE_BOTH);

	K007232Init(0, 3579545, DrvSndROM, 0x80000); // no idea...
	K007232SetPortWriteHandler(0, DrvK007232VolCallback);
	K007232SetRoute(0, BURN_SND_K007232_ROUTE_1, 0.44, BURN_SND_ROUTE_BOTH);
	K007232SetRoute(0, BURN_SND_K007232_ROUTE_2, 0.50, BURN_SND_ROUTE_BOTH);

	K051649Init(3579545/2);
	K051649SetRoute(0.45, BURN_SND_ROUTE_BOTH);

	k007121_init(0, (0x200000 / (8 * 8)) - 1, DrvSprRAM[0]);
	k007121_init(1, (0x200000 / (8 * 8)) - 1, DrvSprRAM[1]);

	GenericTilesInit();

	GenericTilemapInit(0, hcastle_map_scan, tile_map_callback<0>, 8, 8, 64, 32);
	GenericTilemapInit(1, hcastle_map_scan, tile_map_callback<1>, 8, 8, 64, 32);
	GenericTilemapSetGfx(0, DrvGfxROM[0], 4, 8, 8, 0x200000,     0, 0x7f);
	GenericTilemapSetGfx(1, DrvGfxROM[1], 4, 8, 8, 0x200000, 0x800, 0x7f);
	GenericTilemapSetOffsets(TMAP_GLOBAL, 0, -16);
	GenericTilemapSetTransparent(0, 0);

	DrvDoReset();

	return 0;
}

static INT32 DrvExit()
{
	GenericTilesExit();

	konamiExit();
	ZetExit();

	K007232Exit();
	K051649Exit();
	BurnYM3812Exit();

	BurnFreeMemIndex();

	return 0;
}

static void draw_sprites(INT32 chip)
{
	INT32 base_color = ((k007121_ctrl_read(chip, 6) & 0x30) << 1);
	INT32 bank_base = (chip == 0) ? 0x4000 * (*gfxbank & 1) : 0;

	k007121_draw(chip, pTransDraw, DrvGfxROM[chip], NULL, base_color, 0, 16, bank_base, -1, 0x0000);
}

static INT32 DrvDraw()
{
	if (DrvRecalc) {
		UINT8 r,g,b;
		UINT32 tmp[0x80];

		for (INT32 i = 0; i < 0x100; i+=2) {
			UINT16 d = DrvPalRAM[i + 1] | (DrvPalRAM[i] << 8);
			r = (d >>  0) & 0x1f;
			g = (d >>  5) & 0x1f;
			b = (d >> 10) & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			tmp[i/2] = BurnHighCol(r, g, b, 0);
		}

		for (INT32 i = 0; i < 0x1000; i++) {
			DrvPalette[i] = tmp[Palette[i]];
		}
	}

	BurnTransferClear();

	UINT8 ctrl_1_0 = k007121_ctrl_read(0, 0);
	UINT8 ctrl_1_1 = k007121_ctrl_read(0, 1);
	UINT8 ctrl_1_2 = k007121_ctrl_read(0, 2);
	UINT8 ctrl_2_0 = k007121_ctrl_read(1, 0);
	UINT8 ctrl_2_1 = k007121_ctrl_read(1, 1);
	UINT8 ctrl_2_2 = k007121_ctrl_read(1, 2);

	GenericTilemapSetScrollY(1, ctrl_2_2);
	GenericTilemapSetScrollX(1, ((ctrl_2_1 << 8) + ctrl_2_0));
	GenericTilemapSetScrollY(0, ctrl_1_2);
	GenericTilemapSetScrollX(0, ((ctrl_1_1 << 8) + ctrl_1_0));

	if ((*gfxbank & 0x04) == 0)
	{
		if (nBurnLayer & 1) GenericTilemapDraw(1, pTransDraw, 0);
		if (nSpriteEnable & 1) draw_sprites(0);
		if (nSpriteEnable & 2) draw_sprites(1);
		if (nBurnLayer & 2) GenericTilemapDraw(0, pTransDraw, 0);
	}
	else
	{
		if (nBurnLayer & 1) GenericTilemapDraw(1, pTransDraw, 0);
		if (nBurnLayer & 2) GenericTilemapDraw(0, pTransDraw, 0);
		if (nSpriteEnable & 1) draw_sprites(0);
		if (nSpriteEnable & 2) draw_sprites(1);
	}
	
	BurnTransferCopy(DrvPalette);

	return 0;
}

static INT32 DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	if (watchdog++ == 60) {
		DrvDoReset();
	}

	{
		memset (DrvInputs, 0xff, 3);
		for (INT32 i = 0; i < 8; i++) {
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
			DrvInputs[2] ^= (DrvJoy3[i] & 1) << i;
		}

		// Clear opposites
		if ((DrvInputs[1] & 0x03) == 0) DrvInputs[1] |= 0x03;
		if ((DrvInputs[1] & 0x0c) == 0) DrvInputs[1] |= 0x0c;
		if ((DrvInputs[2] & 0x03) == 0) DrvInputs[2] |= 0x03;
		if ((DrvInputs[2] & 0x0c) == 0) DrvInputs[2] |= 0x0c;
	}

	konamiNewFrame();
	ZetNewFrame();

	INT32 nCyclesTotal[2] = { (INT32)((double)3000000 * 100 / nBurnFPS), (INT32)((double)3579545 * 100 / nBurnFPS) };
	INT32 nCyclesDone[2] = { nExtraCycles, 0 };
	INT32 nInterleave = 30;

	ZetOpen(0);
	konamiOpen(0);

	for (INT32 i = 0; i < nInterleave; i++) {
		CPU_RUN(0, konami);
		CPU_RUN_TIMER(1);
	}

	konamiSetIrqLine(KONAMI_IRQ_LINE, CPU_IRQSTATUS_AUTO);
	k007121_buffer(0);
	k007121_buffer(1);

	konamiClose();
	ZetClose();

	nExtraCycles = nCyclesDone[0] - nCyclesTotal[0];

	if (pBurnSoundOut) {
		BurnYM3812Update(pBurnSoundOut, nBurnSoundLen);
		K007232Update(0, pBurnSoundOut, nBurnSoundLen);
		K051649Update(pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static INT32 DrvScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029702;
	}

	if (nAction & ACB_VOLATILE) {
		memset(&ba, 0, sizeof(ba));

		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd - AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);

		konamiCpuScan(nAction);
		ZetScan(nAction);

		BurnYM3812Scan(nAction, pnMin);
		K007232Scan(nAction, pnMin);
		K051649Scan(nAction, pnMin);
		k007121_scan(nAction);

		SCAN_VAR(watchdog);

		SCAN_VAR(nExtraCycles);
	}

	if (nAction & ACB_WRITE) {
		konamiOpen(0);
		bankswitch(*nDrvKonBank);
		konamiClose();

		sound_bankswitch(*sound_bank);
	}

	return 0;
}


// Haunted Castle (version M)

static struct BurnRomInfo hcastleRomDesc[] = {
	{ "m03.k12",	0x08000, 0xd85e743d, 1 | BRF_PRG | BRF_ESS }, //  0 Konami Custom Code
	{ "b06.k8",	    0x20000, 0xabd07866, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "768e01.e4",	0x08000, 0xb9fff184, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "768c09.g21",	0x80000, 0xe3be3fdd, 3 | BRF_GRA }, 	      //  3 Bank #0 Tiles
	{ "768c08.g19",	0x80000, 0x9633db8b, 3 | BRF_GRA },           //  4

	{ "768c04.j5",	0x80000, 0x2960680e, 4 | BRF_GRA },           //  5 Bank #1 Tiles
	{ "768c05.j6",	0x80000, 0x65a2f227, 4 | BRF_GRA },           //  6

	{ "768c07.e17",	0x80000, 0x01f9889c, 5 | BRF_SND },           //  7 K007232 Samples

	{ "768c13.j21",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           //  8 Color Proms
	{ "768c14.j22",	0x00100, 0xb32071b7, 6 | BRF_GRA },           //  9
	{ "768c11.i4",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           // 10
	{ "768c10.i3",	0x00100, 0xb32071b7, 6 | BRF_GRA },           // 11

	{ "768b12.d20",	0x00100, 0x362544b8, 0 | BRF_GRA | BRF_OPT }, // 12 Priority Prom
};

STD_ROM_PICK(hcastle)
STD_ROM_FN(hcastle)

struct BurnDriver BurnDrvHcastle = {
	"hcastle", NULL, NULL, NULL, "1988",
	"Haunted Castle (version M)\0", NULL, "Konami", "GX768",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_HISCORE_SUPPORTED, 2, HARDWARE_PREFIX_KONAMI, GBF_SCRFIGHT | GBF_PLATFORM, 0,
	NULL, hcastleRomInfo, hcastleRomName, NULL, NULL, NULL, NULL, HcastleInputInfo, HcastleDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};


// Haunted Castle (version K)

static struct BurnRomInfo hcastlekRomDesc[] = {
	{ "768k03.k12",	0x08000, 0x40ce4f38, 1 | BRF_PRG | BRF_ESS }, //  0 Konami Custom Code
	{ "768g06.k8",	0x20000, 0xcdade920, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "768e01.e4",	0x08000, 0xb9fff184, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "768c09.g21",	0x80000, 0xe3be3fdd, 3 | BRF_GRA }, 	      //  3 Bank #0 Tiles
	{ "768c08.g19",	0x80000, 0x9633db8b, 3 | BRF_GRA },           //  4

	{ "768c04.j5",	0x80000, 0x2960680e, 4 | BRF_GRA },           //  5 Bank #1 Tiles
	{ "768c05.j6",	0x80000, 0x65a2f227, 4 | BRF_GRA },           //  6

	{ "768c07.e17",	0x80000, 0x01f9889c, 5 | BRF_SND },           //  7 K007232 Samples

	{ "768c13.j21",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           //  8 Color Proms
	{ "768c14.j22",	0x00100, 0xb32071b7, 6 | BRF_GRA },           //  9
	{ "768c11.i4",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           // 10
	{ "768c10.i3",	0x00100, 0xb32071b7, 6 | BRF_GRA },           // 11

	{ "768b12.d20",	0x00100, 0x362544b8, 0 | BRF_GRA | BRF_OPT }, // 12 Priority Prom
};

STD_ROM_PICK(hcastlek)
STD_ROM_FN(hcastlek)

struct BurnDriver BurnDrvHcastlek = {
	"hcastlek", "hcastle", NULL, NULL, "1988",
	"Haunted Castle (version K)\0", NULL, "Konami", "GX768",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_HISCORE_SUPPORTED, 2, HARDWARE_PREFIX_KONAMI, GBF_SCRFIGHT | GBF_PLATFORM, 0,
	NULL, hcastlekRomInfo, hcastlekRomName, NULL, NULL, NULL, NULL, HcastleInputInfo, HcastleDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};


// Haunted Castle (version E)

static struct BurnRomInfo hcastleeRomDesc[] = {
	{ "768e03.k12",	0x08000, 0x0b32619c, 1 | BRF_PRG | BRF_ESS }, //  0 Konami Custom Code
	{ "768e06.k8",	0x20000, 0x0431b8c0, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "768e01.e4",	0x08000, 0xb9fff184, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "768c09.g21",	0x80000, 0xe3be3fdd, 3 | BRF_GRA }, 	      //  3 Bank #0 Tiles
	{ "768c08.g19",	0x80000, 0x9633db8b, 3 | BRF_GRA },           //  4

	{ "768c04.j5",	0x80000, 0x2960680e, 4 | BRF_GRA },           //  5 Bank #1 Tiles
	{ "768c05.j6",	0x80000, 0x65a2f227, 4 | BRF_GRA },           //  6

	{ "768c07.e17",	0x80000, 0x01f9889c, 5 | BRF_SND },           //  7 K007232 Samples

	{ "768c13.j21",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           //  8 Color Proms
	{ "768c14.j22",	0x00100, 0xb32071b7, 6 | BRF_GRA },           //  9
	{ "768c11.i4",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           // 10
	{ "768c10.i3",	0x00100, 0xb32071b7, 6 | BRF_GRA },           // 11

	{ "768b12.d20",	0x00100, 0x362544b8, 0 | BRF_GRA | BRF_OPT }, // 12 Priority Prom
};

STD_ROM_PICK(hcastlee)
STD_ROM_FN(hcastlee)

struct BurnDriver BurnDrvHcastlee = {
	"hcastlee", "hcastle", NULL, NULL, "1988",
	"Haunted Castle (version E)\0", NULL, "Konami", "GX768",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_HISCORE_SUPPORTED, 2, HARDWARE_PREFIX_KONAMI, GBF_SCRFIGHT | GBF_PLATFORM, 0,
	NULL, hcastleeRomInfo, hcastleeRomName, NULL, NULL, NULL, NULL, HcastleInputInfo, HcastleDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};


// Akuma-Jou Dracula (Japan version P)

static struct BurnRomInfo akumajouRomDesc[] = {
	{ "768p03.k12",	0x08000, 0xd509e340, 1 | BRF_PRG | BRF_ESS }, //  0 Konami Custom Code
	{ "768j06.k8",	0x20000, 0x42283c3e, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "768e01.e4",	0x08000, 0xb9fff184, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "768c09.g21",	0x80000, 0xe3be3fdd, 3 | BRF_GRA }, 	      //  3 Bank #0 Tiles
	{ "768c08.g19",	0x80000, 0x9633db8b, 3 | BRF_GRA },           //  4

	{ "768c04.j5",	0x80000, 0x2960680e, 4 | BRF_GRA },           //  5 Bank #1 Tiles
	{ "768c05.j6",	0x80000, 0x65a2f227, 4 | BRF_GRA },           //  6

	{ "768c07.e17",	0x80000, 0x01f9889c, 5 | BRF_SND },           //  7 K007232 Samples

	{ "768c13.j21",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           //  8 Color Proms
	{ "768c14.j22",	0x00100, 0xb32071b7, 6 | BRF_GRA },           //  9
	{ "768c11.i4",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           // 10
	{ "768c10.i3",	0x00100, 0xb32071b7, 6 | BRF_GRA },           // 11

	{ "768b12.d20",	0x00100, 0x362544b8, 0 | BRF_GRA | BRF_OPT }, // 12 Priority Prom
};

STD_ROM_PICK(akumajou)
STD_ROM_FN(akumajou)

struct BurnDriver BurnDrvAkumajou = {
	"akumajou", "hcastle", NULL, NULL, "1988",
	"Akuma-Jou Dracula (Japan version P)\0", NULL, "Konami", "GX768",
	L"\u60AA\u9B54\u57CE \u30C9\u30E9\u30AD\u30E5\u30E9 (Japan ver. P)\0Akuma-Jou Dracula\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_HISCORE_SUPPORTED, 2, HARDWARE_PREFIX_KONAMI, GBF_SCRFIGHT | GBF_PLATFORM, 0,
	NULL, akumajouRomInfo, akumajouRomName, NULL, NULL, NULL, NULL, HcastleInputInfo, HcastleDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};


// Akuma-Jou Dracula (Japan version N)

static struct BurnRomInfo akumajounRomDesc[] = {
	{ "768n03.k12",	0x08000, 0x3e4dca2a, 1 | BRF_PRG | BRF_ESS }, //  0 Konami Custom Code
	{ "768j06.k8",	0x20000, 0x42283c3e, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "768e01.e4",	0x08000, 0xb9fff184, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "768c09.g21",	0x80000, 0xe3be3fdd, 3 | BRF_GRA }, 	      //  3 Bank #0 Tiles
	{ "768c08.g19",	0x80000, 0x9633db8b, 3 | BRF_GRA },           //  4

	{ "768c04.j5",	0x80000, 0x2960680e, 4 | BRF_GRA },           //  5 Bank #1 Tiles
	{ "768c05.j6",	0x80000, 0x65a2f227, 4 | BRF_GRA },           //  6

	{ "768c07.e17",	0x80000, 0x01f9889c, 5 | BRF_SND },           //  7 K007232 Samples

	{ "768c13.j21",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           //  8 Color Proms
	{ "768c14.j22",	0x00100, 0xb32071b7, 6 | BRF_GRA },           //  9
	{ "768c11.i4",	0x00100, 0xf5de80cb, 6 | BRF_GRA },           // 10
	{ "768c10.i3",	0x00100, 0xb32071b7, 6 | BRF_GRA },           // 11

	{ "768b12.d20",	0x00100, 0x362544b8, 0 | BRF_GRA | BRF_OPT }, // 12 Priority Prom
};

STD_ROM_PICK(akumajoun)
STD_ROM_FN(akumajoun)

struct BurnDriver BurnDrvAkumajoun = {
	"akumajoun", "hcastle", NULL, NULL, "1988",
	"Akuma-Jou Dracula (Japan version N)\0", NULL, "Konami", "GX768",
	L"\u60AA\u9B54\u57CE \u30C9\u30E9\u30AD\u30E5\u30E9 (Japan ver. N)\0Akuma-Jou Dracula\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_HISCORE_SUPPORTED, 2, HARDWARE_PREFIX_KONAMI, GBF_SCRFIGHT | GBF_PLATFORM, 0,
	NULL, akumajounRomInfo, akumajounRomName, NULL, NULL, NULL, NULL, HcastleInputInfo, HcastleDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x1000,
	256, 224, 4, 3
};
