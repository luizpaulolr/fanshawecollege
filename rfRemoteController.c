		//--- Heating PAD Remote Controller ---
/*----------------------------------------------------------------------------------------------
	File Name:	heatingPadRemoteController.c
	Author:		LRIBEIRO
	Date:		16/07/2019
	Modified:	None
	Copyright 	Fanshawe College, 2019

	Description: 	This program remotely controls a RF Heating PAD using predefined commands
					received from a Bluetooth Android App connected in the Serial Port 1.

----------------------------------------------------------------------------------------------*/

// Preprocessor ------------------------------------------------------------------------------

#include "pragmas.h"
#include <stdlib.h>
#include <delays.h>
#include <stdio.h>
#include <string.h>

// Constants ---------------------------------------------------------------------------------

#define TRUE			1
#define	FALSE			0

#define LEFTSIDE		0
#define RIGHTSIDE		1

// Command Index
#define LEFTONSIGNAL	0
#define LEFTOFFSIGNAL	1
#define RIGHTONSIGNAL	2
#define RIGHTOFFSIGNAL	3
	
#define UPDCONTROL		0
#define SETPOWER		1

// Index for 24 -> 12 hours convertion
#define HRSINDEX		0
#define ISPMINDEX		1

// RF Module Data Pin
#define DATA1OUT		LATDbits.LATD0

#define BTH_CONNECTED	PORTDbits.RD2

// Buffer sizes
#define ONSIGNALSIZE		94
#define OFFSIGNALSIZE		31
#define BUFSIZE				80
#define TOKENSIZE			20

// TOKENS POSITIONS UPDADE MSG
#define TKCMDSTM			0
#define TKCURRENTHRS		1
#define TKCURRENTMIN		2
#define TKCURRENTSEC		3
#define TKCURRENTISPM		4
#define TKLAUTOON			5
#define TKLONHRS			6
#define TKLONMIN			7
#define TKLAUTOOFF			8
#define TKLOFFHRS			9
#define TKLOFFMIN			10
#define TKRAUTOON			11
#define TKRONHRS			12
#define TKRONMIN			13
#define TKRAUTOOFF			14
#define TKROFFHRS			15
#define TKROFFMIN			16

// TOKENS POSITIONS SET MSG
#define TKSIDE			1
#define TKPOWER			2

#define RC1FLAG			PIR1bits.RC1IF
#define RC1BYTE			RCREG1

// Timers										
#define TMR0FLAG		INTCONbits.TMR0IF	// Define the flag generate after the overflow of Timer0
#define TMR1FLAG		PIR1bits.TMR1IF		// Define the flag generate after the overflow of Timer1

#define TMR1EN			T1CONbits.TMR1ON	// Only enable when sending RF signals

// Global Variables ===============================================================
unsigned char bufferSerial1 [BUFSIZE] = {0};
char messageFlag = 0;

char sentenceReady = 0;
unsigned char *strPtr = bufferSerial1;
char byte = 0;

char leftPowerBefore = 0, rightPowerBefore = 0;

char clkUpd[] = {"CLKUPD"};
char setPad[] = {"SETPAD"};
char *cmdStats[] = {clkUpd, setPad};

char *tokens[TOKENSIZE] = { 0 };

// Variables for am/pm conversion function
char convert [2] = {0};

char amMsg[] = {"a.m."};
char pmMsg[] = {"p.m."};
char *ampm[2] = {amMsg, pmMsg};

// Bit wise data stored in a array of characters
const rom char leftOnSignal25H[] = {	0xFC,0x0B,0x24,0x96,0x49,0x24,0x92,0x49,0x2D,0xB2,0x48,0x16,0x49,0x2C,0x92,
										0x49,0x24,0x92,0x5B,0x64,0x90,0x2C,0x92,0x59,0x24,0x92,0x49,0x24,0xB6,0xC9,
										0x27,0xE0,0x59,0xFD,0x96,0x49,0x65,0x92,0x59,0x6C,0xB6,0x5B,0xBE,0xDB,0x2C,
										0x92,0xCB,0x24,0xB2,0xD9,0x6C,0xB7,0x7D,0xB6,0x59,0x25,0x96,0x49,0x65,0xB2,
										0xD9,0x7F,0xB2,0xCF,0xE5,0xB6,0x49,0x65,0x92,0x5B,0x65,0x96,0xDD,0xF6,0xCB,
										0x6C,0x92,0xCB,0x24,0xB6,0xCB,0x2D,0xBB,0xED,0x96,0xD9,0x25,0x96,0x49,0x6D,
										0x96,0x5B,0x65,0x92};

