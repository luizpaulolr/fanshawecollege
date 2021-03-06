		//--- Three Axis Controller (TAC) ---
/*----------------------------------------------------------------------------------------------
	File Name:	stepperMotorControl.c
	Author:		LRIBEIRO
	Date:		08/07/2019
	Modified:	None
	Copyright 	Fanshawe College, 2019

	Description: 	This programs simulates the operation of three steppers motors receiving its
			position through a potentiometer and display the status in the standard
			Terminal through Serial asynchronous communication.
                	It also send status strings to another microcontroller connected in the Serial
			Port 2.		

----------------------------------------------------------------------------------------------*/

// Preprocessor ------------------------------------------------------------------------------

#include "pragmas.h"
#include <stdio.h>
#include <stdlib.h>
#include <delays.h>

// Constants ---------------------------------------------------------------------------------

#define TRUE		1	// On Build, Defined Labels will be replaced by values as listed
#define	FALSE		0	
// Push buttons location following the schematic diagram
#define RESET 			PORTBbits.RB3  
#define ADVPOS 			PORTBbits.RB2
#define SET 			PORTBbits.RB1
#define SELECT			PORTBbits.RB0

#define REGULARMSG		0
#define MAJORCHGMSG		1

#define BTNSELECT		0
#define BTNADVPOS		1

// LEDs buttons location following the schematic diagram
#define DROPLED 		LATBbits.LATB7
#define PROCSLED		LATBbits.LATB6	// Define the pins where LEDs are connected on the uC
#define APROCHLED		LATBbits.LATB5
#define HOMELED			LATBbits.LATB4

#define TOOLLED			LATAbits.LATA1

#define DEGSYMBOL		248

#define AXES			3

#define X			0
#define Y			1
#define Z			2
#define TOOL			3

#define STEPPERRES		3

// Min diference to take a step, to be always 1 degree close to the desired position
#define MINDEG4STEP		1

#define NUMOFPATTERN		4

#define MANUAL			0
#define AUTOMATIC		1

#define TX2FLAG			PIR3bits.TX2IF
#define TMR0FLAG		INTCONbits.TMR0IF	// Define the flag generate after the overflow of Timer0

#define HALFSECFS		0x0BDC
#define ONESECOUNT		2
#define THREESECOUNT		6

#define OFFSET			512
#define COEFF			5.6888

// Macro for implementing the absolute function
#define abs(n)   		( ((n) >= 0) ? (n) : -(n) )

// Masks for resetting the setteper pin before setting the pattern
#define XMASK			0x0F
#define YMASK			0xF0
#define ZMASK			0xF0
#define MAJORMASK		0x0F

// Lab 2 Constants
#define MAJORPOSITIONS		4
#define NUMOFINFOPOS		4

#define HOME			0
#define APROCH			1
#define PROCS			2
#define DROP			3

// LAB 3 Constants
#define CONTROLLER		1
#define HEADER			'$'
#define BUFFERSIZE		80

// Global Variables --------------------------------------------------------------------------
char clrScreen[] = { "\e[2J\e[H" };
char patterns[NUMOFPATTERN] = {0x01, 0x02, 0x04, 0x08};
char axisTitles[][5] = {"X", "Y", "Z", "TOOL"};

char axisCheckFlag = 0, threeSecFlag = 0, tapOrHoldFlag = 0;

char angleAssignmentFlag = 0;
char majorPosIndex = 0, axisIndex = 0;

char timerCount = 0;

char majorPosition0[] = {"HOME"};
char majorPosition1[] = {"APRCH"};
char majorPosition2[] = {"PROCS"};
char majorPosition3[] = {"DROP"};

char messageBuffer[BUFFERSIZE] = { 0 };

char *positionTitles[MAJORPOSITIONS] = { 0 };

char displayMessageCounter = 0;

int desiredPosition = 0;

char defaultPosition [MAJORPOSITIONS][NUMOFINFOPOS];

typedef struct axis
{
	char position;
	char currentPosition;
	char pattern;
	char patternCounter;
} axis_t;

typedef struct threeAxisController
{
	char controllerAddress[4];
	char operationMode;
	char currentAxis;
	char majorPosition;
	axis_t axisInfo[AXES];
} threeAxisController_t;

threeAxisController_t tac302;


// Functions ---------------------------------------------------------------------------------

        // --- Checsum calculation using XOR operator ---
/*--- calcCheck ----------------------------------------------------------------
Author:     LRibeiro
Date:       15 May, 2019    
Modified:   None
Desc:       Calculate the checksum of a string using XOR operator.
Input:      char * - Address of the string
Returns:    char - checksum 8-bits value
------------------------------------------------------------------------------*/
char calcChecksum (char *strptr)
{
	char checksum = 0;
	while (*strptr)
	{
		checksum ^= *strptr;
		strptr ++;
	}
	return checksum;
}    // eo calcChecksum

        // --- Message creation ---
