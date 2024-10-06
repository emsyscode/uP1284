/*
 * atmega1284p-LED.c
 *
 * Created: 3/31/2018 10:44:45 PM
 * Author : jlsmo
  
// this is a clock but now with a large VFD of 16 grids by 16 segm
// the 6315 don't support all, but is only to test VFD tube

Purpose: This is a for loop that blinks an LED on PORTC
*/

//set your clock speed
#define F_CPU 16000000UL
//these are the include files. They are outside the project folder
#include <avr/io.h>
//#include <iom1284p.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* Port controls  (Platform dependent) */
//#define CS_LOW()	PORTB &= ~1			/* CS=low */
//#define	CS_HIGH()	PORTB |= 1			/* CS=high */

//#define LCD_Port PORTB
#define VFD_stb PINC5 // Must be pulsed to LCD fetch data of bus
#define VFD_in PINC0// If 0 write LCD, if 1 read of LCD
#define VFD_clk PINC1 // if 0 is a command, if 1 is a data0

#define delay_tcsh _delay_us(16)


typedef int boolean;
#define true 1
#define false 0
#define L ((1<<14)-1)
#define R ((1<<12)-1)

//#define SegPins		PORTB

//#define Grid	PORTD
#define AdjustPins		PIND // before is C, but I'm use port C to VFC Controle signals
#define SegCntrlDDR		DDRD