const rom char leftOffSignal25H[] = {	0xFC,0x09,0x24,0x96,0x49,0x24,0x92,0x49,0x2C,0xB2,0x48,0x12,0x49,0x2C,0x92,
										0x49,0x24,0x92,0x59,0x64,0x90,0x24,0x92,0x59,0x24,0x92,0x49,0x24,0xB2,0xC9,
										0x20};

const rom char rightOnSignal25H[] = {	0xFC,0x0B,0x24,0x96,0x49,0x24,0x92,0x49,0x24,0xB2,0x48,0x16,0x49,0x2C,0x92,
										0x49,0x24,0x92,0x49,0x64,0x90,0x2C,0x92,0x59,0x24,0x92,0x49,0x24,0x92,0xC9,
										0x20,0x1F,0x81,0xFC,0x96,0x59,0x65,0x92,0x49,0x64,0x96,0xC9,0x2E,0xD9,0x2C,
										0xB2,0xCB,0x24,0x92,0xC9,0x2D,0x92,0x5D,0xB2,0x59,0x65,0x96,0x49,0x25,0x92,
										0x5B,0x24,0xFE,0x4F,0xE4,0xB6,0x59,0x65,0x92,0x49,0x64,0x96,0x49,0x76,0xC9,
										0x6C,0xB2,0xCB,0x24,0x92,0xC9,0x2C,0x92,0xED,0x92,0xD9,0x65,0x96,0x49,0x25,
										0x92,0x59,0x25,0x92};

const rom char rightOffSignal25H[] = {	0xFC,0x09,0x24,0x96,0x49,0x24,0x92,0x49,0x25,0x92,0x48,0x12,0x49,0x2C,0x92,
										0x49,0x24,0x92,0x4B,0x24,0x90,0x24,0x92,0x59,0x24,0x92,0x49,0x24,0x96,0x49,
										0x20};

const rom char *signalsControl [] = {leftOnSignal25H, leftOffSignal25H, rightOnSignal25H, rightOffSignal25H};

// Real-time clock properties
typedef struct
{
	char hrs;
	char min;
	char sec;
	char isPM;
	char is24;
}clock_t;

// Heating Pad Properties
typedef struct
{
	char leftPower;

	char rightPower;

	char leftAutoOnSet;
	char leftAutoOffSet;
	char rightAutoOnSet;
	char rightAutoOffSet;

	char rightOnHrs;
	char rightOnMin;
	char rightOnPM;

	char leftOnHrs;
	char leftOnMin;
	char leftOnPM;

	char rightOffHrs;
	char rightOffMin;
	char rightOffPM;

	char leftOffHrs;
	char leftOffMin;
	char leftOffPM;
}pad_t;

// Global Object creation
clock_t myClock;
pad_t myPad;


// Functions ---------------------------------------------------------------------------------
// Functions Prototypes used in the ISR
void High_ISR (void);
void timer0Setup(void);
void clockUpdate(void);
void timedPowerSet(void);

// Interrupt Vector
#pragma code high_vector = 0x0008
void high_vector (void)
{
	_asm
	goto High_ISR
	_endasm
}

#pragma code

		// --- Interrupt Service Routine ---