/*--- messageCreation ----------------------------------------------------------------
Author:     LRibeiro
Date:       17 Jun, 2019    
Modified:   None
Desc:       Create the codified message to be sent to the controller
Input:      None
Returns:    None
------------------------------------------------------------------------------*/
void messageCreation (char messageType)
{
	char checksum = 0;
	if (messageType == REGULARMSG)
	{
		// Manual Mode message
		if (tac302.operationMode == MANUAL)
		{
			sprintf(messageBuffer,"%cMAXCHG,%i,%s,%c,%i", 	HEADER, CONTROLLER,
									tac302.controllerAddress,
									axisTitles[tac302.currentAxis], 
									tac302.axisInfo[tac302.currentAxis].position);
		}
		// Automatic Mode message
		else
		{
			sprintf(messageBuffer,"%cSMPCHG,%i,%s,%c,%i,%i,%i", 	HEADER, CONTROLLER,
																	tac302.controllerAddress,
																	*positionTitles[tac302.majorPosition],
																	tac302.axisInfo[X].position,
																	tac302.axisInfo[Y].position,
																	tac302.axisInfo[Z].position);
		}
	}
	else if (messageType == MAJORCHGMSG)
	{
		sprintf(messageBuffer,"%cMAJORCHG,%i,%s,%c,%i,%i,%i,%i,%c,%i,%i,%i,%i,%c,%i,%i,%i,%i,%c,%i,%i,%i,%i",
									HEADER, CONTROLLER,
									tac302.controllerAddress,
									*positionTitles[HOME], defaultPosition[HOME][X],
									defaultPosition[HOME][Y], defaultPosition[HOME][Z], defaultPosition[HOME][TOOL],
									*positionTitles[APROCH], defaultPosition[APROCH][X], defaultPosition[APROCH][Y],
									defaultPosition[APROCH][Z], defaultPosition[APROCH][TOOL],
									*positionTitles[PROCS], defaultPosition[PROCS][X], defaultPosition[PROCS][Y],
									defaultPosition[PROCS][Z], defaultPosition[PROCS][TOOL],
									*positionTitles[DROP], defaultPosition[DROP][X], defaultPosition[DROP][Y],
									defaultPosition[DROP][Z], defaultPosition[DROP][TOOL]);


	}
	// Calculation of the checksum
	checksum = calcChecksum (messageBuffer);
	// Attach check sum and the END CHAR
	sprintf(messageBuffer, "%s,%i#", messageBuffer, checksum);
	// Flag for displaying the message  on the terminal
	displayMessageCounter = 5; 	// 5 secs
}   // eo messageCreation

        // --- Send a byte using UART2 ---
/*--- sendByte ----------------------------------------------------------------
Author:     LRibeiro
Date:       17 Jun, 2019     
Modified:   None
Desc:       Send a byte through the UART2
Input:      char - Byte to be sent through UART2
Returns:    None
------------------------------------------------------------------------------*/
void sendByte (char byte)
{
	while(!TX2FLAG);
	TXREG2 = byte;
}   // eo sendByte

        // --- Send a string using UART2 ---
/*--- sendStringUART2 ----------------------------------------------------------------
Author:     LRibeiro
Date:       17 Jun, 2019    
Modified:   None
Desc:       Send a string through the UART2
Input:      char * - Address of the string
Returns:    None
------------------------------------------------------------------------------*/
void sendStringUART2 (char * string)
{
	do
	{
		sendByte (*string);
	} while (*string ++);
}   // eo sendStringUART2

		// --- Iniatilize Major Positions Array ---