#define BIN(x) \
( ((0x##x##L & 0x00000001L) ? 0x01 : 0) \
| ((0x##x##L & 0x00000010L) ? 0x02 : 0) \
| ((0x##x##L & 0x00000100L) ? 0x04 : 0) \
| ((0x##x##L & 0x00001000L) ? 0x08 : 0) \
| ((0x##x##L & 0x00010000L) ? 0x10 : 0) \
| ((0x##x##L & 0x00100000L) ? 0x20 : 0) \
| ((0x##x##L & 0x01000000L) ? 0x40 : 0) \
| ((0x##x##L & 0x10000000L) ? 0x80 : 0))

//this include is in your project folder

/*Function Declarations*/
/*****************************************************************************/
/*Decimal Digit (0-9) to Seven Segment Values Encoder*/
unsigned char DigitTo7SegEncoder(unsigned char digit, unsigned char common);


/*Global Variables Declarations*/
unsigned char day = 7;  // start at 7 because the VFD start the day on the left side and move to rigth... grid is reverse way
unsigned char hours = 0;
unsigned char minutes = 0;
unsigned char seconds = 0;
unsigned char milisec = 0;
unsigned char points = 0;
unsigned char SegLSB =0;
unsigned char SegMSB = 0;

void delay_ms(uint16_t ms) {
	//while loop that creates a delay for the duration of the millisecond countdown
	while ( ms )
	{
		_delay_ms(1);
		ms--;
	}
}

void __delay_ms(int x)
{
	int y = (x*1000)/12;
	while(--y != 0)
	continue;
}

void __delay_us(int x)
{
	int y = x/12; // Note: x must be bigger at least 12
	while(--y != 0)
	continue;
}

//###################################################
void AD16315_send_cmd(unsigned char a)
{
 // This send use the strob... good for send commands, not burst of data.	
	unsigned char chr;
	chr = a;
	
	PORTC =(0 << VFD_clk);
	delay_ms(1);//Delay
	PORTC =(0 << VFD_in);
	delay_ms(1);//Delay
	PORTC =(0 << VFD_stb);
	delay_ms(1);
	for (int i=0; i<8; i++) // 8 bit 0-7 // aqui inverti para ficar com 0x88 correccto
	{
		PINC = (1 << VFD_in);
		delay_ms(1);
		PINC = (1 << VFD_clk); // O (^)XOR logo só muda se for diferente de "1 1" e "0 0"
		delay_ms(1);
		PINC = (0 << VFD_clk);
		delay_ms(1);
		PORTC = (0 << VFD_in);
		delay_ms(1);
	}
	PORTC = (1 << VFD_stb) | (0 << VFD_clk) | (1 << VFD_in);
	delay_ms(10);
}

serial_test(unsigned char a)
{
	for (int i=0; i<8; i++) // 8 bit 0-7 // aqui inverti para ficar com 0x88 correccto
	{
		PORTC =a;
		delay_ms(25);//Delay
		a=a << 1;
		
	}
	return;
}

 test(unsigned char a)
{
	for (int i=0; i<8; i++) // 8 bit 0-7 // aqui inverti para ficar com 0x88 correccto
	{
		// Note-se que estou a mudar o pin em vez do port, caso contrario ele volta a colocar os dados IN em low quando
		// moda o PORTC e surge o impulso de clock... assume a reconfiguração do porto inteiro!!!!
		PINC = (1 << VFD_in);
		delay_ms(1);
		PINC = (1 << VFD_clk); // XOR logo só muda se for diferente de "1 1" e "0 0"
		delay_ms(1);
		PINC = (0 << VFD_clk);
		delay_ms(1);
		PINC = (0 << VFD_in);
		delay_ms(1);
		
	}
}

void StringToHex(unsigned char* string, unsigned char* hexstring)
{
	unsigned char ch,i,j,len;
	
	len = strlen(string);
	
	for(i=0,j=0;i<len;i++,j+=2)
	{
		ch = string[i];
		
		if( ch >= 0 && ch <= 0x0F)
		{
			hexstring[j] = 0x30;
			
			if(ch >= 0 && ch <= 9)
			hexstring[j+1] = 0x30 + ch;
			else
			hexstring[j+1] = 0x37 + ch;
		}
		else  if( ch >= 0x10 && ch <= 0x1F)
		{
			hexstring[j] = 0x31;
			ch -= 0x10;
			
			if(ch >= 0 && ch <= 9)
			hexstring[j+1] = 0x30 + ch;
			else
			hexstring[j+1] = 0x37 + ch;
		}
		else  if( ch >= 0x20 && ch <= 0x2F)
		{
			hexstring[j] = 0x32;
			ch -= 0x20;
			
			if(ch >= 0 && ch <= 9)
			hexstring[j+1] = 0x30 + ch;
			else
			hexstring[j+1] = 0x37 + ch;
		}
		else  if( ch >= 0x30 && ch <= 0x3F)
		{
			hexstring[j] = 0x33;
			ch -= 0x30;
			
			if(ch >= 0 && ch <= 9)
			hexstring[j+1] = 0x30 + ch;
			else
			hexstring[j+1] = 0x37 + ch;
		}
		else  if( ch >= 0x40 && ch <= 0x4F)
		{
			hexstring[j] = 0x34;
			ch -= 0x40;
			
			if(ch >= 0 && ch <= 9)
			hexstring[j+1] = 0x30 + ch;
			else
			hexstring[j+1] = 0x37 + ch;
		}
		else  if( ch >= 0x50 && ch <= 0x5F)
		{
			hexstring[j] = 0x35;
			ch -= 0x50;
			
			if(ch >= 0 && ch <= 9)
			hexstring[j+1] = 0x30 + ch;
			else
			hexstring[j+1] = 0x37 + ch;
		}
		else  if( ch >= 0x60 && ch <= 0x6F)
		{
			hexstring[j] = 0x36;
			ch -= 0x60;
			
			if(ch >= 0 && ch <= 9)
			hexstring[j+1] = 0x30 + ch;
			else
			hexstring[j+1] = 0x37 + ch;
		}
		else  if( ch >= 0x70 && ch <= 0x7F)
		{
			hexstring[j] = 0x37;
			ch -= 0x70;
			
			if(ch >= 0 && ch <= 9)
			hexstring[j+1] = 0x30 + ch;
			else
			hexstring[j+1] = 0x37 + ch;
		}
	}
	hexstring[j] = 0x00;
}

void send_data(unsigned char a)
{
	unsigned char transmit = 7; //define our transmit pin
	unsigned char data = 170; //value to transmit, binary 10101010
	unsigned char mask = 1; //our bitmask
	unsigned char bitDelay = 100;
	
	data=a;
	//This don't send the strobe signal, to be used in burst data send
	 for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
		 PINC = (0 << VFD_clk);
		 if (data & mask){ // if bitwise AND resolves to true
			 PORTC = (1 << VFD_in);
		 }
		 else{ //if bitwise and resolves to false
			 PORTC = (0 << VFD_in); // send 0
		 }
		  _delay_us(48); //delay
		 PINC = (1 << VFD_clk);
		 _delay_us(48); //delay
		 PINC &= ~(0 << VFD_clk);
		  _delay_us(48); //delay
	 }
}
void send(unsigned char a)
{
	 unsigned char transmit = 7; //define our transmit pin
	 unsigned char data = 170; //value to transmit, binary 10101010
	 unsigned char mask = 1; //our bitmask
	 unsigned char bitDelay = 100;
	
	data=a;
	PINC &= ~(1 << VFD_stb);
	 _delay_us(48);
	for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
		PINC = (0 << VFD_clk);
		if (data & mask){ // if bitwise AND resolves to true
			PORTC = (1 << VFD_in);
		}
		else{ //if bitwise and resolves to false
			PORTC = (0 << VFD_in); // send 0
		}
		 _delay_us(48); //delay
		PINC = (1 << VFD_clk);
		 _delay_us(48); //delay
		PINC &= ~(0 << VFD_clk);
		 _delay_us(48); //delay
	}
	PINC = (1 << VFD_stb);
	 _delay_us(48);
}

void pt6315_init(void)
{
	delay_ms(200); //power_up delay

	// Configure VFD display (grids)
	send(0b00001000);//  (0b01000000)    cmd2 12 grids 16 segm
	delay_tcsh; // 4/16 4 grids & 16 segments
	// Write to memory display, increment address, normal operation
	send(0b01000000);//(BIN(01000000)); //(BIN(01000000)));//  (0b01000000)
	delay_tcsh;
	// Address 00H - 15H ( total of 11*2Bytes=176 Bits)
	send(0b11000000);//(BIN(01100110)); //(BIN(01100110)));
	delay_tcsh;
	// set DIMM/PWM to value
	send((0b10001000) | 7);//0 min - 7 max  )(0b01010000)
	delay_tcsh;
}
void clear(void)
{
	/*
	Here I clean all registers 
	Could be done only on the number of grid
	to be more fast. The 12 * 3 bytes = 36 registers
	*/
	send(0b01000000); // cmd 2
	PINC &= ~(1 << VFD_stb);
	 __delay_us(48);
	send_data(0b11000000); //cmd 3
	
	for (int i=0; i<36; i++){
	send_data(0b00000000); // data
	}
	
	PINC = (1 << VFD_stb);
	 __delay_us(48);
}

void test_VFD(void)
{
	/* 
	Here do a test for all segments of 6 grids
	each grid is controlled by a group of 3 bytes
	by these reason I'm send a burst of 3 bytes of
	data. The cycle for do a increment of 3 bytes on 
	the variable "i" on each test cycle of FOR.
	*/
	
	// to test 6 grids is 6*3=18, the 8 gird result in 8*3=24.
for (int i =0; i< 36; i=i+3){
	//send(0b00000010); // cmd 1
	send(0b01000000); // cmd 2
	PINC &= ~(1 << VFD_stb);
	 __delay_us(48);
	send_data((0b11000000) | i); //cmd 3 Each grid use 3 bytes here define the 1º, 2º, 3º until max address

	send_data(0b11111111); // data
	send_data(0b11111111); // data
	send_data(0b11111111); // data

	PINC = (1 << VFD_stb);
	 __delay_us(48);

	send(0b00001000); // cmd1 Here I define the 8 grids and 20 Segments
	send((0b10001000) | 7); //cmd 4
	 __delay_us(48);
}
}
	
	
	void send_digit(unsigned char value7seg)
{
	/* 
	Here do a test for all segments of 6 grids
	each grid is controlled by a group of 3 bytes
	by these reason I'm send a burst of 3 bytes of
	data. The cycle for do a increment of 3 bytes on 
	the variable "i" on each test cycle of FOR.
	*/
for (int i =0; i< 36; i=i+3){
	//send(0b00000010); // cmd 1
	send(0b01000000); // cmd 2
	PINC &= ~(1 << VFD_stb);
	 __delay_us(48);
	send_data((0b11000000) | i); //cmd 3 Each grid use 3 bytes here define the 1º, 2º, 3º until max address

	send_data(value7seg); // data

	PINC = (1 << VFD_stb);
	 __delay_us(48);

	send(0b00001000); // cmd1 Here I define the 8 grids and 20 seg
	send((0b10001000) | 7); //cmd 4
	 __delay_us(48);
}
}

void send_clock_digits(unsigned char value7seg, unsigned char gridNumber)
{
	//send(0b00000010); // cmd 1
	send(0b01000000); // cmd 2
	PINC &= ~(1 << VFD_stb);
	 _delay_us(48);
	send_data((0b11000000) | gridNumber); //cmd 3 Each grid use 3 bytes here define the 1º, 2º, 3º until max address

	send_data(SegLSB); // data
	send_data(SegMSB); // data

	PINC = (1 << VFD_stb);
	 _delay_us(48);
	send(0b00001000); // cmd1 Here I define the 8 grids and 20seg
	send((0b10001000) | 7); //cmd 4
	 _delay_us(48);
}
/*
* common - Common Anode (0), Common Cathode(1)
* SegLSB - Encoded Seven Segment Value
*
* Connections:
* return value to 7 segments digits (a,b,c,d,e,f,g and point case use it)
* significant bit A is (bit 0) and G is bit 6, point bit 7.
*
* remember I if use a vfd tube with the segment of meaddle broke in tow segments...MSB is also used
*/
 unsigned char DigitTo7SegEncoder( unsigned char digit, unsigned char common)
{
	
	switch(digit)
	{
		case 0:	if(common == 1){	SegLSB = 0b01100011;
									SegMSB = 0b11000110;}
		else			SegLSB = ~0b00111111;
		break;
		case 1:	if(common == 1){	SegLSB = 0b00100000;
									SegMSB = 0b00000010;}
		else			SegLSB = ~0b00000110;
		break;
		case 2:	if(common == 1){	SegLSB = 0b10100011;
									SegMSB = 0b11000101;}
		else			SegLSB = ~0b01011011;
		break;
		case 3:	if(common == 1){	SegLSB = 0b10100011;
									SegMSB = 0b11000011;}
		else			SegLSB = ~0b01001111;
		break;
		case 4:	if(common == 1)	{	SegLSB = 0b11100000;
									SegMSB = 0b00000011;}
		else			SegLSB = ~0b01100110;
		break;
		case 5:	if(common == 1)	{	SegLSB = 0b11000011;
									SegMSB = 0b11000011;}
		else			SegLSB = ~0b01101101;
		break;
		case 6:	if(common == 1)	{	SegLSB = 0b11000011;
									SegMSB = 0b11000111;}
		else			SegLSB = ~0b01111101;
		break;
		case 7:	if(common == 1)	{	SegLSB = 0b00100011;
									SegMSB = 0b00000010;}
		else			SegLSB = ~0b00000111;
		break;
		case 8:	if(common == 1)	{	SegLSB = 0b11100011;
									SegMSB = 0b11000111;}
		else			SegLSB = ~0b01111111;
		break;
		case 9:	if(common == 1)	{	SegLSB = 0b11100011;
									SegMSB = 0b00000011;}
		else			SegLSB = ~0b01101111;
	}
	
}	

ISR(TIMER1_COMPA_vect)
{
	seconds++;
	send_clock_points(); //Blinks two points meadle hours
	
		if(seconds == 60)
		{
			seconds = 0;
			minutes++;
		}
		if(minutes == 60) //60
		{
			minutes = 0;
			hours++;
		}
		if(hours > 23)
		{
			hours = 0;
			day--;
			if (day <=0 )
			day=7;
		}
}
send_clock_points(void)
{
	unsigned char grid_number;
	grid_number =0x1B;
	

	//Block to blink 2 points of seconds
	if (points == 0 )
	{
		send_clock_digits(0b00010000, grid_number);
		points =1;
	}
	else
	{
		send_clock_digits(0b00000000, grid_number);
		points =0;
	}
	//
	send_update_clock();
}

send_update_clock()
{
	unsigned char Digit;
	unsigned char number, DisplayCommonPin;
	unsigned char Grid;
	number= 0x07;
	DisplayCommonPin = 1;
	
		/* Reset Seconds to 00*/
		if((AdjustPins & 0x10) == 0 )
		{
			_delay_ms(100);
			seconds=00;
		}
		
		/* Set Minutes when SegCntrl Pin 6 Switch is Pressed*/
		if((AdjustPins & 0x20) == 0 )
		{
			_delay_ms(100);
			if(minutes < 59)
			minutes++;
			else
			minutes = 0;
		}
		/* Set Hours when SegCntrl Pin 7 Switch is Pressed*/
		if((AdjustPins & 0x40) == 0 )
		{
			_delay_ms(100);
			if(hours < 23)
			hours++;
			else
			hours = 0;
		}
		
		/* Set Days when SegCntrl Pin 7 Switch is Pressed*/
		if((AdjustPins & 0x80) == 0 )
		{
			_delay_ms(100);
			if(day < 7 )
			day++;
			else
			day = 1;
		}
		/*
		//*****Only necessary if you want send miliseconds ********
		//SegPins = DigitTo7SegEncoder(milisec%10,1);
		Grid = 0x00;
		send_clock_digits(0b00000000 , Grid);
		//SegPins = DigitTo7SegEncoder(milisec/10,1);
		Grid = 0x03;
		send_clock_digits(0b00000000, Grid);
		*/
		
		
		//********** Only to update day of week ******/
		/*
		Grid = 0x00;
		if (day==1) // tuhsday
		{
			send_clock_digits((0x00 | 0b10000000), Grid);
		}
		else
		{
			send_clock_digits((0x00 | 0b00000000), Grid);
		}
		Grid = 0x03;
		if (day==2) // tuhsday
		{
			send_clock_digits((0x00 | 0b10000000), Grid);
		}
		else
		{
			send_clock_digits((0x00 | 0b00000000), Grid);
		}
		//********************************************/
		
		 DigitTo7SegEncoder(seconds%10,1);
		Grid = 0x09;
		if (day==9)  // thusday not valid day to skip it
		{
		send_clock_digits((SegLSB), Grid);  // grid small, without day segment
		send_clock_digits((SegMSB), Grid);  // grid small, without day segment
		}
		else
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		
		 DigitTo7SegEncoder(seconds/10,1);
		Grid = 0x0C;
		if (day==3) // wethsday
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		else
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		//*************************************************************
		 DigitTo7SegEncoder(minutes%10,1);
		Grid = 0x0F;
		if (day==4) // tuhsday
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		else
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		
		DigitTo7SegEncoder(minutes/10,1);
		Grid = 0x12;
		if (day==5) // monday
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		else
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		//**************************************************************
		DigitTo7SegEncoder(hours%10,1);
		Grid = 0x15;
		if (day==6) // sunday
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		else
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		
		 DigitTo7SegEncoder(hours/10,1);
		Grid = 0x18;
		if (day==7) // sunday
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		else
		{
		send_clock_digits((SegLSB), Grid);
		send_clock_digits((SegMSB), Grid);
		}
		//**************************************************************
}
int main (void)
{
	unsigned char a,b;
	
	seconds = 0x00;
	minutes =0x00;
	hours = 0x00;
	
	TCCR1B = (1<<CS12|1<<WGM12);
	OCR1A = 32760-1;  //32768-1;
	TIMSK1 = 1<<OCIE1A;
	
	a=0x33;
	b=0x01;

CLKPR=(0x80);
//Set PORT
DDRC = 0xFF;
PORTC=0x00;
DDRA = 0xFF;
PORTA=0x00;
DDRD = 0x00;
PORTD=0x00;
pt6315_init();
clear();
test_VFD();
delay_ms(1400);
clear();
test_VFD();
delay_ms(1400);
clear();
//only here I active the enable to allow run the test of VFD
sei();
//create an infinite loop
while(1) {

}
}