/*--- High_ISR -----------------------------------------------------------------------------
Author:		LRibeiro
Date:		15 July, 2019	
Modified:	None
Desc:		Interrupt service routine called on the overflow of Timer0 or when the RCREG 2 is
			full to update the real-time clock,	turn on/off when the programmed time arrive,
			 set the flag for update the app and receive commands from the app.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
#pragma interrupt High_ISR
void High_ISR (void)
{
	if (TMR0FLAG)
	{
		timer0Setup();
		// Update Clock
		clockUpdate();
		// Check time set to power ON/OFF
		timedPowerSet();
		messageFlag = 1;
	}
	if (RC1FLAG)
	{
		byte = RC1BYTE;
		if (byte == '$')
		{
			strPtr = bufferSerial1;
		}
		*strPtr = byte;
		strPtr++;
		if (byte == '#')
		{
			sentenceReady = 1;
			// NULL in the end of the buffer to create a string.
			*strPtr = 0;
		}
		if (strPtr > &bufferSerial1[BUFSIZE -1])
		{
			strPtr = bufferSerial1;
		}
	}
}	//eo High_ISR::

		// --- Interrupt Configuration ---
/*--- interruptConfig -----------------------------------------------------------------------------
Author:		LRibeiro
Date:		15 July, 2019	
Modified:	None
Desc:		Enable Interrupts and arm the TMR0
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void interruptConfig (void)
{
	// Enable GIE, PEIE and TMR0IE (3 MSB bits)
	INTCON = 0xE0;
	PIE1bits.RC1IE = 1;
}	//eo interruptConfig::

		// --- Initialize PAD and Clock Structure ---
/*--- initializePadAndClock ----------------------------------------------------------------
Author:		LRibeiro
Date:		15 July, 2019	
Modified:	None
Desc:		Initilize default values to the PAD and clock structure.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void initializePadAndClock (void)
{
	myPad.leftPower = 0;

	myPad.rightPower = 0;

	myPad.leftAutoOnSet = 0;
	myPad.leftAutoOffSet = 0;

	myPad.rightAutoOnSet = 0;
	myPad.rightAutoOffSet = 0;
	
	myPad.rightOnHrs = 9;
	myPad.rightOnMin = 45;
	myPad.rightOnPM = 1;

	myPad.rightOffHrs = 6;
	myPad.rightOffMin = 30;
	myPad.rightOffPM = 0;

	myPad.leftOnHrs = 9;
	myPad.leftOnMin = 45;
	myPad.leftOnPM = 1;

	myPad.leftOffHrs = 6;
	myPad.leftOffMin = 30;
	myPad.leftOffPM = 0;

	myClock.hrs = 12;
	myClock.min = 0;
	myClock.sec = 0;
	myClock.isPM = 1;
	myClock.is24 = 0;
}	//eo initializePadAndClock::

		// --- Send Update Message ---
/*--- sendUpdateMessage ----------------------------------------------------------------------
Author:		LRibeiro
Date:		15 July, 2019	
Modified:	None
Desc:		Send a structurized update message through Serial Port 1 for remote controlling.
Input: 		None
Returns:	None
----------------------------------------------------------------------------------------------*/
void sendUpdateMessage (void)
{
	printf("$UPDAPP,%i,%i,%i,%i,%i,%i,%02i,%02i,%i,%02i,%02i,%i,%02i,%02i,%i,%02i,%02i,%i,%02i,%02i,%02i,%i#",	
					myPad.leftPower, myPad.rightPower,
					myPad.leftAutoOnSet, myPad.leftAutoOffSet,myPad.rightAutoOnSet, myPad.rightAutoOffSet,
					myPad.leftOnHrs, myPad.leftOnMin, myPad.leftOnPM,
					myPad.rightOnHrs, myPad.rightOnMin, myPad.rightOnPM,
					myPad.leftOffHrs, myPad.leftOffMin, myPad.leftOffPM,
					myPad.rightOffHrs, myPad.rightOffMin, myPad.rightOffPM,
					myClock.hrs, myClock.min ,myClock.sec, myClock.isPM); 
}	//eo sendUpdateMessage::

		// --- Set FOSC to 8MHz ---