/*--- initializePositions ----------------------------------------------------------------
Author:		LRibeiro
Date:		15 May, 2019	
Modified:	None
Desc:		Initialize values for the 3 axes in each major position and the toggle status.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void initializePositions (void)
{
	char index = 0, index1 = 0;
	for (index = 0; index < MAJORPOSITIONS; index ++)
	{
		switch (index)
		{
			case HOME:
					defaultPosition[index][X] = 0;
					defaultPosition[index][Y] = 90;
					defaultPosition[index][Z] = -90;
					defaultPosition[index][TOOL] = FALSE;
					positionTitles[index] = majorPosition0;
				break;
			case APROCH:
					defaultPosition[index][X] = 90;
					defaultPosition[index][Y] = 15;
					defaultPosition[index][Z] = 90;
					defaultPosition[index][TOOL] = TRUE;
					positionTitles[index] = majorPosition1;
				break;
			case PROCS:
					defaultPosition[index][X] = -45;
					defaultPosition[index][Y] = -30;
					defaultPosition[index][Z] = 30;
					defaultPosition[index][TOOL] = TRUE;
					positionTitles[index] = majorPosition2;
				break;
			case DROP:
					defaultPosition[index][X] = 0;
					defaultPosition[index][Y] = 60;
					defaultPosition[index][Z] = 0;
					defaultPosition[index][TOOL] = FALSE;
					positionTitles[index] = majorPosition3;
				break;
			default:
				break;
		}
	}
}	//eo initializePositions::

		// --- Set FOSC to 4MHz ---
/*--- set_osc_p18f45k22_4MHz ----------------------------------------------------------------
Author:		LRibeiro
Date:		15 May, 2019	
Modified:	None
Desc:		Sets the internal Oscillator of the Pic 18F45K22 to 4MHz.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void set_osc_p18f45k22_4MHz(void)
{
	OSCCON	= 0x52;				// Sleep on slp cmd, HFINT 4MHz, INT OSC Blk 0b (0101 0010) => 0x52
	OSCCON2 = 0x04;				// PLL No, CLK from OSC, MF off, Sec OSC off, Pri OSC 0b (0000 0100) => 0x04;
	OSCTUNE = 0x80;				// PLL disabled, Default factory freq tuning 0b (1000 0000) => 0x80;
	
	while (OSCCONbits.HFIOFS != 1); 	// wait for osc to become stable
}	//eo set_osc_p18f45k22_4MHz::

		// --- Port Configuration ---
/*--- portConfig ---------------------------------------------------------------------------
Author:		LRibeiro
Date:		13 Mar, 2019
Modified:	None
Desc:		Define GPIO pins whether input, output, analog, digital, and voltage levels following the 
			schematic provided.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void portConfig(void)
{
	ANSELA = 0x01;	// POT ANALOG A0
	LATA = 0x00;
	TRISA = 0x0D;	// POT INPUT A0 + A1 LED TOOL OUTPUT + NOT CONNECTED A2-A3 + A4-A7 OUTPUT STEPPER X
	
	ANSELB = 0x00;
	LATB = 0x00;
  	TRISB = 0x0F;	// PUSHBUTTONS RB0-RB3 + LED RB7-RB4

	ANSELC = 0x00;
	LATC = 0x00;
	TRISC = 0xF0;	// RC0-RC3 LED STEPPER Z + NOT CONNECTED RC4-RC5 +RC6 + RC7 (INPUT SERIAL 1)

	ANSELD = 0x00;
	LATD = 0x00;
	TRISD = 0xF0;	// RD0-RD3 LED STEPPER Y + NOT CONNECTED RD4-RD7 

	ANSELE = 0x00;
	LATE = 0x00;
	TRISE = 0xFF;	// NOT CONNECTED

}	// eo portconfig::

		// --- ADC Configuration ---
/*--- initializeADC ---------------------------------------------------------------------------
Author:		CTalbot
Date:		14 Sept, 2016
Modified:	CTalbot
Desc:		Initalizes the ADC Module in the Processor to sample from AN0 (A0 pin).
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void initializeADC(void)
{
	TRISAbits.TRISA0= 1;			 	// Config pin as input
 	ANSELAbits.ANSA0 = 1; 				// Disable digital buffer, set to analog
	ADCON0 = 0x01; 					// Channel AN0 selects (PIN A0), ADON = 1
	ADCON1 = 0x00;					// Internal Reference for VDD and VSS
	ADCON2 = 0xA9;					// Right justified, 12 TAD, Fosc/8 
}	// eo initializeADC::

	// --- Get ADC Value ---
/*--- getAdc ---------------------------------------------------------------------------
Author:		LRIBEIRO
Date:		19 Mar, 2019
Modified:	None
Desc:		Reads from the ADC at A0, returns the result
Input: 		None
Returns:	int - The ADC read result as a binary value
----------------------------------------------------------------------------------------*/
int getAdc(void)
{
	int result = 0x00;
	ADCON0bits.GO = 1;			// Start conversion 
	while (ADCON0bits.GO);			// Wait for completion 
	result = ADRES;				// Read result (((unsigned int)ADRESH)<<8)|(ADRESL)   
	return result;
}	// eo getAdc::

	// --- Linear Equation Convertion ---
/*--- binary2Degree ------------------------------------------------------------------------
Author:		LRibeiro
Date:		15 May, 2019	
Modified:	None
Desc:		Initialize values to the TAC.
Input: 		int - Binary Value from 0-1023
Returns:	int - Integer representing degrees from -90 to 90
--------------------------------------------------------------------------------------------*/
int binary2Degree (int adcValue)
{
	adcValue -= OFFSET;
	adcValue = (float)(adcValue / COEFF);
	return adcValue;
}	//eo binary2Degree::

	// --- Configure UART1 ---
