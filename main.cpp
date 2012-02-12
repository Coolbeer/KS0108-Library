#include "t_ks0108.h"

t_ks0108 lcd;

int main(void)
{
    lcd.init();

    lcd.gotoXY(20, 3);
    lcd.writeString((char*)"ks0108 library");

    lcd.gotoXY(20, 4);
    lcd.writeString((char*)"Some text here");
    
    for(uint8_t i = 0; i != 127; ++i)
    {
        lcd.setBit(i, 3, 7);
    }
    while(true)
    {
        asm volatile ( "nop" );
    }
    return 0;
}