/*--- set_osc_p18f45k22_8MHz ----------------------------------------------------------------
Author:		LRibeiro
Date:		15 July, 2019	
Modified:	None
Desc:		Sets the internal Oscillator of the PIC18F45K22 to 8MHz.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void set_osc_p18f45k22_8MHz(void)
{
	OSCCON	= 0x62;				// Sleep on slp cmd, HFINT 8MHz, INT OSC Blk 0b (0110 0010) => 0x62
	OSCCON2 = 0x04;				// PLL No, CLK from OSC, MF off, Sec OSC off, Pri OSC 0b (0000 0100) => 0x04;
	OSCTUNE = 0x80;				// PLL disabled, Default factory freq tuning 0b (1000 0000) => 0x80;
	
	while (OSCCONbits.HFIOFS != 1); 	// wait for osc to become stable
}	// eo portconfig::


		// --- Port Configuration ---
/*--- portConfig -----------------------------------------------------------------------------
Author:		LRibeiro
Date:		13 Mar, 2019
Modified:	None
Desc:		Define GPIO pins whether input, output, analog, digital, and voltage levels
			following the schematic provided. For safety, unused pins are configured as
			digital inputs.
Input: 		None
Returns:	None
----------------------------------------------------------------------------------------------*/
void portConfig(void)
{
	ANSELA = 0x00; 	// Makes it digital
	LATA = 0x00;	// Set the voltage on the pin to low
	TRISA = 0xFF;	// If 1 the pin is input, if 0 the is output.
	
	ANSELB = 0x00;
	LATB = 0x00;
  	TRISB = 0xFF; 	

	ANSELC = 0x00;	// RC6 and RC7 TX/RX digital (Serial Port 1)
	LATC = 0x00;
	TRISC = 0xFF;	// RC6 and RC7 TX/RX input (Serial Port 1)

	ANSELD = 0x00;	// RD4 digital (RF Data Pin)
	LATD = 0x00;
	TRISD = 0xFE;	// RD0 output (RF Data Pin), RD2 input EN BTH pin

	ANSELE = 0x00;
	LATE = 0x00;
	TRISE = 0xFF;

}	// eo portconfig::

	// --- Configure UART1 ---
/*--- configSerial1 --------------------------------------------------------------------
Author:		LRibeiro
Date:		27 Mar, 2019
Modified:	None
Desc:		Configures serial port 1 to work asynchronous, full-duplex and
			with 9600 bps baud rate, if the OSC if set to 8 MHz.
Input: 		None
Returns:	None
---------------------------------------------------------------------------------------*/
void configSerial1(void)
{
	SPBRG1 = 51; 		// Baud rate selector for 9600 bps in 8 Mhz
	TXSTA1 = 0x26;		// TXEN = 1, SYNC = 0, BRGH = 1
	RCSTA1 = 0x90;		// SPEN = 1, CREN = 1
	BAUDCON1 = 0x40;	// BRG16 = 0
}	// eo configSerial1::

	// --- Timer 0 Setup ---
/*--- timer0Setup-------------------------------------------------------------------------
Author:		LRIBEIRO
Date:		10 Apr, 2019
Modified:	None
Desc:		Reset Timer flag and assign the False Start for 1 sec overflow if the OSC
			if set to 8 MHz.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
void timer0Setup(void)
{
	// 1 sec overflow
	TMR0FLAG	=	0;
	TMR0H		=	0x0B;	
	TMR0L		=	0xE3; // 0xDC discounted the time to enter the ISR and update the seconds.
}	// eo timer0Setup::

	// --- Timer 1 Setup ---
/*--- timer1Setup------------------------------------------------------------------------
Author:		LRIBEIRO
Date:		10 Apr, 2019
Modified:	None
Desc:		Reset Timer flag and assign the False Start for 625 us overflow if the OSC
			is set to 8 MHz.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
int timer1Setup(void)
{
	// 625 us overflow
	TMR1FLAG	=	0;
	TMR1H		=	0xFD;	
	TMR1L		=	0x8F;
}	// eo timer1Setup::

	// --- Timer 0 Configuration ---
/*--- configTimer0 ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		10 Apr, 2019
Modified:	None
Desc:		Enable Timer0 and configure the Pre-scaler to count up to 1 second.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
int configTimer0(void)
{
	T0CON = 0x94;					// Pre-scaler = 1:32
	timer0Setup();
}	// eo configTimer0::

	// --- Timer 1 Configuration ---
/*--- configTimer1 ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		10 Apr, 2019
Modified:	None
Desc:		Enable Timer1 and configure the Pre-scaler to count up to 625 us.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
int configTimer1(void)
{
	T1CON = 0x12;					// Pre-scaler = 1:2
	timer1Setup();
}	// eo configTimer1::



		// --- Send RF Command ---
/*--- sendRfCommand ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		15 July, 2019
Modified:	None
Desc:		Bit bang the characters inside a array, following the MSB->LSB order and a 
			bit length of 625 us, controller by Timer 1.
Input: 		char - Signal index that the user wants to send (0-3).
Returns:	None
-----------------------------------------------------------------------------------------*/
void sendRfCommand (char signal)
{
	int count = 0, index = 0, index1 = 0, size = 0;
	// Timer1 enable before transmitting the RF signal, to generate the bitlenght.
	TMR1EN = TRUE;
	timer1Setup();
	if (signal == LEFTONSIGNAL || signal == RIGHTONSIGNAL)
	{
		size = ONSIGNALSIZE;
	}
	else
	{
		size = OFFSIGNALSIZE;
	}
	// Send the same command 2 times to guarantee the reception, with a delay in between
	for (index1 = 0; index1 < 2; index1 ++)
	{
		count = 0;
		while (count < size)
		{
			for (index = 0; index < 8; index ++)
			{
				while(!TMR1FLAG);
				timer1Setup();
	   			DATA1OUT = ((*(signalsControl[signal] + count) >> (7 - index)) % 2);
			}
			count ++;
		}
		if (index1 == 0)
		{
			// Delay between two send of the same signal. If the BTH is connected the daly is the time to send
			// the update sentence, else 100 ms.
			if (BTH_CONNECTED)
			{
				sendUpdateMessage();
			}
			else 
			{
				Delay10KTCYx ( 20 );
			}
		}
	}
	TMR1EN = FALSE;
}	// eo sendRfCommand::

		// --- Clock Update ---