/*--- configSerial1 ---------------------------------------------------------------------------
Author:		LRibeiro
Date:		27 Mar, 2019
Modified:	None
Desc:		Configures serial port 1 to work asynchronous, full-duplex and
			with 9600 bps baud rate.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void configSerial1(void)
{
	SPBRG1 = 25; 		// Baud rate selector for 9600 bps in 4 Mhz
	TXSTA1 = 0x26;		// TXEN = 1, SYNC = 0, BRGH = 1
	RCSTA1 = 0x90;		// SPEN = 1, CREN = 1
	BAUDCON1 = 0x40;	// BRG16 = 0
}	// eo configSerial1::

	// --- Configure UART2 ---
/*--- configSerial1 ---------------------------------------------------------------------------
Author:		LRibeiro
Date:		27 Mar, 2019
Modified:	None
Desc:		Configures serial port 1 to work asynchronous, full-duplex and
			with 9600 bps baud rate.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void configSerial2(void)
{
	SPBRG2 = 25; 		// Baud rate selector for 9600 bps in 4 Mhz
	TXSTA2 = 0x26;		// TXEN = 1, SYNC = 0, BRGH = 1
	RCSTA2 = 0x90;		// SPEN = 1, CREN = 1
	BAUDCON2 = 0x40;	// BRG16 = 0
}	// eo configSerial2::

	// --- Reset Timer 0 ---
/*--- timer0Setup------------------------------------------------------------------------
Author:		LRIBEIRO
Date:		10 Apr, 2019
Modified:	None
Desc:		Reset Timer flag and assign the False Start.
Input: 		int - False start
Returns:	None
-----------------------------------------------------------------------------------------*/
void timer0Setup(int falseStart)
{
	TMR0FLAG	=	0;
	TMR0H		=	(char) (falseStart >> 8);
	TMR0L		=	(char) (falseStart);
}	// eo timerSetup00::

	// --- Timer 0 Configuration ---
/*--- configTimer0 ----------------------------------------------------------------------
Author:		LRIBEIRO
Date:		10 Apr, 2019
Modified:	None
Desc:		Configure Timer0 to flag after 1 seconds.
Input: 		None
Returns:	None
-----------------------------------------------------------------------------------------*/
void configTimer0(void)
{
	T0CON = 0x92;					// Pre-scaler = 1:8
	timer0Setup(HALFSECFS);
}	// eo configTimer0::

	// --- Stepper voltage levels setting ---
/*--- setStepper ---------------------------------------------------------------------------
Author:		LRibeiro
Date:		15 May, 2019	
Modified:	None
Desc:		Send pattern to the LAT in which the motor is connected. 
			Masking applied to avoid change undesired pins.
Input: 		char - Pattern to be assign to the stepper motor (0x01, 0x02...).
			char - Stepper motor to spin, identified bt the axis.
Returns:	None
--------------------------------------------------------------------------------------------*/
void setStepper (char pattern, char currentAxis)
{
	switch (currentAxis)
	{
		case 0:
			// X motor is on the high nibble of LATA
			LATA = ((LATA & XMASK) | (((unsigned int)pattern ) << 4));
			break;
		case 1:
			// Y motor is on the low nibble of LATD
			LATC = ((LATD & YMASK) | pattern);
			break;
		case 2:
			// Z motor is on the low nibble of LATC
			LATD = ((LATC & ZMASK) | pattern);
			break;
		default:
			break;
	}
}	//eo setStepper::
		
	// --- Initialize values in the TAC ---
/*--- initializeController ----------------------------------------------------------------
Author:		LRibeiro
Date:		15 May, 2019	
Modified:	None
Desc:		Initialize values to the TAC.
Input: 		Pointer the the TAC struct to be initialized.
Returns:	None
--------------------------------------------------------------------------------------------*/
void initializeController (threeAxisController_t *ptrTac)
{
	int index = 0;

	ptrTac->controllerAddress[0] = '3';
	ptrTac->controllerAddress[1] = '0';
	ptrTac->controllerAddress[2] = '2';
	ptrTac->controllerAddress[3] = 0;

	ptrTac->operationMode = MANUAL;
	
	ptrTac->majorPosition = 0;

	ptrTac->currentAxis = X;
	
	for (index = 0; index < AXES; index ++)
	{
		ptrTac->axisInfo[index].position = 0;
		ptrTac->axisInfo[index].currentPosition = 0;
		/* If the pattern counter is 0 the pattern is actually 0x01 not 0x00 as stated in the Lab instructions
		NB: There is no pattern 0, the options are (0x01, 0x02, 0x04, 0x08)									*/
		ptrTac->axisInfo[index].pattern = 0x01;
		ptrTac->axisInfo[index].patternCounter = 0;
		// Load the Initial Pattern to the motors so its locked on the "Initial position"
		setStepper(ptrTac->axisInfo[index].pattern, index);
	}
}	//eo initializeController::

	// --- Display Terminal ---
