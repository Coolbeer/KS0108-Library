#include "t_ks0108.h"

t_ks0108 lcd;

int main(void)
{
	lcd.init();

	lcd.gotoXY(20, 3);
	lcd.writeString((char*)"ks0108 library");

	lcd.gotoXY(20, 4);
	lcd.writeString((char*)"Some text here");
	while(true)
	{
		asm volatile ( "nop" );
	}
	return 0;
}