/*--- clockUpdate ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		10 Apr, 2019
Modified:	None
Desc:		Update the global clock sec, min and hours each 1 sec, controller by Timer0.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
void clockUpdate (void)
{
	myClock.sec ++;
	if (myClock.sec > 59)
	{
		myClock.sec -= 60;
		myClock.min += 1;
		if (myClock.min > 59)
		{
			myClock.min = 0;
			myClock.hrs += 1;
			
			if (myClock.is24)
			{
				if(myClock.hrs > 23)
				{
					myClock.hrs = 0;
				}
			}
			else
			{
				if (myClock.hrs > 12)
				{
					myClock.hrs = 1;
				}
				if (myClock.hrs == 12)
				{
					myClock.isPM ^= 1;
				}
			}
		} 	
	}
}	// eo clockUpdate::

		// --- Parse String ---
/*--- parseString ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		01 July, 2019
Modified:	None
Desc:		Parse a string eliminating the delimiters and saving the key information in 
			order a token array.
Input: 		char* - Address of the string to be parsed
Returns:	None
-----------------------------------------------------------------------------------------*/
void parseString (char *strptr)
{
	char tokenCounter = 0;
	while (*strptr)
	{
		if (*strptr == ',' || *strptr == ':' || *strptr == '#' || *strptr == ' ' || *strptr == '$')
		{
			*strptr = 0;
			tokens[tokenCounter] = (strptr + 1);
			tokenCounter ++;
		}
		strptr ++;
	}
}	// eo parseString::

		// --- Convert 24 to 12 Hours ---
/*--- convert24to12hrs ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		01 July, 2019
Modified:	None
Desc:		Convert a 24 format time to a 12 hours format, saving the hours and the am/pm
			status in a global array.
Input: 		char - 24 hours format hour
Returns:	None
-----------------------------------------------------------------------------------------*/
void convert24to12hrs (char hours)
{
	if (hours > 12)
	{
		convert[HRSINDEX] = hours - 12;
		convert[ISPMINDEX] = TRUE;
	}
	else if (hours == 12)
	{
		convert[HRSINDEX] = hours;
		convert[ISPMINDEX] = TRUE;
	}
	else if (hours == 0)
	{
		convert[HRSINDEX] = 12;
		convert[ISPMINDEX] = FALSE;
	}
	else
	{
		convert[HRSINDEX] = hours;
		convert[ISPMINDEX] = FALSE;
	}
}	// eo convert24to12hrs::

		// --- Update Controller ---