/*--- displayTerminal ----------------------------------------------------------------------
Author:		LRibeiro
Date:		15 May, 2019	
Modified:	None
Desc:		Display TAC status on the standard output.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void displayTerminal(void)
{
	printf("%s", clrScreen);
	printf("TAC%s System Properties\n\r", tac302.controllerAddress);
	printf("Mode: \t\t");
	if (angleAssignmentFlag == 0)
	{
		if(tac302.operationMode == 0)
		{
			printf("Manual\n\r");
			printf("Axis Select: \t%s\n\r", axisTitles[tac302.currentAxis]);
		}
		else
		{
			printf("Auto\n\r");
			printf("Axis Select: \tAll\n\r");
		}
		printf("Axis Control: \t%i%c\n\r", desiredPosition, DEGSYMBOL);
		printf("Major position: %s\n\n\r", positionTitles[tac302.majorPosition]);
	
		printf("X\t\tY\t\tZ\n\r");
		printf("Position: %i%c\tPosition: %i%c\tPosition: %i%c\n\r", 	tac302.axisInfo[X].position,DEGSYMBOL,
																		tac302.axisInfo[Y].position, DEGSYMBOL,
																		tac302.axisInfo[Z].position, DEGSYMBOL);
		printf("Current : %i%c\tCurrent : %i%c\tCurrent : %i%c\n\n\r", 	tac302.axisInfo[X].currentPosition, DEGSYMBOL,
																		tac302.axisInfo[Y].currentPosition, DEGSYMBOL,
																		tac302.axisInfo[Z].currentPosition, DEGSYMBOL);
		printf("Tool: ");
	
		if(TOOLLED == FALSE)
		{
			printf("OFF\n\r");
		}
		else
		{
			printf("ON\n\r");
		}
		// Display the contrller's message every time that its crated for 5 sec (flag set in messageCreation function)
		if (displayMessageCounter)
		{
			printf("\n\n%s", messageBuffer);
			displayMessageCounter --;
		}
	}
	else 
	{
		printf("Manual \033[31;1;4m(Angle Assignment)\033[0m\n\r");
		printf("Axis Select: \t%s\n\r", axisTitles[axisIndex]);
		printf("Axis Control: \t%i%c\n\r", desiredPosition, DEGSYMBOL);
		printf("Major position: %s\n\n\r", positionTitles[majorPosIndex]);
		printf("X\t\tY\t\tZ\n\r");
		printf("Position: %i%c\tPosition: %i%c\tPosition: %i%c\n\r", 	defaultPosition[majorPosIndex][X],DEGSYMBOL,
																		defaultPosition[majorPosIndex][Y], DEGSYMBOL,
																		defaultPosition[majorPosIndex][Z], DEGSYMBOL);
		printf("Tool: ");
		if(defaultPosition[majorPosIndex][TOOL] == FALSE)
		{
			printf("OFF\n\r");
		}
		else
		{
			printf("ON\n\r");
		}
	}
}	//eo displayTerminal::

	// --- Routine Execution for 1 step ---
/*--- routineExecution ----------------------------------------------------------------------
Author:		LRibeiro
Date:		15 May, 2019	
Modified:	None
Desc:		Resolve the spin direction, call the function to send patterns, to the steppers
			and update the position on the structure.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void routineExecution (char axis)
{
		/* Check if the motor needs to spin to reach the desired position. As the resolution is 3 degrees, and to be as close as
		possible it should move if the distance is equal or grater than greater one.*/
		if (abs(tac302.axisInfo[axis].position - tac302.axisInfo[axis].currentPosition) > MINDEG4STEP)
		{
			// If the pattern is chaging the timer is reset here to ensure that the dalay is happening between the pattern changes
			timer0Setup(HALFSECFS);
			// If the desired position is higher, rotate clockwise and increment the current position
			if (tac302.axisInfo[axis].position > tac302.axisInfo[axis].currentPosition)
			{
				tac302.axisInfo[axis].patternCounter ++;
				if (tac302.axisInfo[axis].patternCounter > (NUMOFPATTERN - 1))
				{
					tac302.axisInfo[axis].patternCounter = 0;
				}
				tac302.axisInfo[axis].pattern = patterns[tac302.axisInfo[axis].patternCounter];
				setStepper(tac302.axisInfo[axis].pattern, axis);
				tac302.axisInfo[axis].currentPosition += STEPPERRES;
			}
			// If the desired position is lesser, rotate anticlockwise and decrement the current position
			else
			{
				tac302.axisInfo[axis].patternCounter --;
				if (tac302.axisInfo[axis].patternCounter < 0)
				{
					tac302.axisInfo[axis].patternCounter = (NUMOFPATTERN -1);
				}
				tac302.axisInfo[axis].pattern = patterns[tac302.axisInfo[axis].patternCounter];
				setStepper(tac302.axisInfo[axis].pattern, axis);
				tac302.axisInfo[axis].currentPosition -= STEPPERRES;
			}	//eo tac302.axisInfo[tac302.currentAxis].position > tac302.axisInfo[tac302.currentAxis].currentPosition
		}	// eo abs(tac302.axisInfo[tac302.currentAxis].position - tac302.axisInfo[tac302.currentAxis].currentPosition) > 1	
}	//eo routineExecution::

	// --- Routine Execution for 1 step ---
