#include "t_ks0108.h"

#include <avr/io.h>
#include <util/delay.h>

void t_ks0108::init(void)
{
	//Set pins as outputs
	DDR(RS_PORT) |= (1 << DD(RS_PORT, RS_PIN));
	DDR(RW_PORT) |= (1 << DD(RW_PORT, RW_PIN));
	DDR(E_PORT) |= (1 << DD(E_PORT, E_PIN));
	DDR(CS1_PORT) |= (1 << DD(CS1_PORT, CS1_PIN));
	DDR(CS2_PORT) |= (1 << DD(CS2_PORT, CS2_PIN));
	DDR(RESET_PORT) |= (1 << DD(RESET_PORT, RESET_PIN));
	DDR(DATA_PORT) = 255;

	//Make sure pins are 0
	CLEARPIN(RS_PORT, RS_PIN);
	CLEARPIN(RW_PORT, RW_PIN);
	CLEARPIN(E_PORT, E_PIN);
	CLEARPIN(CS1_PORT, CS1_PIN);
	CLEARPIN(CS2_PORT, CS2_PIN);
	CLEARPIN(RESET_PORT, RESET_PIN);
	PORT(DATA_PORT) = 0;
	
	//Blink reset
	SETPIN(RESET_PORT, RESET_PIN);
	_delay_ms(100);
	CLEARPIN(RESET_PORT, RESET_PIN);
	_delay_ms(100);
	SETPIN(RESET_PORT, RESET_PIN);
	_delay_ms(100);

	selectSide(LEFT);
	writeInstruction(DISPLAY_OFF);
	writeInstruction(START_LINE);
	writeInstruction(X_ADRESS);
	writeInstruction(Y_ADRESS);
	writeInstruction(DISPLAY_ON);
	
	selectSide(RIGHT);
	writeInstruction(DISPLAY_OFF);
	writeInstruction(START_LINE);
	writeInstruction(X_ADRESS);
	writeInstruction(Y_ADRESS);
	writeInstruction(DISPLAY_ON);
	
	clearScreen();
	posx = posy = 0;
}

void t_ks0108::selectSide(e_side side)
{
	CLEARPIN(RS_PORT, RS_PIN);
	CLEARPIN(E_PORT, E_PIN);
	SETPIN(RW_PORT, RW_PIN);
	if(side == LEFT)
	{
		CLEARPIN(CS2_PORT, CS2_PIN);
		SETPIN(CS1_PORT, CS1_PIN);
	}
	else if(side == RIGHT)
	{
		CLEARPIN(CS1_PORT, CS1_PIN);
		SETPIN(CS2_PORT, CS2_PIN);
	}
	writeInstruction(X_ADRESS);
}

void t_ks0108::writeInstruction(uint8_t instruction)
{
	waitBusy();
	CLEARPIN(RS_PORT, RS_PIN);
	CLEARPIN(RW_PORT, RW_PIN);
	
	PORT(DATA_PORT) = instruction;
	
	SETPIN(E_PORT, E_PIN);
	_delay_ms(1);
	CLEARPIN(E_PORT, E_PIN);
}

void t_ks0108::writeData(uint8_t data)
{
	waitBusy();
	SETPIN(RS_PORT, RS_PIN);
	CLEARPIN(RW_PORT, RW_PIN);
	
	PORT(DATA_PORT) = data;
	
	SETPIN(E_PORT, E_PIN);
	_delay_ms(1);
	CLEARPIN(E_PORT, E_PIN);
}

void t_ks0108::writeString(char *string)
{
	while(*string != 0)
	{
		putChar(*string);
		++string;
	}
}

void t_ks0108::putChar(uint8_t ch)
{
	const uint8_t charWidth = 5;
	uint8_t rightSide = 0;
	if(posx > 128 - charWidth)
	{
		posx = 0;
		++posy;
		if(posy == 8)
			posy = 0;
		
	}
	if(posx < 64)
	{
		selectSide(LEFT);
	}
	else
	{
		selectSide(RIGHT);
		rightSide = 64;
	}

	writeInstruction(X_ADRESS + posx - rightSide);
	writeInstruction(Y_ADRESS + posy);

	for(uint8_t i = 0; i != 5; ++i)
	{
		if(posx == 64)
		{
			selectSide(RIGHT);
			writeInstruction(Y_ADRESS + posy);
		}
		writeData(pgm_read_byte(&System5x7[((ch - 32)*5)+i]));
		++posx;
	}
	if(posx != 128)
	{
		writeData(0x00); //One space between letters
		++posx;
	}
}

void t_ks0108::waitBusy(void)
{
	DDRD = 0;
	CLEARPIN(RS_PORT, RS_PIN);
	SETPIN(RW_PORT, RW_PIN);

	SETPIN(E_PORT, E_PIN);
	_delay_ms(1);
	CLEARPIN(E_PORT, E_PIN);

	while((0x80 & PIND))
		asm volatile ( "nop" );
	DDRD = 255;
	return;
}

void t_ks0108::gotoXY(uint8_t x, uint8_t y)
{
	posx = x;
	posy = y;
}

void t_ks0108::clearScreen(void)
{
	clearSide(RIGHT);
	clearSide(LEFT);
	posx = posy = 0;
}

void t_ks0108::clearSide(e_side side)
{
	selectSide(side);
	for(uint8_t i = 0; i != 8; ++i)
	{
		writeInstruction(Y_ADRESS | i);
		writeInstruction(X_ADRESS);
		for(uint8_t t = 0; t != 64; ++t)
			writeData(0x00);
	}
}