/*--- updateController ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		16 July, 2019
Modified:	None
Desc:		Using tokens infomation, convert it to integral values and save in the correct
			struct data members, updating the time, and auto ON/OFF settings.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
void updateController (void)
{
		// Clock update information come in a 12 hours format
		myClock.hrs = atoi(tokens[TKCURRENTHRS]);
		myClock.min = atoi(tokens[TKCURRENTMIN]);
		myClock.sec = atoi(tokens[TKCURRENTSEC]);
		if (*tokens[TKCURRENTISPM] == 'p')
		{
			myClock.isPM = TRUE;
		}
		else
		{
			myClock.isPM = FALSE;
		}

		// The token will a 't' if the auto on/off configuration is set and a 'f' if not
		if (*tokens[TKLAUTOON] == 't')
		{
			myPad.leftAutoOnSet = TRUE;
		}
		else
		{
			myPad.leftAutoOnSet = FALSE;
		}
		if (*tokens[TKLAUTOOFF] == 't')
		{
			myPad.leftAutoOffSet = TRUE;
		}
		else
		{
			myPad.leftAutoOffSet = FALSE;
		}

		if (*tokens[TKRAUTOON] == 't')
		{
			myPad.rightAutoOnSet = TRUE;
		}
		else
		{
			myPad.rightAutoOnSet = FALSE;
		}

		if (*tokens[TKRAUTOOFF] == 't')
		{
			myPad.rightAutoOffSet = TRUE;
		}
		else
		{
			myPad.rightAutoOffSet = FALSE;
		}
		// The time picker in the android send data in 24 hours format, this part of code perform the 
		// convertion for update the clock in a 12 hours mode
		// Left ON Time
		convert24to12hrs(atoi(tokens[TKLONHRS]));
		myPad.leftOnHrs = convert[HRSINDEX];
		myPad.leftOnMin = atoi(tokens[TKLONMIN]);
		myPad.leftOnPM = convert [ISPMINDEX];
		// Left OFF Time
		convert24to12hrs(atoi(tokens[TKLOFFHRS]));
		myPad.leftOffHrs = convert[HRSINDEX];
		myPad.leftOffMin = atoi(tokens[TKLOFFMIN]);
		myPad.leftOffPM = convert [ISPMINDEX];
		// Right ON Time
		convert24to12hrs(atoi(tokens[TKRONHRS]));
		myPad.rightOnHrs = convert[HRSINDEX];
		myPad.rightOnMin = atoi(tokens[TKRONMIN]);
		myPad.rightOnPM = convert [ISPMINDEX];
		// Rigth OFF Time
		convert24to12hrs(atoi(tokens[TKROFFHRS]));
		myPad.rightOffHrs = convert[HRSINDEX];
		myPad.rightOffMin = atoi(tokens[TKROFFMIN]);
		myPad.rightOffPM = convert [ISPMINDEX];
}	// eo updateController::

		// --- Update Power Status ---
/*--- updatePowerStatus ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		16 July, 2019
Modified:	None
Desc:		Using token information for the SETPAD commands, save the desired status in the 
			struct data member, updating the ON/OFF status.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
void updatePowerStatus (void)
{
	if (atoi(tokens[TKSIDE]) == LEFTSIDE)
	{
		myPad.leftPower = atoi(tokens[TKPOWER]);
	}
	else if (atoi(tokens[TKSIDE]) == RIGHTSIDE)
	{
		myPad.rightPower = atoi(tokens[TKPOWER]);;
	}
}	// eo updatePowerStatus::

		// --- Timed Power Set ---
/*--- F ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		16 July, 2019
Modified:	None
Desc:		If the Auto ON/OFF settings is set, when the time has came, update the power status
			in the pad strucute.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
void timedPowerSet (void)
{
	if (myPad.leftAutoOnSet == TRUE)
	{
		if (myClock.hrs == myPad.leftOnHrs && myClock.min == myPad.leftOnMin && myClock.sec == 0)
		{
			myPad.leftPower = TRUE;
		}
	}

	if (myPad.rightAutoOnSet == TRUE)
	{
		if (myClock.hrs == myPad.rightOnHrs && myClock.min == myPad.rightOnMin && myClock.sec == 0)
		{
			myPad.rightPower = TRUE;
		}
	}

	if (myPad.leftAutoOffSet == TRUE)
	{
		if (myClock.hrs == myPad.leftOffHrs && myClock.min == myPad.leftOffMin && myClock.sec == 0)
		{
			myPad.leftPower = FALSE;
		}
	}

	if (myPad.rightAutoOffSet == TRUE)
	{
		if (myClock.hrs == myPad.rightOffHrs && myClock.min == myPad.rightOffMin && myClock.sec == 0)
		{
			myPad.rightPower = FALSE;
		}
	}
}	// eo timedPowerSett::

	// --- Select Command Routine ---
/*--- selectCommandRoutine ---------------------------------------------------------------
Author:		LRIBEIRO
Date:		15 July, 2019
Modified:	None
Desc:		Differentiate the commands sentences received from the controller and act
			accordingly.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
void selectCommandRoutine (void)
{
	// Check if the CMDSTM is in the correct position
	if (!strcmp(tokens[TKCMDSTM], cmdStats[UPDCONTROL]))
	{
		// Update Clock and Auto-ON and OFF settings
		updateController();
	}
	// Check if the CMDSTM is in the correct position
	if (!strcmp(tokens[TKCMDSTM], cmdStats[SETPOWER]))
	{
		// Update ON/OFF status 
		updatePowerStatus();
	}
}	// eo selectCommandRoutine::

	// --- Power Status Change ---
/*--- powerStatusChange -----------------------------------------------------------------
Author:		LRIBEIRO
Date:		16 July, 2019
Modified:	None
Desc:		Check if the desired power status for each side is different of the previous one,
			and if so, send the the correct RF signal to the existing receiver.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
void powerStatusChange (void)
{
	char leftChangeFlag = 0;
	if (leftPowerBefore != myPad.leftPower)
	{
		leftPowerBefore = myPad.leftPower;
		leftChangeFlag = 1;
		//Turn Left ON
		if (myPad.leftPower == TRUE)
		{
			// RF Signal Request
			sendRfCommand (LEFTONSIGNAL);
		}
		//Turn Left OFF
		else
		{
			// RF Signal Request
			sendRfCommand (LEFTOFFSIGNAL);
		}
	} // eo if
	
	if (rightPowerBefore != myPad.rightPower)
	{
		rightPowerBefore = myPad.rightPower;
		// Turn Right ON, if LEFT signal was also exectued, Delay before sending the command.
		// The delay can be the time for sending the update message to the app through serial, if the device is connected
		// if not a regular delay is called of 100 ms.
		if (leftChangeFlag)
		{
			if (BTH_CONNECTED)
			{
				sendUpdateMessage();
			}
			else 
			{
				Delay10KTCYx ( 20 );
			}
		}
		if (myPad.rightPower == TRUE)
		{
			sendRfCommand (RIGHTONSIGNAL);
		}
		//Turn Right OFF
		else
		{
			sendRfCommand (RIGHTOFFSIGNAL);
		}
	}	// eo if
}	// eo powerStatusChange::

		// --- Initialize System ---
/*--- initializeSystem ----------------------------------------------------------------------
Author:		LRibeiro
Date:		16 July, 2019
Modified:	None
Desc:		Call all the configuration functions that will be used on the program, the functions
			to initialize values into structures and arrays.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void initializeSystem(void)
{
	set_osc_p18f45k22_8MHz();			// Set the processor speed
	portConfig();
	configSerial1();
	configTimer0();
	configTimer1();
	initializePadAndClock();
	interruptConfig();
}	// eo initializeSystem::

/*--- MAIN FUNCTION -------------------------------------------------------------------------
-------------------------------------------------------------------------------------------*/

void main(void)	
{
	initializeSystem();	// Function call for setting the system I/Os and enabling UART 1
						//	Many PIC library functions are called within
						// End of the initialization portion of this code
	
	// Begin indefinite loop for program
	while(1)
	{
		if(BTH_CONNECTED)
		{
			// Check command messages comming from the app
			if (sentenceReady)
			{
				sentenceReady = 0;
				//getSerialString(bufferSerial1);
				parseString (bufferSerial1);
				// Process the message received checking the command statements directive
				selectCommandRoutine();		
			}	// eo if (RC1FLAG)
			if(messageFlag)
			{
				messageFlag = 0;
				// Control Update Message
				sendUpdateMessage();
			} // eo if (TMR0FLAG)
		}

		// Check if the desired Power settings changed and send the repective command to the Sunbeam Receiver
		// Changes can happen in the ISR, when a AUTO OFF/OFF is set and the time arrives, or through manual
		// ON/OFF comamands.
		powerStatusChange();

	} // eo while
// eo main
}