/*--- majorPostionLed ----------------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Set one the LED for the major position beeing executed in AUTO mnode, if in manual
			mode all 4 LEDs are set to LOW.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void majorPostionLed (void)
{
	if (tac302.operationMode == MANUAL)
	{
		// Resetting Major Positions LED when in manual mode
		LATB = (LATB & MAJORMASK);
	}
	else
	{
		// Setting Major Positions LED while in automatic mode
		LATB = ((LATB & MAJORMASK) | (patterns[tac302.majorPosition]) << 4);		
	}
}	// eo majorPostionLed::

	// --- Routine Execution for 1 step ---
/*--- tapOrHold ----------------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Check during half-second if the button was released in any time, to diferenciate
			a tap and a hold in that button.
Input: 		None
Returns:	char - 1 if the button was hold, and 0 if the button was tap
--------------------------------------------------------------------------------------------*/
char tapOrHold (char button)
{
	// HOLD vs TAP software differentiation
	timer0Setup(HALFSECFS);
	if (button == BTNSELECT)
	{
		while(!SELECT)
		{
			if(TMR0FLAG)
			{
				return TRUE;
			}
		}
		return FALSE;
	}
	else if (button == BTNADVPOS)
	{
		while(!ADVPOS)
		{
			if(TMR0FLAG)
			{
				return TRUE;
			}
		}
		return FALSE;
	}
}	// eo tapOrHold::

	// --- Routine Execution for 1 step ---
