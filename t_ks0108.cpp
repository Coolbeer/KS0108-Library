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
    writeInstruction(DISPLAY_ON);
    
    selectSide(RIGHT);
    writeInstruction(DISPLAY_OFF);
    writeInstruction(START_LINE);
    writeInstruction(DISPLAY_ON);

    clearScreen();
    gotoXY(0,0);
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
    _delay_us(10);
    CLEARPIN(E_PORT, E_PIN);
}

void t_ks0108::writeData(uint8_t data)
{
    waitBusy();
    SETPIN(RS_PORT, RS_PIN);
    CLEARPIN(RW_PORT, RW_PIN);
    
    PORT(DATA_PORT) = data;
    
    SETPIN(E_PORT, E_PIN);
    _delay_us(10);
    CLEARPIN(E_PORT, E_PIN);
}

void t_ks0108::writeString(char *string, text_modifiers mod)
{
    while(*string != 0)
    {
        putChar(*string, mod);
        ++string;
    }
}

void t_ks0108::putChar(uint8_t ch, text_modifiers mod)
{
    const uint8_t charWidth = 5;
    uint8_t mods = 0;
    if(mod == UNDERLINE)
        mods = 0x80;
    if(posx > 128 - charWidth)
    {
        posx = 0;
        ++posy;
        if(posy == 8)
            posy = 0;
        
    }
    gotoXY(posx, posy);
    for(uint8_t i = 0; i != 5; ++i)
    {
        if(posx == 64)
            gotoXY(posx, posy);
        writeData(pgm_read_byte(&System5x7[((ch - 32)*5)+i])|mods);
        ++posx;
    }
    if(posx != 128)
    {
        writeData(0x00|mods); //One space between letters
        ++posx;
    }
}

void t_ks0108::waitBusy(void)
{
    DDR(DATA_PORT) = 0;
    CLEARPIN(RS_PORT, RS_PIN);
    SETPIN(RW_PORT, RW_PIN);

    SETPIN(E_PORT, E_PIN);
    _delay_us(10);
    CLEARPIN(E_PORT, E_PIN);

    while(0x80 & PIN(DATA_PORT))
        asm volatile ( "nop" );

    DDR(DATA_PORT) = 255;
    return;
}

void t_ks0108::gotoXY(uint8_t x, uint8_t y)
{
    uint8_t rOffset = 0;
    posx = x;
    posy = y;
    if(posx >= 64)
    {
        selectSide(RIGHT);
        rOffset = 64;
    }
    else
        selectSide(LEFT);

    writeInstruction(X_ADRESS + posx - rOffset); 
    writeInstruction(Y_ADRESS + y);
}

void t_ks0108::drawRectangle(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY)
{
    //NOTE: This function does not preserve any data, rows startY and endY will be wiped.
    //Sanitycheck
    if(startX >= 128 || endX >= 128 || startY >= 8 || endY >= 8)
        return;
    
    //Flip start and end if start > end
    if(startX > endX)
    {
        uint8_t temp = startX;
        startX = endX;
        endX = temp;
    }
    if(startY > endY)
    {
        uint8_t temp = startY;
        startY = endY;
        endY = temp;
    }

    drawHorizontalLine(startX, startY, 0x00, endX - startX);
    drawHorizontalLine(startX, endY, 7, endX - startX);
    
    for(uint8_t i = startY; i != endY +1; ++i)
    {
        gotoXY(startX, i);
        writeData(0xFF);
        gotoXY(endX, i);
        writeData(0xFF);
    }
}

void t_ks0108::drawHorizontalLine(uint8_t startX, uint8_t page, uint8_t bit, uint8_t length)
{
    //Sanitycheck
    if(startX >= 128 || page >= 8 || bit >= 8 || startX+length >= 128)
        return;

    gotoXY(startX, page);
    for(uint8_t i = startX; i != startX + length; ++i)
    {
        if(i == 64)
            gotoXY(i, page);
        writeData(1 << bit);
    }
}

uint8_t t_ks0108::readData(uint8_t x, uint8_t y)
{
    //Sanitycheck
    if(x >= 128 || y >= 8)
        return 0;

    //Goto destination
    gotoXY(x, y);

    //Wait til LCD is ready
    waitBusy();

    //Put data port in input mode
    DDR(DATA_PORT) = 0;

    //Both high = Data Read command
    SETPIN(RS_PORT, RS_PIN);
    SETPIN(RW_PORT, RW_PIN);
    
    //Send command, this is a dummy read
    SETPIN(E_PORT, E_PIN);
    _delay_us(10);
    CLEARPIN(E_PORT, E_PIN);

    //Send command again
    SETPIN(E_PORT, E_PIN);
    _delay_us(10);
    CLEARPIN(E_PORT, E_PIN);

    //And here is our data
    uint8_t retValue = PIN(DATA_PORT);

    //Data port is output again
    DDR(DATA_PORT) = 255;
    return retValue;
}

void t_ks0108::setBit(uint8_t x, uint8_t y, uint8_t bit)
{
    //Sanitycheck
    if(x >= 128 || y >= 8 || bit >= 8)
        return;
    
    uint8_t data = readData(x, y);
    data |= (1 << bit);

    gotoXY(x, y);
    writeData(data);
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
        writeInstruction(Y_ADRESS + i);
        for(uint8_t t = 0; t != 64; ++t)
            writeData(0x00);
    }
}
