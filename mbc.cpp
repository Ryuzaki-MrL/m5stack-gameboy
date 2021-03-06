#include "mbc.h"
#include "rom.h"
#include "sdl.h"

#define SET_ROM_BANK(n)		(rombank = &rom[((n) & (rom_banks - 1)) * 0x4000])
#define SET_RAM_BANK(n)		(rambank = &ram[((n) & (ram_banks - 1)) * 0x2000])

static unsigned int curr_rom_bank = 1;
static unsigned char rom_banks;
static unsigned int curr_ram_bank = 0;
static unsigned char ram_banks;
static bool ram_select;
static bool ram_enabled;
static const unsigned char *rom;
static unsigned char *ram;
const unsigned char *rombank;
unsigned char *rambank;
static const s_rominfo *rominfo;

MBCReader mbc_read_rom;
MBCReader mbc_read_ram;
MBCWriter mbc_write_rom;
MBCWriter mbc_write_ram;


void mbc_init()
{
	rom = rom_getbytes();
	rominfo = rom_get_info();
	
	rom_banks = rominfo->rom_banks;
	ram_banks = rominfo->ram_banks;
	
	int ram_size = rom_get_ram_size();
	ram = (unsigned char *)calloc(1, ram_size < 1024*8 ? 1024*8 : ram_size);
	
	if (rominfo->has_battery && ram_size)
		sdl_load_sram(ram, ram_size);
	
	SET_ROM_BANK(1);
	SET_RAM_BANK(0);
	
	switch(rominfo->rom_mapper)
	{
		case MBC2:
			mbc_write_rom = MBC3_write_ROM;
			mbc_write_ram = MBC1_write_RAM;
			mbc_read_ram = MBC1_read_RAM;
		break;
		case MBC3:
			mbc_write_rom = MBC3_write_ROM;
			mbc_write_ram = MBC3_write_RAM;
			mbc_read_ram = MBC3_read_RAM;
		break;
		default:
			mbc_write_rom = MBC1_write_ROM;
			mbc_write_ram = MBC1_write_RAM;
			mbc_read_ram = MBC1_read_RAM;
		break;
	}
}

unsigned char* mbc_get_ram()
{
	return ram;
}


/* Unfinished, no clock etc */
void MBC3_write_ROM(unsigned short d, unsigned char i)
{
	if(d < 0x2000)
		ram_enabled = ((i & 0x0F) == 0x0A);
	
	else if(d < 0x4000) {
		curr_rom_bank = i & 0x7F;

		if(curr_rom_bank == 0)
			curr_rom_bank++;

		SET_ROM_BANK(curr_rom_bank);
	}
	
	else if(d < 0x6000) {
		// TODO: select RTC
		curr_ram_bank = i & 0x07;
		SET_RAM_BANK(curr_ram_bank);
	}
}

void MBC3_write_RAM(unsigned short d, unsigned char i)
{
	// TODO: write to RTC
	if (!ram_enabled)
		return;
	rambank[d - 0xA000] = i;
	//sram_modified = true;
}

unsigned char MBC3_read_RAM(unsigned short i)
{
	return ram_enabled ? rambank[i - 0xA000] : 0xFF;
}


void MBC1_write_ROM(unsigned short d, unsigned char i)
{
	if(d < 0x2000)
		ram_enabled = ((i & 0x0F) == 0x0A);
	
	else if(d < 0x4000) {
		curr_rom_bank = (curr_rom_bank & 0x60) | (i & 0x1F);

		if(curr_rom_bank == 0 || curr_rom_bank == 0x20 || curr_rom_bank == 0x40 || curr_rom_bank == 0x60)
			curr_rom_bank++;

		SET_ROM_BANK(curr_rom_bank);
	}
	
	else if(d < 0x6000) {
		if (ram_select) {
			curr_ram_bank = i&3;
			SET_RAM_BANK(curr_ram_bank);
		}
		else {
			curr_rom_bank = ((i & 0x3)<<5) | (curr_rom_bank & 0x1F);
			SET_ROM_BANK(curr_rom_bank);
		}
	}
	
	else if(d < 0x8000) {
		ram_select = i&1;
		if (ram_select) {
			curr_rom_bank &= 0x1F;
			SET_ROM_BANK(curr_rom_bank);
		}
		else {
			curr_ram_bank = 0;
			SET_RAM_BANK(curr_ram_bank);
		}
	}
}

void MBC1_write_RAM(unsigned short d, unsigned char i)
{
	if (!ram_enabled)
		return;
	rambank[d - 0xA000] = i;
	//sram_modified = true;
}

unsigned char MBC1_read_RAM(unsigned short i)
{
	return ram_enabled ? rambank[i - 0xA000] : 0xFF;
}