/*--- updateController ----------------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Created a structurized message and send it through UART2
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void updateController (char messageType)
{
	// Created the message for the controller
	messageCreation(messageType);
	// Send the message in the UART2
	sendStringUART2(messageBuffer);
}	// eo updateController::

	// --- Set positions from the Major Position Table ---
/*--- setPositionAuto ----------------------------------------------------------------------
Author:		LRibeiro
Date:		08 Jul, 2019	
Modified:	None
Desc:		Assign position from the major position table to tac properties
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void setPositionAuto (void)
{
	char index = 0;
	for (index = 0; index < AXES; index ++)
	{
		// Assign desired position to all three axis
		tac302.axisInfo[index].position = defaultPosition[tac302.majorPosition][index];
	}
}	// eo setPositionAuto::

	// --- Routine Execution for 1 step ---
/*--- shiftMajorPosition ----------------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Clock-wise shift between major positions
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void shiftMajorPosition (void)
{
	tac302.majorPosition ++;
	if (tac302.majorPosition > DROP)
	{
		tac302.majorPosition = HOME;
	}
	if (tac302.operationMode == AUTOMATIC)
	{
		setPositionAuto();
		updateController(REGULARMSG);
	}
}	// eo shiftMajorPosition::

	// --- Routine Execution for 1 step ---
/*--- shiftCurrentAxis ----------------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Clock-wise shift between selected axis
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void shiftCurrentAxis (void)
{
	// Shift the selected axis
	tac302.currentAxis ++;
	if (tac302.currentAxis > Z)
	{
		tac302.currentAxis = X;
	}
}	// eo shiftCurrentAxis::

	// --- Routine Execution for 1 step ---
/*--- shiftOperationMode ----------------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Clock-wise shift between operation mode
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void shiftOperationMode (void)
{
	tac302.operationMode ++;
	if (tac302.operationMode > AUTOMATIC)
	{
		tac302.operationMode = MANUAL;
	}
	if (tac302.operationMode == AUTOMATIC)
	{
		setPositionAuto();
		updateController(REGULARMSG);
	}
	else
	{
		updateController(REGULARMSG);
	}
}	// eo shiftOperationMode::

	// --- Routine Execution for 1 step ---
/*--- setHome2MajorPosition ----------------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Major position reset to HOME
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void setHome2MajorPosition (void)
{
	if (tac302.majorPosition != HOME)
	{
		tac302.majorPosition = HOME;
		if (tac302.operationMode == AUTOMATIC)
		{
			setPositionAuto();
			updateController(REGULARMSG);
		}
	}
}	// eo setHome2MajorPosition::

	// --- Check Major Position Reached ---
/*--- checkMajorPositionReached ----------------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Check if all there axis reached the positions of the major position being executed. 
			If true, wait 3 sec change to the next one.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void checkMajorPositionReached (void)
{
	// It means that all there axis reached the desired position.
	if (axisCheckFlag == 3)
	{
		// Increment the 3 seconds flag
		threeSecFlag ++;

		// Toggle the tool according to the position setting
		TOOLLED = defaultPosition[tac302.majorPosition][TOOL];

		// If the three axes are in position and 3 seconds have passed move to the next position
		if (threeSecFlag == THREESECOUNT)
		{
			threeSecFlag = FALSE;
			shiftMajorPosition();
		}
	}
}	// eo checkMajorPositionReached::

	// --- Auto Mode Stepper Operation ---
/*--- autoModeStepperOperation --------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Assign major position's value to all the axis, rotate the motor and verify if
			all theree axis have reached the assigned position.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void autoModeStepperOperation (void)
{
	int index = 0;
	// Reset the Check Flag, which counts up to 3
	axisCheckFlag = 0;
	for (index = 0; index < AXES; index ++)
	{
		// If a current position in a axis is equal to the desired one, increment the flag
		if(abs(tac302.axisInfo[index].currentPosition - tac302.axisInfo[index].position) < MINDEG4STEP)
		{
			axisCheckFlag ++;
		}
		// Send the patterns to the stepper of each of three axis and update the data in the structure
		routineExecution(index);
	}
}	// eo autoModeStepperOperation::

	// --- Update Termian Each One Sec ---
/*--- updateTermianlEachOneSec ---------------------------------------------------------------
Author:		LRibeiro
Date:		30 Jun, 2019	
Modified:	None
Desc:		Update the terminal each 1 sec if the timer which call this function is set to 0.5 secs.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void updateTermianlEachOneSec (void)
{
	char test = 0;
	timerCount ++;
	if (timerCount == ONESECOUNT)
	{
		timerCount = FALSE;
		displayTerminal();
	}

}	// eo updateTermianlEachOneSec::

	// --- Angle Assignment Routine ---
/*--- angleAssignemnt ---------------------------------------------------------------
Author:		LRibeiro
Date:		09 Aug, 2019	
Modified:	None
Desc:		Enter in a alternative routine to manully input new values for the major position
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void angleAssignemnt (void)
{
	// Start of the Angle assignment routine
	while (angleAssignmentFlag)
	{
		// Read the ADC binary value in each loop of the program and convert it to degrees.
		desiredPosition = binary2Degree(getAdc());
		// Exit Condition
		if (!ADVPOS)
		{
			Delay10KTCYx( 5 );
			if (!ADVPOS)
			{
				tapOrHoldFlag = tapOrHold(BTNADVPOS);
				// HOLD
				if (tapOrHoldFlag)
				{
					// Exit the Angle Assignment mode
					angleAssignmentFlag = FALSE;
				}
			}
		}
		// Change the Major Position to be configured if hold, and the axis if tap
		if(!SELECT)
		{
			Delay10KTCYx( 5 );
			if(!SELECT)
			{
				tapOrHoldFlag = tapOrHold(BTNSELECT);
				// HOLD
				if (tapOrHoldFlag)
				{
					// Shift the operation mode
					majorPosIndex ++;
					if (majorPosIndex > DROP)
					{
						majorPosIndex = HOME;
					}
				}
				// TAP
				else
				{
					// Shift the selected axis
					axisIndex ++;
					if (axisIndex > TOOL)
					{
						axisIndex = X;
					}
				}
				// Reset the Flag
				tapOrHoldFlag = FALSE;
			}
		}
		// Assign the potentiometer value to the selected position, or ON/OFF to the toll
		if(!SET)
		{
			Delay10KTCYx( 5 );
			if (!SET)
			{
				// Axis assignment
				if (axisIndex != TOOL)
				{
					defaultPosition[majorPosIndex][axisIndex] = desiredPosition;
				}
				// Tool assignment
				else
				{
					// If the user select a angle higher than Zero the Tool is ON, else Tool is OFF
					if (desiredPosition > 0)
					{
						defaultPosition[majorPosIndex][axisIndex] = TRUE;
					}
					else 
					{
						defaultPosition[majorPosIndex][axisIndex] = FALSE;
					}
				}
			}
		}	
		if (TMR0FLAG)
		{
			// Timer is reset here if the pattern is not changing
			timer0Setup(HALFSECFS);

			// Update the terminal screen each 1 second since the timer is set for 0.5 secs
			updateTermianlEachOneSec();
		}
	}
	updateController(MAJORCHGMSG);
}	// eo angleAssignemnt::

		// --- Initialize System ---
/*--- initializeSystem ----------------------------------------------------------------------
Author:		LRibeiro
Date:		13 Mar, 2019
Modified:	None
Desc:		Call all the configuration functions that will be used on the program, the functions
			to initialize values into structures and arrays and print a message on the terminal 
			when the system is ready.
Input: 		None
Returns:	None
--------------------------------------------------------------------------------------------*/
void initializeSystem(void)
{
	set_osc_p18f45k22_4MHz();			// Set the processor speed
	portConfig();						// Set PIN configuration								
	initializeADC();					// Prepare the ADC module of the processor
	configSerial1();
	configSerial2();
	configTimer0();
	initializeController(&tac302);
	initializePositions();
	printf("System Ready...\n\r");
}	// eo initializeSystem::
	

/*--- MAIN FUNCTION -------------------------------------------------------------------------
-------------------------------------------------------------------------------------------*/

void main(void)
{
	// Call configuration functions and memory initialization.
	initializeSystem();
	
	// Infinite loop program
	while (TRUE)
	{
		// Read the ADC binary value in each loop of the program and convert it to degrees.
		desiredPosition = binary2Degree(getAdc());
		if(RCONbits.TO == 0)
		{
			shiftOperationMode();
		}
		// Push button debouncing is used for each push buttons.
		// Check the status, wait 100 ms and check it again.
		// Return the for the HOME major position
		if(!RESET)
		{
			Delay10KTCYx( 5 );
			if (!RESET)
			{	
				// Return to the Home major position
				setHome2MajorPosition();
			}
		}

		// Move to the next major position
		if(!ADVPOS)
		{
			Delay10KTCYx( 5 );
			if (!ADVPOS)
			{
				if (tac302.operationMode == MANUAL)
				{
					tapOrHoldFlag = tapOrHold(BTNADVPOS);
					// HOLD
					if (tapOrHoldFlag)
					{
						angleAssignmentFlag = 1;
						// Shift the operation mode
						angleAssignemnt();
					}
					else 
					{
						// Move to the next major position
						shiftMajorPosition();						
					}
					// Reset the Flag
					tapOrHoldFlag = FALSE;
				}
				else
				{
					// Move to the next major position
					shiftMajorPosition();
				}
			}
		}

		// Set the value define in the pontentiometer to the current axis position
		if(!SET)
		{
			Delay10KTCYx( 5 );
			if (!SET)
			{
				// Set the ADC selectable position to the position in the selected axis
				tac302.axisInfo[tac302.currentAxis].position = desiredPosition;

				if (tac302.operationMode == MANUAL)
				{
					// Send update message to the mBed controller (Set button pressed && Manual Mode active)
					updateController(REGULARMSG);
				}
			}
		}

		// Two functions Push-button: if hold change the operation mode, if tap change the axis selected.
		if(!SELECT)
		{
			Delay10KTCYx( 5 );
			if(!SELECT)
			{
				tapOrHoldFlag = tapOrHold(BTNSELECT);
				// HOLD
				if (tapOrHoldFlag)
				{
					// Shift the operation mode
					shiftOperationMode();
				}
				// TAP
				else
				{
					// Shift the selected axis
					shiftCurrentAxis();
				}
				// Reset the Flag
				tapOrHoldFlag = FALSE;
			}
		}

		// The timer controls the display update 2:1 and the motor rotation 1:1
		if (TMR0FLAG)
		{
			// Timer is reset here if the pattern is not changing
			timer0Setup(HALFSECFS);

			// Update the terminal screen each 1 second since the timer is set for 0.5 secs
			updateTermianlEachOneSec();
			
			// MANUAL MODE: 1 AXIS AT TIME OPERATION AND MAJOR POSITION LEDS OFF
			if (tac302.operationMode == MANUAL)
			{
				// Rotate the stepper and update the position
				routineExecution(tac302.currentAxis);
				// Resetting Major Positions LEDs when in manual mode
				majorPostionLed();
			} // eo if (MANUAL MODE)

			// AUTOMATIC MODE 3 AXES OPERATION, CURRENT MAJOR POSITION LED SET AND TOOL TOGGLING
			else
			{
				// Check is all axes reached the position desired, wait 3 sec and change to the next
				checkMajorPositionReached();
				// Assign position to each axis
				setPositionAuto();
				// Send the pattern to the stepper and update the step taken in the TAC
				autoModeStepperOperation();
				// Setting Major Positions LED while in automatic mode
				majorPostionLed();
			} // eo else (AUTOMATIC MODE)
		} // eo TMR0FLAG
	} // eo while
// eo main
}
