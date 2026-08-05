/****************************************************************************
TinyFSK Version 2.2.0
Copyright (C) 2013-2015 Andrew T. Flowers K0SM

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
****************************************************************************
2.28.0: ZAW_01: FSK_PIN is now held literally LOW (not the configured
        "mark" polarity level, which could be HIGH) for the entire
        duration of an RTS-sourced TX--at key-up in setPTT(), at unkey in
        setPTT() (covers TX_ABORT while RTS is still asserted), and at the
        inhibit force-stop path in loop(). No effect on TNC-sourced TX.
Revisions:
2.27.0: ZAW_01: Added lastPttWasRts, a sticky version of rtsKeyed for the
        LCD source label only. Line 1 now keeps showing "RTS DIGI" across
        any number of subsequent RX periods after an RTS-sourced TX, not
        just during it--switching back to "FSK ..." only once the UART
        actually receives a data byte bound for FSK transmission (see
        addToSendBuffer()). rtsKeyed itself is unchanged and still governs
        the actual bit-banging skip in processHalfBit().
2.26.0: ZAW_01: Fixed a bug where FSK_PIN was actively bit-banging idle
        diddle characters for the entire duration of an RTS-sourced TX
        (PTT_USB_RTS_PIN), contradicting the documented behavior that
        RTS-sourced keying doesn't use the internal Baudot/FSK generator
        at all. processHalfBit() now also returns immediately when
        rtsKeyed is true, leaving FSK_PIN at its static post-key-up mark
        level for the whole transmission instead.
2.25.0: ZAW_01: Added serial commands D/d to enable/disable live TX text
        on the status LCD (persisted to EEPROM, default on). When off,
        getNextSendChar() skips the lcdAppendTxChar() call entirely--no
        I2C traffic at all from that call site, not just a blanked
        display. Added a matching toggle to tools/config-tool.html.
2.24.0: ZAW_01: Removed the pre-TX text preview on the status LCD (loop()'s
        serial handler and waitDrainingSerial(), renamed from
        waitWithTxPreview(), no longer call lcdAppendTxChar()--still queue
        text into the send buffer, just don't show it early). Line 2 now
        shows *only* live text, one character at a time as it's actually
        transmitted, starting from a blank line for every new TX.
2.23.0: ZAW_01: Fixed a bug where the LCD's pre-TX text preview and the
        live during-TX text display could show the same characters twice
        (once quickly during the PA/PTT lead delay, again slowly as they
        were actually transmitted). setPTT(true) now clears line 2 right
        before real bit-banging starts, so the live display always starts
        clean. lcdResetTxLine() now actually blanks the LCD line, not just
        the internal column counter.
2.22.0: ZAW_01: PTT_PIN and PTT_PA_PIN are now forced LOW (unkeyed) as the
        very first thing in setup(), before Serial/EEPROM/timer/LCD--
        previously PTT_PA_PIN had no explicit boot-time LOW write at all
        (relied only on the implicit post-reset register default), and
        PTT_PIN's was set much later in setup(). PTT can still only ever
        be commanded to key up once loop() starts, which is inherently
        after setup() (including the full boot splash) completes.
2.21.0: ZAW_01: Restored live per-character TX text on the status LCD
        (line 2 updates as each character is actually sent to FSK_PIN,
        TNC-sourced only), after evaluating and rejecting word-batched
        alternatives (worse: unbounded flush duration for long words).
        This is a deliberate, measured, user-accepted timing tradeoff--see
        the notes on getNextSendChar() and lcdAppendTxChar(): ~1.9ms of
        I2C traffic per character (~3.8ms on a line-wrap character),
        shortening whatever bit it lands on by ~9-28%, worst at 75 baud.
2.20.0: ZAW_01: Two status LCD fixes. (1) CPU_INH_PIN now reflected on the
        LCD continuously and immediately (edge-triggered via inhibitShown
        in loop()), not just when it happens to block or interrupt a TX--
        previously, asserting it while idle showed nothing until a TX was
        attempted. (2) Line 2 no longer shows a literal "TX" during
        transmission--setPTT() now only refreshes line 1 (lcdRefreshLine1())
        at key-up, leaving whatever text the pre-TX preview wrote on line 2
        visible for the rest of that transmission. Also renamed the
        inhibited-state label from "INHIBITED" to "INHIBIT".
2.19.0: ZAW_01: TX text preview on the status LCD now also catches text
        that arrives in the same burst as TX_ON (the common case with
        N1MM etc., which send '[' + message back-to-back)--drained and
        previewed during setPTT()'s PA/PTT lead delay via the new
        waitWithTxPreview(), still before this TX's first FSK bit is due.
        Control characters (TX_ABORT/TX_END/COMMAND_ESCAPE/TX_ON) arriving
        in that window are left untouched for loop() to handle as usual.
2.18.0: ZAW_01: Boot splash PTT/PA timing screens now spell out "lead"/
        "tail" instead of "L"/"T" (e.g. "PTT lead:200ms").
2.17.0: ZAW_01: Removed the last serial output tied to an inhibited stop
        (the "cmd:" line sent when CPU_INH_PIN force-stopped an in-progress
        TX). Nothing is sent to the PC for an inhibited key-up attempt or
        an inhibited mid-TX stop--only the LCD ("INHIBITED") reflects it.
2.16.0: ZAW_01: Fixed a bug where changing baud rate or polarity via the
        ~ config menu didn't refresh the status LCD--line 1's baud/polarity
        field would show stale values until some unrelated update (PTT
        transition, callsign change) happened to redraw it.
2.15.0: ZAW_01: Status LCD line 1 TNC-sourced label changed from "TNC" to
        "FSK" (e.g. "FSK 45b H")--RTS-sourced label ("RTS DIGI") unchanged.
2.14.0: ZAW_01: Status LCD line 2 text display moved from "live during TX"
        (2.12.0, which measurably shortened FSK start bits, see prior
        entry) to "preview of queued text before TX starts"--now driven
        from the serial-receive handler in loop() while !ptt, never from
        inside processHalfBit()'s call chain. Genuinely zero timing impact,
        at the cost of no longer updating once a TX is actually underway.
2.13.0: ZAW_01: Removed the "inh:" serial status line sent when a TX is
        blocked or cut short by CPU_INH_PIN. The inhibit behavior itself
        (blocking/force-stopping keying) and the LCD "INHIBITED" display
        are unchanged--only the serial notification was removed.
2.12.0: ZAW_01: Status LCD line 2 shows the live TX text again for a
        TNC-sourced TX, in place of "TX"--this time as a single-character
        write per character (not a full-line redraw) to keep the residual
        I2C time inside the bit-timing path as small as possible.
2.11.0: ZAW_01: Added LCD boot splash (setup()-only, before Serial "cmd:"
        ready signal): brand for 2s, FW version for 1s, then PTT lead/tail,
        PA lead/tail, and baud/polarity, 1.5s each.
2.10.0: ZAW_01: Status LCD layout: callsign now columns 1-6, one blank
        separator column, PTT source detail now columns 8-16: "TNC " +
        2-digit baud + "b " + polarity when TNC-sourced (e.g. "TNC 45b H"),
        "RTS DIGI" when RTS-sourced.
2.9.0:  ZAW_01: Status LCD line 1, columns 10-16, now shows the current PTT
        source with detail: when TNC-sourced, "TN" + 2-digit baud rate +
        "b" + FSK polarity (H/L); when RTS-sourced, "RT" + "DIGI".
2.8.0:  ZAW_01: Status LCD line 1, columns 10-12, now shows the current PTT
        source: "TNC" (this board's own serial/Baudot engine) or "RTS"
        (PTT_USB_RTS_PIN, an external TNC's RTS line).
2.7.0:  ZAW_01: Added CALLSIGN (serial command C, max 6 chars), persisted to
        EEPROM. Shown on status LCD line 1, columns 1-6 (falls back to
        "CALL" when not set). Not used for auto-ID, just stored/displayed.
2.6.0:  ZAW_01: Added support for an external 16x2 I2C status LCD (PCF8574
        backpack, address 0x27, on the Nano's hardware I2C pins). Shows
        RX/TX/INHIBITED state; wording/layout to be refined later.
2.5.0:  ZAW_01: Added CPU_INH_PIN (Arduino D3 / ATmega328 PD3) hardware
        inhibit input, internal pull-up, active LOW. Blocks PTT/FSK keying
        while asserted, force-stops an in-progress TX immediately if
        asserted mid-transmission, and notifies the PC with an "inh:"
        status line whenever a TX is blocked or cut short by it.
2.4.0:  ZAW_01: Added LED_RX_PIN (Arduino D4 / ATmega328 PD4) receive
        indicator LED output, active HIGH, always opposite of PTT_PIN.
2.3.0:  ZAW_01: Added PTT_USB_RTS_PIN (Arduino D2 / ATmega328 PD2) hardware
        PTT-request input, external pull-up, active LOW. Lets an external/
        alternate TNC key through the PA/PTT lead-tail sequencer without
        going through this board's serial port or internal FSK generator.
2.2.0:  ZAW_01: Serial set/get commands (L, T, l, t) for PTT/PA lead and tail
        timing, persisted to EEPROM.
2.1.0:  ZAW_01: PTT sequencer
1.1.0:  Make "Robust UnShift On Space" transmission to be compatible with MMTTY's
        non-USOS default receiver.  It is effectively non-USOS transmission
        with extra FIGS shifts if a figs character appears after a space. This should
        always print properly on by USOS and non-USOS demodulators at the expense of
        having to send a few extra symbols in some contest exchages.

        Version information displayed at beginning of configuration screen so people
        can tell what version of firmware they have.

1.0.1:  Swap FSK and PTT pins to make pin-compatible with K3NG "nanokeyer"
1.0.0:  Initial release
************************************************************************************/

#include "Arduino.h"
#include "TimerOne.h"
#include "EEPROM.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"
#include <string.h>

#define FW_VERSION "2.28.0"
#define VERSION FW_VERSION " - OK2ZAW mods."

// OK2ZAW mod: external status LCD--16x2 character display on a PCF8574
// I2C backpack. Wired to the Nano's hardware I2C pins (A4=SDA, A5=SCL).
#define LCD_I2C_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

//Arduino pins for PTT and FSK to control transmitter
#define FSK_PIN 11
#define PTT_PIN 13
#define PTT_PA_PIN 10
#define ON_PIN 8

// OK2ZAW mod: hardware PTT-request input, e.g. from the RTS line of an
// external/alternate TNC's USB-serial adapter.  Lets that external TNC key
// through this board's PA/PTT lead-tail sequencer without going through
// this board's own serial port or internal FSK/Baudot generator.
// Arduino pin D2 = ATmega328 pin 32 (PD2).  External pull-up on the line;
// active LOW (asserted/PTT-requested = LOW).
#define PTT_USB_RTS_PIN 2

// OK2ZAW mod: receive indicator LED.  Active HIGH; always the opposite
// state of PTT_PIN (lit while receiving, off while transmitting).
// Arduino pin D4 = ATmega328 pin 2 (PD4).
#define LED_RX_PIN 4

// OK2ZAW mod: hardware inhibit input.  While asserted (LOW), the firmware
// refuses to key PTT_PIN/PTT_PA_PIN and will not clock any data out
// FSK_PIN--checked before every key-up, and polled continuously so an
// in-progress TX is force-stopped immediately if this becomes asserted
// mid-transmission. Shown on the status LCD ("INHIBITED"); no serial
// notification is sent for it.
// Internal pull-up, so a disconnected pin defaults to "not inhibited".
// Arduino pin D3 = ATmega328 pin 1 (PD3).  Active LOW.
#define CPU_INH_PIN 3

//EEPROM addresses to persist configuration
#define EE_SPEED_ADDR 0
#define EE_POLARITY_ADDR 1
#define EE_PTT_LEAD_ADDR 2     // int (2 bytes): pttLeadMillis
#define EE_PTT_TAIL_ADDR 4     // int (2 bytes): pttTailMillis
#define EE_PA_LEAD_ADDR 6      // int (2 bytes): ptt_PA_LeadMillis
#define EE_PA_TAIL_ADDR 8      // int (2 bytes): ptt_PA_TailMillis
#define EE_CALLSIGN_ADDR 10    // null-terminated string, up to CALLSIGN_MAX_LEN chars + NUL
#define EE_LIVE_LCD_ADDR 17    // 1 byte: raw COMMAND_LIVE_LCD_ON/OFF char

// Sane range for the millisecond timing values above--used to detect
// blank/uninitialized EEPROM (which reads as -1) and reject garbage.
#define TIMING_MIN_MS 0
#define TIMING_MAX_MS 9999

// OK2ZAW mod: operator callsign, settable over serial and persisted to
// EEPROM. Not used by the TX engine (no auto-ID)--just stored/reported and
// shown on the status LCD's line 1, columns 1-6 (see lcdShowStatus()).
#define CALLSIGN_MAX_LEN 6

//Special Baudot symbols for shift
#define LTRS_SHIFT 0x1F  //baudot letter shift byte
#define FIGS_SHIFT 0x1B  //baudot figs shift byte

#define SHIFT_UNKNOWN 0  //Undefined shift--used at TX start to force shift state

//Special ASCII SYMBOLS (8 bit)
#define ASCII_NULL 0x00
#define ASCII_LF 0x0A
#define ASCII_CR 0x0D

//BUFFER SETTINGS
#define SEND_BUFFER_SIZE 500   // Allow up to 500 chars in the buffer
                               // before overrunning (wrapping around).
                               // This can be increased on most boards
                               // with more RAM.

#define TX_END_FLAG 0xFF      // Used in Baudot stream to indicate EOT

//References used in banging out the bits for 5-bit baudot. These
//are relative to the first data bit in the frame.
#define START_BIT_POS -1
#define STOP_BIT_POS 5

//Commands that control transmitter sequencing
#define TX_ON '['   // TX now => {TX} in N1MM
#define TX_END ']'  // Buffered switch to RX => {END} in N1MM
#define TX_ABORT '\\' // (Backslash) Immediate switch to RX and clear buffer => {ESC} in N1MM

//Configuration commands.  These are also the values saved in the EEPROM.
#define COMMAND_ESCAPE '~'
#define COMMAND_POLARITY_MARK_HIGH '0'
#define COMMAND_POLARITY_MARK_LOW  '1'
#define COMMAND_45BAUD '4'
#define COMMAND_50BAUD '5'
#define COMMAND_75BAUD '7'
#define COMMAND_DUMP_CONFIG '?'
#define COMMAND_SET_PTT_LEAD 'L'  // main relay: time before first start bit
#define COMMAND_SET_PTT_TAIL 'T'  // main relay: time after last stop bit
#define COMMAND_SET_PA_LEAD  'l'  // PA/amp stage: time before main relay
#define COMMAND_SET_PA_TAIL  't'  // PA/amp stage: time after main relay
#define COMMAND_SET_CALLSIGN 'C'  // operator callsign
#define COMMAND_LIVE_LCD_ON  'D'  // enable live TX text on the status LCD
#define COMMAND_LIVE_LCD_OFF 'd'  // disable live TX text on the status LCD

// Stop bit settings
#define STOP_BITS_1     1    // 1 stop bit
#define STOP_BITS_1R5   2    // 1.5 stop bits
#define STOP_BITS_2     3    // 2 stop bits

// TX USOS settings
#define USOS_OFF  1       //Assumes that RX will not reset to LTRS shift after space
                          //All shift symbols are explicit and spaces do not change
                          //shift state:
                          //  K0SM 599 05 NY NY
                          //     --> <LTR>K<FIG>0<LTR>SM <FIG>599 05 <LTR>NY NY

#define USOS_ON 2         //Space is an implicit LTRS shift character.  "Ham standard"
                          //demodulators operate in this mode.
                          //  K0SM 599 05 NY NY
                          //     --> <LTR>K<FIG>0<LTR>SM <FIG>599 <FIG>05 NY NY

#define USOS_MMTTY_HACK 3 //Essentially USOS OFF plus extra FIGS shifts for all words
                          //starting with numbers.  This is what MMTTY somewhat misleadingly
                          //calls "USOS transmission."  This settings makes many contest exchanges
                          //longer (and slightly more prone to bit errors) all to be
                          //compatible with MMTTY's non-USOS RX default
                          //  K0SM 599 05 NY NY
                          //     --> <LTR>K<FIG>0<LTR>SM <FIG>599 <FIG>05 <LTR>NY NY


/******************************************************
     Function prototypes
*******************************************************/
void handleConfigurationCommand(byte b);
void startNumericEntry(byte cmd);
void handleNumericEntry(byte b);
void applyNumericEntry();
void startCallsignEntry();
void handleCallsignEntry(byte b);
void applyCallsignEntry();
void eeLoad();
void initTimer();
void timerISR();
void displayConfigurationPrompt();
void displayConfiguration();
void processHalfBit();
void resetChar();
void resetSendBuffer();
void addToSendBuffer(byte newByte);
byte getNextSendChar();
boolean requiresLetters(byte asciiByte);
boolean requiresFigures(byte asciiByte);
void waitDrainingSerial(unsigned long ms);
void setPTT(byte b);
boolean isInhibited();
void lcdRefreshLine1();
void lcdShowStatus(const char* line2);
void lcdShowSplash();
void lcdResetTxLine();
void lcdAppendTxChar(byte b);
void echo(byte b);

/******************************************************
     Variable declarations
*******************************************************/
// Mapping of ascii to baudot symbols.  This is the
// translation table that maps an incoming ASCII byte
// on the serial interface to a equivalent (or reasonable
// substitute) that exists the ITA2 or US 5-bit code.
// In general, any ASCII control character will be mapped
// to a Baudot NULL.  Punctuation will be mapped to '?' if
// there is no equivalent in the Baudot set.  Note that
// some punctuation such as '[', ']' and '\' are used
// to control the PTT behavior.  Tilda (~) is used
// to enter the configuration menu.  You can use
// your imagination to add other control functions here.
int asciiToBaudot[127] = {

  ////      ASCII                  ASCII IDX (decimal)
  0,//	Null character	          //	0
  0,//	Start of Header	          //	1
  0,//	Start of Text	            //	2
  0,//	End of Text	              //	3
  0,//	End of Transmission	      //	4
  0,//	Enquiry	                  //	5
  0,//	Acknowledgment	          //	6
  5,//	Bell	                    //	7
  0,//	Backspace	                //	8
  0,//	Horizontal Tab  	        //	9
  2,//	Line feed	                //	10
  0,//	Vertical Tab	            //	11
  0,//	Form feed	                //	12
  8,//	Carriage return  	        //	13
  0,//	Shift Out	                //	14
  0,//	Shift In	                //	15
  0,//	Data Link Escape	        //	16
  0,//	Device Control 1 	        //	17
  0,//	Device Control 2	        //	18
  0,//	Device Control 3 	        //	19
  0,//	Device Control 4	        //	20
  0,//	Negative Acknowledgement  //	21
  0,//	Synchronous idle	        //	22
  0,//	End of Transmission Block //	23
  0,//	Cancel	                  //	24
  0,//	End of Medium	            //	25
  0,//	Substitute	              //	26
  0,//	Escape  	                //	27
  0,//	File Separator	          //	28
  0,//	Group Separator	          //	29
  0,//	Record Separator	        //	30
  0,//	Unit Separator	          //	31
  4,//	space	                  //	32
  13,//	!	                  //	33
  17,//	"	                  //	34
  20,//	#	                  //	35
  9,//	$	                  //	36
  25,//	%	                  //	37
  26,//	&	                  //	38
  11,//	'	                  //	39
  15,//	(	                  //	40
  18,//	)	                  //	41
  25,//	*	                  //	42
  17,//	+	                  //	43  //ITA2
  12,//	,	                  //	44
  3,//	-	                  //	45
  28,//	.	                  //	46
  29,//	/	                  //	47
  22,//	0	                  //	48
  23,//	1	                  //	49
  19,//	2	                  //	50
  1,//	3	                  //	51
  10,//	4	                  //	52
  16,//	5	                  //	53
  21,//	6	                  //	54
  7,//	7	                  //	55
  6,//	8	                  //	56
  24,//	9	                  //	57
  14,//	:	                  //	58
  30,//	;	                  //	59
  25,//	<	                  //	60
  30,//	=	                  //	61 //ITA2
  25,//	>	                  //	62
  25,//	?	                  //	63
  25,//	@	                  //	64
  3,//	A	                  //	65
  25,//	B	                  //	66
  14,//	C	                  //	67
  9,//	D	                  //	68
  1,//	E	                  //	69
  13,//	F	                  //	70
  26,//	G	                  //	71
  20,//	H	                  //	72
  6,//	I	                  //	73
  11,//	J	                  //	74
  15,//	K	                  //	75
  18,//	L	                  //	76
  28,//	M	                  //	77
  12,//	N	                  //	78
  24,//	O	                  //	79
  22,//	P	                  //	80
  23,//	Q	                  //	81
  10,//	R	                  //	82
  5,//	S	                  //	83
  16,//	T	                  //	84
  7,//	U	                  //	85
  30,//	V	                  //	86
  19,//	W	                  //	87
  29,//	X	                  //	88
  21,//	Y	                  //	89
  17,//	Z	                  //	90
  15,//	[ Used to start TX        //	91
  20,//	\ Used to escape TX       //	92
  18,//	] Buffered end TX         //	93
  25,//	^	                  //	94
  4,//	_	                  //	95
  25,//	`	                  //	96
  3,//	a	                  //	97
  25,//	b	                  //	98
  14,//	c	                  //	99
  9,//	d	                  //	100
  1,//	e	                  //	101
  13,//	f	                  //	102
  26,//	g	                  //	103
  20,//	h	                  //	104
  6,//	i	                  //	105
  11,//	j	                  //	106
  15,//	k	                  //	107
  18,//	l	                  //	108
  28,//	m	                  //	109
  12,//	n	                  //	110
  24,//	o	                  //	111
  22,//	p	                  //	112
  23,//	q	                  //	113
  10,//	r	                  //	114
  5,//	s	                  //	115
  16,//	t	                  //	116
  7,//	u	                  //	117
  30,//	v	                  //	118
  19,//	w	                  //	119
  29,//	x	                  //	120
  21,//	y	                  //	121
  17,//	z	                  //	122
  15,//	{	                  //	123
  20,//	|	                  //	124
  18,//	}	                  //	125
  25 //	~ Command escape char     //	126
};

/*******************************************************
This section defines static runtime variables that affect
RTTY transmission.  They are NOT directly changeable by user
commands because they can get ops into trouble.  They are
here for the tinkerer/experimenter in case you want access
to them at runtime.
********************************************************/

long serialSpeed = 9600; //This is the speed for the serial
                         //(more likely USB) connection, 8-N-1

// Not user selectable, but USOS behavior can be changed here.
// We set this to TX extra shifts to be compatible with silly
// MMTTY default reciever
int usos = USOS_MMTTY_HACK;

int stopBits =  STOP_BITS_1R5;  // TX 1.5 stop bits

/***************************************
Dynamic runtime variables these are minipulated with
user commands or during normal TX operation.
*****************************************/
float baudrate = 45.45;  //default--can be changed by user command

int pttLeadMillis = 150; //time before first start bit
int pttTailMillis = 25;  //time after last stop bit

// OK2ZAW MOD PTT sequencer *** TOTAL sequencing is ptt_PA_LeadMillis + pttLeadMillis
int ptt_PA_LeadMillis = 80; //time before first start bit
int ptt_PA_TailMillis = 80;  //time after last stop bit

// OK2ZAW mod: enables/disables the live TX-text write in getNextSendChar()
// (see lcdAppendTxChar())--set/get via COMMAND_LIVE_LCD_ON/OFF ('D'/'d').
// Default on, matching prior behavior; off skips that I2C write entirely.
boolean liveLcdTextEnabled = true;


// Polarity--changed with user commands and stored in EEPROM
boolean mark = LOW;     //High indicates +V on the FSK pin
boolean space = HIGH;   //Low indicates 0V on the FSK pin

// Buffer management variables to handle TX text input
byte sendBufferArray[SEND_BUFFER_SIZE];  // size of TX buffer
byte sendBufferBytes = 0;    // number of bytes unsent in TX buffer
byte lastAsciiByteSent = 0;  // needed to echo back sent characters to terminal
boolean endWhenBufferEmpty = true;  //flag to kill TX when buffer empty (']')


byte currentShiftState = SHIFT_UNKNOWN;  //Keeps track of Letter/Figs state to determine
                                         //if we need to send shift chars

boolean ptt = false; // Keeps track of PTT state (true = Transmitter is on)

// OK2ZAW mod: true while the current TX was started by PTT_USB_RTS_PIN
// (rather than by the [/] serial commands), so we know it's ours to release.
boolean rtsKeyed = false;

// OK2ZAW mod: sticky version of the above, for the LCD source label only.
// Unlike rtsKeyed (which clears the moment the RTS-sourced TX ends), this
// stays true--continuing to show "RTS DIGI" on line 1--across any number
// of subsequent RX periods, until the UART actually receives a data byte
// bound for FSK transmission (see addToSendBuffer()). Set true the moment
// PTT_USB_RTS_PIN triggers a key-up.
boolean lastPttWasRts = false;

volatile boolean isrFlag = false;   //set by timer interrupt.  Set high every 1/2 bit
                                    //to indicate when we should exectute the bit-banging
                                    //routine.  This is handled in the main loop function.

boolean configurationMode = false;  //flag indicates if we are in the menu system or
                                    //in normal operation.

// State for the "set/get timing value" sub-mode of the configuration menu
// (commands L, T, l, t).  While numericEntryMode is true, incoming serial
// bytes are digits being accumulated into numericEntryValue rather than
// being treated as new configuration commands.
boolean numericEntryMode = false;
byte numericEntryCommand = 0;      // which command (L/T/l/t) triggered entry
int numericEntryValue = 0;         // accumulated value so far
boolean numericEntryHasDigits = false;  // false if user pressed Enter with no digits (a "get")

// OK2ZAW mod: operator callsign, settable via the C command and persisted
// to EEPROM. Not used by the TX engine itself--just stored/reported.
char callsign[CALLSIGN_MAX_LEN + 1] = "";

// State for the "set/get callsign" sub-mode of the configuration menu
// (command C), mirroring numericEntryMode above but for text.
boolean callsignEntryMode = false;
char callsignEntryBuffer[CALLSIGN_MAX_LEN + 1] = "";
byte callsignEntryLen = 0;
boolean callsignEntryHasChars = false;  // false if user pressed Enter with no chars (a "get")

// OK2ZAW mod: remembers the last line-2 status text passed to
// lcdShowStatus(), so other updates (e.g. a callsign change) can redraw
// line 1 without needing to know the current TX/RX state.
const char* lcdLastLine2 = "RX";

// OK2ZAW mod: true while the LCD is currently showing "INHIBIT"--tracks
// isInhibited() edge-to-edge in loop(), so the display reflects the pin
// continuously (including while idle), not just when it blocks/interrupts
// a TX attempt.
boolean inhibitShown = false;

// OK2ZAW mod: current write column (0-15) on LCD line 2 for the live TX
// text display--see lcdAppendTxChar(). Wraps back to 0 (and starts
// overwriting from the left again) once it reaches LCD_COLS.
byte lcdTxCol = 0;

/*********************************************************************
Main execution
***********************************************************************/

/**
* Exectutes *once* at program start (when power applied or
* reset button pressed.  Note that many Arudino devices have a
* "software reset" option that will reset the processor when
* the serial port is opened.
* It opens the port, configures the output pins, and loads
* configuration from EEPROM.
*/
void setup()
{
  // OK2ZAW: assert the safe (unkeyed) state on both PTT outputs as the very
  // first thing this sketch does--before Serial, EEPROM, the timer, or the
  // LCD. This is the earliest point our code can control them; the pins
  // are floating (not actively driven) for the brief window before this,
  // while the bootloader itself is running, which is outside firmware's
  // control since our code hasn't started yet. If that matters for your
  // PTT circuit, add an external pull-down resistor on PTT_PIN/PTT_PA_PIN
  // as a hardware-level guard for that window.
  pinMode(PTT_PIN, OUTPUT);
  pinMode(PTT_PA_PIN, OUTPUT);
  digitalWrite(PTT_PIN, LOW);
  digitalWrite(PTT_PA_PIN, LOW);

  Serial.begin(serialSpeed);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // configure remaining pins
  pinMode(FSK_PIN, OUTPUT);
  pinMode(ON_PIN, OUTPUT); // OK2ZAW
  pinMode(PTT_USB_RTS_PIN, INPUT); // OK2ZAW: external pull-up already on the line
  pinMode(LED_RX_PIN, OUTPUT); // OK2ZAW
  pinMode(CPU_INH_PIN, INPUT_PULLUP); // OK2ZAW
  eeLoad();
  displayConfiguration();
  // start the half-bit timer.
  initTimer();

  Wire.begin(); // OK2ZAW: I2C for the status LCD
  lcd.init();
  lcd.backlight();
  lcdShowSplash(); // OK2ZAW: ~7.5s boot splash (brand, FW version, PTT/PA/baud settings)

  Serial.write("\ncmd:\n"); // Tell N1MM we are in "RX" mode.  This will be sent
                            // at the end of transmission.
  digitalWrite(LED_RX_PIN, HIGH); // OK2ZAW: RX indicator on--opposite of PTT_PIN
  inhibitShown = isInhibited(); // OK2ZAW: correct initial LCD text if already inhibited at boot
  lcdShowStatus(inhibitShown ? "INHIBIT" : "RX"); // OK2ZAW
  // OK2ZAW: main status screen is now showing--this is the point [/RTS
  // can first actually key up (loop(), where those are handled, can't run
  // any earlier than this regardless--see the note at the top of setup()).
}


/**
* Main loop.  This loop does two things:
* (1) Process any input from the serial connection one byte at a time.
*
* (2) If the half-bit timer interrupt fired we need to execute the bit-banging
* routine to keep clocking out RTTY.
*/
void loop()
{

  // (0) OK2ZAW mod: hardware inhibit, edge-triggered against inhibitShown
  // so the LCD reflects it continuously--including while idle--not just
  // when it blocks/interrupts a TX attempt. If it becomes asserted while
  // we're transmitting, also force an immediate stop--bypassing the
  // configured PA/PTT tail delays entirely, since this is a safety
  // cutoff, not a graceful end of transmission. A blocked key-up attempt
  // is instead handled inside setPTT() itself (see isInhibited()).
  // Deliberately silent on the serial port either way--no "cmd:" or any
  // other line is sent for an inhibited stop, only the LCD reflects it.
  boolean inhibitedNow = isInhibited();
  if (inhibitedNow && !inhibitShown)
  {
    if (ptt)
    {
      digitalWrite(PTT_PA_PIN, LOW);
      digitalWrite(PTT_PIN, LOW);
      digitalWrite(LED_RX_PIN, HIGH);
      digitalWrite(FSK_PIN, rtsKeyed ? LOW : space); // OK2ZAW: hold FSK_PIN LOW, not "space", while RTS-sourced
      ptt = false;
      rtsKeyed = false;
      resetSendBuffer();
      endWhenBufferEmpty = true;
      resetChar();
      currentShiftState = SHIFT_UNKNOWN;
      lastAsciiByteSent = 0;
    }
    lcdShowStatus("INHIBIT"); // OK2ZAW
    inhibitShown = true;
  }
  else if (!inhibitedNow && inhibitShown)
  {
    inhibitShown = false;
    if (!ptt) lcdShowStatus("RX"); // OK2ZAW: only if not (still) transmitting
  }

  // (1) OK2ZAW mod: poll the external hardware PTT-request input.  This lets
  // an external/alternate TNC (which does its own FSK/AFSK generation, not
  // routed through this board) key through the same PA/PTT lead-tail
  // sequencer as the [/] serial commands, just via a hardware line instead.
  // Active LOW.  Skipped while in the configuration menu.
  if (!configurationMode && !numericEntryMode)
  {
    boolean rtsAsserted = (digitalRead(PTT_USB_RTS_PIN) == LOW);
    if (rtsAsserted && !ptt)
    {
      rtsKeyed = true;
      lastPttWasRts = true; // OK2ZAW: sticky--see declaration comment
      endWhenBufferEmpty = false;
      setPTT(true);
    }
    else if (!rtsAsserted && rtsKeyed)
    {
      rtsKeyed = false;
      endWhenBufferEmpty = true;
    }
  }

  // (2) Now read *one* byte from serial port if anything is there.
  // We only read one byte so as not to bog down the processor if
  // hundreds of bytes arrive all at once.  If there is more to read
  // it will be picked up once each time through the loop.
  if (Serial.available() > 0)
  {
    // get incoming byte:
    byte b = Serial.read();

    // if we're mid-entry of a numeric timing value (L/T/l/t commands), this
    // byte is a digit or the Enter key that commits it--handle that first.
    if (numericEntryMode)
    {
      handleNumericEntry(b);
    }
    // similarly, mid-entry of a callsign (C command)--handle that next.
    else if (callsignEntryMode)
    {
      handleCallsignEntry(b);
    }
    // if we are in configuration mode, this byte is likely intended to change
    // a configuration setting.
    else if (configurationMode)
    {
      handleConfigurationCommand(b);
    }
    else  //not in configuration mode
    {
      // check for TX abort character.  This immediately kills the
      // transmitter and dumps anything remaining in the buffer.
      if (b == TX_ABORT)
      {
        setPTT(false);
        resetSendBuffer();
        endWhenBufferEmpty = true;
        return;
      }
      else if (b == TX_ON)
      {
        // The set PTT method has a delay
        // in it so that there is a chance for another character to
        // arrive in the input buffer.  We return immediately here
        // so that we can pick it up at the top of this loop.  If we
        // didn't do this, we would like continue on, see the buffer
        //is empty, and transmit a diddle before the first real character.
        endWhenBufferEmpty = false;
        setPTT(true);
        return; //return to beginning of loop to pick up first char if any
      }
      else if  (b == TX_END)
      {
        endWhenBufferEmpty = true;
      }
      else if (b == COMMAND_ESCAPE)
      {
       configurationMode = true;
       displayConfigurationPrompt();
      }
      else  // character to TX, so add to send buffer
      {
        addToSendBuffer(b);
        // OK2ZAW: deliberately no LCD write here--only getNextSendChar()
        // shows text live, as each character is actually transmitted, so
        // every new TX starts from a blank line 2. See lcdResetTxLine()
        // and the note on getNextSendChar().
      }
    }
  }
  // (3) if the ISR fired we need may need to bit-bang something out the the FSK port
  if (isrFlag)
  {
    processHalfBit();
    isrFlag = false;
  }
}

/**
* Handles configuration change commands by changing variables
* and writing new values to EEPROM.
*/
void handleConfigurationCommand(byte b)
{
  switch (b)
    {
      case (COMMAND_POLARITY_MARK_HIGH):
      {
        mark = HIGH;
        space = LOW;
        EEPROM.write(EE_POLARITY_ADDR, b);
        break;
      }
      case (COMMAND_POLARITY_MARK_LOW):
      {
        mark = LOW;
        space = HIGH;
        EEPROM.write(EE_POLARITY_ADDR, b);
        break;
      }
      case (COMMAND_45BAUD):
      {
        baudrate = 45.45;
        initTimer();
        EEPROM.write(EE_SPEED_ADDR, b);
        break;
      }
      case (COMMAND_50BAUD):
      {
        baudrate = 50.0;
        initTimer();
        EEPROM.write(EE_SPEED_ADDR, b);
        break;
      }
      case (COMMAND_75BAUD):
      {
        baudrate = 75.0;
        initTimer();
        EEPROM.write(EE_SPEED_ADDR, b);
        break;
      }
      case (COMMAND_DUMP_CONFIG):
      {
        // we dump it out below
        break;
      }
      case (COMMAND_SET_PTT_LEAD):
      case (COMMAND_SET_PTT_TAIL):
      case (COMMAND_SET_PA_LEAD):
      case (COMMAND_SET_PA_TAIL):
      {
        // These commands need a numeric value typed in next, so hand off
        // to the numeric entry sub-mode instead of finishing the command here.
        startNumericEntry(b);
        return;
      }
      case (COMMAND_SET_CALLSIGN):
      {
        // Needs a text value typed in next--hand off to the callsign entry
        // sub-mode instead of finishing the command here.
        startCallsignEntry();
        return;
      }
      case (COMMAND_LIVE_LCD_ON):
      {
        liveLcdTextEnabled = true;
        EEPROM.write(EE_LIVE_LCD_ADDR, b);
        break;
      }
      case (COMMAND_LIVE_LCD_OFF):
      {
        liveLcdTextEnabled = false;
        EEPROM.write(EE_LIVE_LCD_ADDR, b);
        break;
      }
      default :
      {
        Serial.write("\nNot a recognized command. Exiting configuration mode.\n");
      }
    }
    lcdShowStatus(lcdLastLine2);  // OK2ZAW: refresh line 1 (baud/polarity affect the FSK/RTS field)
    displayConfiguration();
    configurationMode = false;
}

/**
* Begins reading a numeric (millisecond) value for one of the PTT/PA
* timing commands (L, T, l, t).  Subsequent bytes are handled by
* handleNumericEntry() until Enter is pressed.
*/
void startNumericEntry(byte cmd)
{
  numericEntryCommand = cmd;
  numericEntryValue = 0;
  numericEntryHasDigits = false;
  numericEntryMode = true;
  Serial.print(F("\nEnter new value in ms (0-9999) and press Enter,\nor press Enter alone to view the current value: "));
}

/**
* Accumulates digits typed after a set/get timing command.  Enter (CR or LF)
* commits the value; COMMAND_ESCAPE cancels without changing anything.
*/
void handleNumericEntry(byte b)
{
  if (b >= '0' && b <= '9')
  {
    if (numericEntryValue <= 999)  // cap accumulation at 4 digits (0-9999)
    {
      numericEntryValue = numericEntryValue * 10 + (b - '0');
      numericEntryHasDigits = true;
      echo(b);  // echo the digit back to the terminal
    }
  }
  else if (b == ASCII_CR || b == ASCII_LF)
  {
    applyNumericEntry();
  }
  else if (b == COMMAND_ESCAPE)
  {
    Serial.print(F("\nCancelled.\n"));
    numericEntryMode = false;
    configurationMode = false;
  }
  // any other byte is ignored while entering a numeric value
}

/**
* Applies (and persists to EEPROM) the value accumulated by
* handleNumericEntry(), or just re-displays the current configuration if
* the user pressed Enter without typing any digits (a "get").
*/
void applyNumericEntry()
{
  if (numericEntryHasDigits)
  {
    switch (numericEntryCommand)
    {
      case (COMMAND_SET_PTT_LEAD):
      {
        pttLeadMillis = numericEntryValue;
        EEPROM.put(EE_PTT_LEAD_ADDR, pttLeadMillis);
        break;
      }
      case (COMMAND_SET_PTT_TAIL):
      {
        pttTailMillis = numericEntryValue;
        EEPROM.put(EE_PTT_TAIL_ADDR, pttTailMillis);
        break;
      }
      case (COMMAND_SET_PA_LEAD):
      {
        ptt_PA_LeadMillis = numericEntryValue;
        EEPROM.put(EE_PA_LEAD_ADDR, ptt_PA_LeadMillis);
        break;
      }
      case (COMMAND_SET_PA_TAIL):
      {
        ptt_PA_TailMillis = numericEntryValue;
        EEPROM.put(EE_PA_TAIL_ADDR, ptt_PA_TailMillis);
        break;
      }
    }
  }
  displayConfiguration();
  numericEntryMode = false;
  configurationMode = false;
}

/**
* Begins reading a callsign for the C command.  Subsequent bytes are
* handled by handleCallsignEntry() until Enter is pressed.
*/
void startCallsignEntry()
{
  callsignEntryLen = 0;
  callsignEntryBuffer[0] = '\0';
  callsignEntryHasChars = false;
  callsignEntryMode = true;
  Serial.print(F("\nEnter new callsign (max 6 chars) and press Enter,\nor press Enter alone to view the current value: "));
}

/**
* Accumulates printable characters typed after the C command (uppercased).
* Enter (CR or LF) commits the value; COMMAND_ESCAPE cancels without
* changing anything.
*/
void handleCallsignEntry(byte b)
{
  if (b == ASCII_CR || b == ASCII_LF)
  {
    applyCallsignEntry();
  }
  else if (b == COMMAND_ESCAPE)
  {
    Serial.print(F("\nCancelled.\n"));
    callsignEntryMode = false;
    configurationMode = false;
  }
  else if (b >= 0x20 && b <= 0x7E && callsignEntryLen < CALLSIGN_MAX_LEN)  // printable ASCII
  {
    if (b >= 'a' && b <= 'z') b -= 32;  // normalize to uppercase
    callsignEntryBuffer[callsignEntryLen++] = (char) b;
    callsignEntryBuffer[callsignEntryLen] = '\0';
    callsignEntryHasChars = true;
    echo(b);  // echo the character back to the terminal
  }
  // any other byte (including overflow past CALLSIGN_MAX_LEN) is ignored
}

/**
* Applies (and persists to EEPROM) the callsign accumulated by
* handleCallsignEntry(), or just re-displays the current configuration if
* the user pressed Enter without typing any characters (a "get").
*/
void applyCallsignEntry()
{
  if (callsignEntryHasChars)
  {
    byte len = strlen(callsignEntryBuffer);
    for (byte i = 0; i < len; i++)
    {
      EEPROM.write(EE_CALLSIGN_ADDR + i, callsignEntryBuffer[i]);
    }
    EEPROM.write(EE_CALLSIGN_ADDR + len, 0);  // terminator--also masks any longer old value
    strcpy(callsign, callsignEntryBuffer);
    lcdShowStatus(lcdLastLine2);  // OK2ZAW: refresh line 1 with the new callsign
  }
  displayConfiguration();
  callsignEntryMode = false;
  configurationMode = false;
}

/**
* Loads speed, polarity, and PTT/PA timing from EEPROM
*/
void eeLoad()
{
  byte speedChar = EEPROM.read(EE_SPEED_ADDR);
  byte polarity  = EEPROM.read(EE_POLARITY_ADDR);

  if (polarity == COMMAND_POLARITY_MARK_LOW)
  {
    mark = LOW;
  } else {
    mark = HIGH;
  }
  space = !mark;

  switch (speedChar)
  {
      case (COMMAND_50BAUD):
      {
        baudrate = 50.0;
        break;
      }
      case (COMMAND_75BAUD):
      {
        baudrate = 75.0;
        break;
      }
      default:
      {
        baudrate = 45.45;
        break;
      }
  }

  // Timing values are only overridden if EEPROM holds something sane.
  // A blank/uninitialized EEPROM reads as 0xFFFF (-1 as a signed int), so
  // that case (and anything else out of range) falls back to the compiled-in
  // defaults already assigned to these variables above.
  int eeValue;

  EEPROM.get(EE_PTT_LEAD_ADDR, eeValue);
  if (eeValue >= TIMING_MIN_MS && eeValue <= TIMING_MAX_MS)
  {
    pttLeadMillis = eeValue;
  }

  EEPROM.get(EE_PTT_TAIL_ADDR, eeValue);
  if (eeValue >= TIMING_MIN_MS && eeValue <= TIMING_MAX_MS)
  {
    pttTailMillis = eeValue;
  }

  EEPROM.get(EE_PA_LEAD_ADDR, eeValue);
  if (eeValue >= TIMING_MIN_MS && eeValue <= TIMING_MAX_MS)
  {
    ptt_PA_LeadMillis = eeValue;
  }

  EEPROM.get(EE_PA_TAIL_ADDR, eeValue);
  if (eeValue >= TIMING_MIN_MS && eeValue <= TIMING_MAX_MS)
  {
    ptt_PA_TailMillis = eeValue;
  }

  // Callsign is a null-terminated string. Blank/uninitialized EEPROM reads
  // as 0xFF, and any non-printable byte (including a stored NUL) marks the
  // end of the string--so this naturally falls back to "" if never set.
  byte i = 0;
  while (i < CALLSIGN_MAX_LEN)
  {
    byte c = EEPROM.read(EE_CALLSIGN_ADDR + i);
    if (c < 0x20 || c > 0x7E) break;
    callsign[i] = (char) c;
    i++;
  }
  callsign[i] = '\0';

  // Live LCD text: default on unless explicitly turned off ('d'). A blank
  // EEPROM byte (0xFF) is not COMMAND_LIVE_LCD_OFF, so this naturally
  // defaults to enabled if never configured.
  liveLcdTextEnabled = (EEPROM.read(EE_LIVE_LCD_ADDR) != COMMAND_LIVE_LCD_OFF);
}

/**
* Init the timer to fire every *half* bit period.  This allows us
* to have 1.5 stop bits if we want.
*/
void initTimer()
{
  Timer1.stop();
  long bitPeriod = (long) ((1.0f/baudrate) * 1000000); //micros
  Timer1.initialize(bitPeriod/2.0);
  Timer1.attachInterrupt(timerISR);
}

/**
* The ISR for the half-bit timer is just to set a flag.  We
* will process in the main loop.
*/
void timerISR()
{
  isrFlag = true;
}

/**
* Displays the configuration options on the console
*/
void displayConfigurationPrompt()
{

  Serial.write("\nEnter configuration command.  Valid commands are:\n");
  Serial.write("   0    Set FSK polarity mark = HIGH\n");
  Serial.write("   1    Set FSK polarity mark = LOW\n");
  Serial.write("   4    Set 45.45 baud\n");
  Serial.write("   5    Set 50.0 baud\n");
  Serial.write("   7    Set 75.0 baud\n");
  Serial.print(F("   L    Set/get PTT lead time, ms (before first bit)\n"));
  Serial.print(F("   T    Set/get PTT tail time, ms (after last bit)\n"));
  Serial.print(F("   l    Set/get PA lead time, ms (before PTT)\n"));
  Serial.print(F("   t    Set/get PA tail time, ms (after PTT)\n"));
  Serial.print(F("   C    Set/get callsign (max 6 chars)\n"));
  Serial.print(F("   D    Enable live TX text on the status LCD\n"));
  Serial.print(F("   d    Disable live TX text on the status LCD\n"));
  Serial.write("\n   ?    Show current configuration\n");
}

/**
* Prints the current configuration the console
*/
void displayConfiguration()
{
  Serial.write("\nTinyFSK ");
  Serial.write(VERSION);
  Serial.write("\nCurrent configuration:\n");
  Serial.write("   Speed (baud):  ");
  Serial.print(baudrate);
  Serial.write("\n");

  Serial.write("   Polarity       ");
  if (mark == LOW)
  {
     Serial.write(" mark = logical LOW");
  }
  else
  {
     Serial.write(" mark = logical HIGH");
  }
  Serial.write("\n");

  Serial.print(F("   PTT lead (ms): "));
  Serial.print(pttLeadMillis);
  Serial.write("\n");

  Serial.print(F("   PTT tail (ms): "));
  Serial.print(pttTailMillis);
  Serial.write("\n");

  Serial.print(F("   PA lead  (ms): "));
  Serial.print(ptt_PA_LeadMillis);
  Serial.write("\n");

  Serial.print(F("   PA tail  (ms): "));
  Serial.print(ptt_PA_TailMillis);
  Serial.write("\n");

  Serial.print(F("   Callsign:      "));
  Serial.write(callsign[0] != '\0' ? callsign : "(not set)");
  Serial.write("\n");

  Serial.print(F("   Live LCD text: "));
  Serial.write(liveLcdTextEnabled ? "on" : "off");
  Serial.write("\n");
}

/******************************************************************
* This called every half-bit period to figure out what to bit-bang
* out the FSK pin.  It is basically an incremental counter that
* counts half bit periods and toggles the bits of the baudot character
* as needed.  It bangs out the start bit, five symbol bits, and the
* stop bit, which is 1.5 bits long (hence the need to have a timer
* counting half bits).
* The 5 bit RTTY character frame looks like this:
*
*       ||Start | LSB |  X  |  X  |  X  | MSB | Stop    ||
* bitPos:   -1     0     1     2     3     4      5
******************************************************************/
int sendingChar = LTRS_SHIFT; // default--this is the "diddle character"
int stopBitCounter = 0; // counts half-bits for stop bit
int bitPos = START_BIT_POS;        // -1 = Start bit
bool midBit = false;    // used as like an ignore flag--we usually don't
                        // toggle state in the middle of a bit.  The exception
                        // is the stop bit, which is often 1.5 bits long.
void processHalfBit() {

  if (!ptt)  //not transmitting, so just return--there's nothing to send.
  {
    return;
  }

  if (rtsKeyed)  // OK2ZAW: RTS-sourced TX doesn't use this board's internal
                 // Baudot generator at all--leave FSK_PIN alone (already
                 // sitting at its post-key-up mark level from setPTT()),
                 // instead of bit-banging idle diddles onto it for the
                 // whole transmission.
  {
    return;
  }

  if (midBit)
  {
   midBit = false; // reset the flag.  Next time we need to send the next bit.
   return;
  }

  // it's time to bang out the next bit.  We check for the special cases
  // first.  If it's a start bit we always sent SPACE and if its a STOP bit
  // we always send MARK.
  if (bitPos == START_BIT_POS) {  // we have to send a start bit

    // If it is time to send a start bit, we grab the next character to send so
    // that it is ready the next time through the loop.  The next character
    // might be the TX_END_FLAG, in which case we need to turn off the transmitter.
    sendingChar = getNextSendChar();

    if (ptt) {
      if (sendingChar == TX_END_FLAG) //end of data to send
      {
         setPTT(false);
         return;
      }
      else
      {
        digitalWrite(FSK_PIN, space);  //start bit is always space
        bitPos++;
        midBit = true;
      }
    }
  }
  else if (bitPos == STOP_BIT_POS) // we have to send a stop bit
  {
    //if stopBitCounter == 0 we are at the beginning of a stop bit
    if (stopBitCounter == 0)
    {
      digitalWrite(FSK_PIN, mark);
      stopBitCounter = stopBits;  //this determines # of half-bit periods we stay in stop bit
    }
    else // already in stop bit, just decrement
    {
      // stopBitCounter counts half-bit periods.  2 ==> one stop bit
      //                                          3 ==> 1.5 stop bits
      //                                          4 ==> two stop bits
      stopBitCounter--;
      if (stopBitCounter == 0) // end of stop bit period
      {
        bitPos = START_BIT_POS;  // move on to start bit of next char

        //  If we just sent an explicit LTRS or FIGS shift, obviously we are in that state.  If USOS is turned on and we
        //  have sent a space character, we are implicitly in LTRS shift.
        if (sendingChar == LTRS_SHIFT || (usos == USOS_ON && sendingChar == 0x04) ) //0x04 = Baudot space
        {
          currentShiftState = LTRS_SHIFT;
        }
        else if (sendingChar == FIGS_SHIFT)
        {
          currentShiftState = FIGS_SHIFT;
        }
      }
    }
  }
  else
  {
    // We are not sending a stop/start bit, so we send the next bit of the
    // of the character.
    bool b = (sendingChar & (0x01 << bitPos));  //LSB first
    if (b)
    {
      digitalWrite(FSK_PIN, mark);
    }
    else
    {
      digitalWrite(FSK_PIN, space);
    }
    bitPos++;
    midBit = true;
  }
}

/**
* Reset character buffer.  This is a helper routine when stop the
* transmitter so that everything is back to initial states ready
* to bang out the first character.
*/
void resetChar()
{
  sendingChar = LTRS_SHIFT;
  stopBitCounter = 0;
  bitPos = START_BIT_POS;
  midBit = false;
}

/**
*Wipes the send buffer. Helper function for aborting
* a transmission.
*/
void resetSendBuffer()
{
  for (int i = 0; i < SEND_BUFFER_SIZE; i++)
  {
     sendBufferArray[i] = 0;
  }
  sendBufferBytes = 0;
}

/**
* Adds a new byte to the transmit text buffer.  These
* are *ASCII* bytes from the terminal, not Baudot.
*
* OK2ZAW mod: this is the UART "received a data byte bound for FSK
* transmission" event--only ever called for real data (never for control
* characters [ ] \ ~, which are handled elsewhere and never reach here).
* If the sticky RTS source label is currently showing, switch it back and
* refresh line 1 right away. Called only from loop()'s serial-receive
* handler and waitDrainingSerial(), never from the bit-timing path, so the
* LCD write here is safe--see the note on getNextSendChar().
*/
void addToSendBuffer(byte newByte)
{
  if (sendBufferBytes < SEND_BUFFER_SIZE)
  {
    sendBufferBytes++;
    sendBufferArray[sendBufferBytes - 1] = newByte;
  }
  if (lastPttWasRts)
  {
    lastPttWasRts = false;
    lcdRefreshLine1();
  }
}

/**
* Gets the next Baudot (5-bit) char from the buffer.  This
* function will return LTRS or FIGS shift characters when
* needed depending on the current shift state and USOS setting.
*
* OK2ZAW note: this runs inside processHalfBit(), on the critical path
* between one half-bit timer tick and the next FSK_PIN transition. Do NOT
* add LCD/I2C calls (or anything else slow/blocking) here or anywhere else
* called from processHalfBit(), beyond the one already-accounted-for
* exception below--it directly jitters TX bit timing (measured: one
* lcd.print() of a single character is ~1.9ms of I2C traffic with
* LiquidCrystal_I2C's 4-bit mode, ~3.8ms if a setCursor() is also
* needed--a real, measured 9-28% shortening of whatever bit it lands on,
* worst at 75 baud).
*
* The one deliberate exception is lcdAppendTxChar(), called below for
* every character actually sent, to show live TX text on the LCD. This was
* tried, measured, reverted for the cost above, tried again as
* word-batched (worse: unbounded flush size), and finally restored here in
* this simplest fixed-size form--a known, user-accepted tradeoff, not an
* oversight. If you're tempted to batch multiple characters into one
* flush to reduce write *frequency*, don't: it increases the per-flush
* *duration* instead (proportional to however much text you batch), which
* is a worse and less predictable bound than one character every time.
* Any other new LCD update still belongs in setPTT() or loop() outside
* this call chain, not here.
*/
byte getNextSendChar()
{

  byte rVal = LTRS_SHIFT;  //default "idle" or "diddles" when nothing to send

  if (sendBufferBytes > 0)  // there is still data in buffer to send
  {
    byte asciiByte = sendBufferArray[0];

    if (currentShiftState != LTRS_SHIFT && requiresLetters(asciiByte))
    {
      //echo('_');
      return LTRS_SHIFT;
    }
    else if (currentShiftState != FIGS_SHIFT && requiresFigures(asciiByte))
    {
      //echo('^');
      return FIGS_SHIFT;
    }
    // Special "robust" USOS case--send FIGS after a space even if already in FIGS state and next
    // character requires FIGS shift.  Note: when this is called
    // sendingChar is the char we just *finished* sending
    else if (usos == USOS_MMTTY_HACK && currentShiftState != LTRS_SHIFT && requiresFigures(asciiByte) && sendingChar == 0x04)
    {
      //echo('^');
      return FIGS_SHIFT;
    }
    else //we don't need to send a shift character.  Just find the baudot equiv of the ascii symbol and return it.
    {
      rVal = asciiToBaudot[asciiByte];
      lastAsciiByteSent = asciiByte;
      sendBufferBytes--;
      if (sendBufferBytes > 0)
      {
        for (int i = 0; i < sendBufferBytes; i++)
        {
          sendBufferArray[i] = sendBufferArray[i+1];
        }
      }
      echo(asciiByte);
      if (!rtsKeyed && liveLcdTextEnabled) lcdAppendTxChar(asciiByte);  // OK2ZAW: live per-character write, TNC-sourced only, user-toggleable--deliberate, measured tradeoff, see comment above
    }
  }
  else  // the buffer is empty
  {
    if (endWhenBufferEmpty)
    {
       rVal = TX_END_FLAG;  // signals to stop the TX
    }
    else  // slow typist?
    {
      if (currentShiftState == SHIFT_UNKNOWN)
      {
        rVal = LTRS_SHIFT;  //send LTRS idle if we haven't sent anything on this TX
      }
      else
      {
        rVal = currentShiftState; // idle on LTRS or FIGS depending on what state we are in
      }
    }
  }
  return rVal;
}


/*
* returns whether or not this is a "letter".  Letters require LTRS
* shift preceding the byte if currently in FIGS mode.
*/
boolean requiresLetters(byte asciiByte)
{
  return (asciiByte >= 'A' && asciiByte <= 'Z')
    || (asciiByte >= 'a' && asciiByte <= 'z');

}

/**
* Helper function to find out whether a particular byte
* needs the FIGS shift preceeding it.
*/
boolean requiresFigures(byte asciiByte)
{
   return !requiresLetters(asciiByte)
    && (asciiByte != ASCII_NULL) //null
    && (asciiByte != ASCII_LF)   //LF
    && (asciiByte != ASCII_CR)
    && (asciiByte != ' ');
}


/**
* OK2ZAW mod: waits for `ms` milliseconds like delay(), but also drains any
* plain text bytes that arrive during the wait straight into the send
* buffer (queued for transmission, not shown on the LCD--line 2 is
* deliberately left alone until getNextSendChar() starts showing text
* live, from the first character actually sent; see lcdResetTxLine()).
* Only ever called from setPTT()'s PA/PTT lead delay, before the first FSK
* bit of this TX is due--so unlike getNextSendChar(), there is no
* bit-timing path here to interfere with.
*
* Control characters (TX_ABORT, TX_END, COMMAND_ESCAPE, TX_ON) are
* deliberately left untouched in the serial buffer via Serial.peek()
* rather than consumed here, so they still get handled by loop() exactly
* as they always have, once this wait returns--this function only ever
* takes plain data bytes off the buffer, never reinterprets a command.
*/
void waitDrainingSerial(unsigned long ms)
{
  unsigned long start = millis();
  while (millis() - start < ms)
  {
    if (Serial.available() > 0)
    {
      byte peeked = Serial.peek();
      if (peeked != TX_ABORT && peeked != TX_END && peeked != COMMAND_ESCAPE && peeked != TX_ON)
      {
        byte b = Serial.read();
        addToSendBuffer(b);
      }
    }
  }
}

/**
* Turns the PTT on or off and applies any delays that might exist.
*/
void setPTT(byte b)
{

  if (b)
  {  // PTT ON
    if (isInhibited()) // OK2ZAW: refuse to key up while inhibited
    {
      lcdShowStatus("INHIBIT"); // OK2ZAW
      return;
    }
    digitalWrite(PTT_PA_PIN, HIGH); // OK2ZAW mod PTT sequencer
    waitDrainingSerial(ptt_PA_LeadMillis); // OK2ZAW: was delay()--see function comment
    // OK2ZAW: RTS-sourced TX never bit-bangs FSK_PIN (see processHalfBit()),
    // so hold it LOW here instead of the usual "always start in mark
    // state"--mark could be HIGH depending on configured polarity, and
    // FSK_PIN is unused for the whole RTS-sourced session either way.
    digitalWrite(FSK_PIN, rtsKeyed ? LOW : mark);
    digitalWrite(PTT_PIN, HIGH);
    digitalWrite(LED_RX_PIN, LOW); // OK2ZAW: RX LED off--opposite of PTT_PIN
    // OK2ZAW: line 1 only--line 2 keeps showing "RX"/"INHIBIT" (whatever
    // was last on it) through the lead delay; see lcdResetTxLine() below
    // for where it actually goes blank, right before live text starts.
    lcdRefreshLine1();
    // we will stay in the mark state for some amount of time
    // before sending the first start bit of the first character
    waitDrainingSerial(pttLeadMillis); // OK2ZAW: was delay()--see function comment
    // OK2ZAW: blank line 2 right before real bit-banging starts, so the
    // live text in getNextSendChar() always begins from column 1 on an
    // empty line for every new TX--no leftover "RX"/"INHIBIT" text, and
    // (since nothing is queued to the LCD during the lead delay anymore)
    // nothing to double-show either.
    lcdResetTxLine();
  }
  else
  {  // PTT OFF
    digitalWrite(PTT_PA_PIN, LOW); // OK2ZAW mod PTT sequencer
    delay (ptt_PA_TailMillis);
    digitalWrite(PTT_PIN, LOW); // drop PTT
    digitalWrite(LED_RX_PIN, HIGH); // OK2ZAW: RX LED on--opposite of PTT_PIN
    // OK2ZAW: prefer INHIBIT over RX if the inhibit line is (already/still)
    // asserted right as this TX ends normally--keeps inhibitShown in sync
    // even in the rare case loop()'s own edge-check hasn't run yet.
    inhibitShown = isInhibited();
    lcdShowStatus(inhibitShown ? "INHIBIT" : "RX"); // OK2ZAW
    lcdResetTxLine(); // OK2ZAW: back in RX--ready for the next TX's live text from column 1
    // OK2ZAW: rtsKeyed can still be true here if this came from TX_ABORT
    // while RTS was still asserted (the normal end-of-buffer path already
    // clears rtsKeyed before this runs)--hold FSK_PIN LOW rather than
    // "space" in that case, same reasoning as the key-up write above.
    digitalWrite(FSK_PIN, rtsKeyed ? LOW : space);
    delay (pttTailMillis);
    stopBitCounter = 0;
    bitPos = -1;
    currentShiftState = SHIFT_UNKNOWN;

    lastAsciiByteSent = 0;
    Serial.write("\ncmd:\n"); // Tells N1MM that TX is finished
  }
   ptt = b;
}

/**
* OK2ZAW mod: returns true while CPU_INH_PIN is asserted (LOW), meaning
* keying is disallowed.
*/
boolean isInhibited()
{
  return digitalRead(CPU_INH_PIN) == LOW;
}

/**
* OK2ZAW mod: one-time boot splash shown on the LCD before normal RX/TX
* status display begins. Only ever called from setup(), before the timer
* and Serial "cmd:" ready signal--never from the FSK bit-timing path (see
* the note on getNextSendChar()), so the delay() calls here are safe.
*/
void lcdShowSplash()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("EasyFSK PLUS"));
  lcd.setCursor(0, 1);
  lcd.print(F("by QRO.CZ"));
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Firmware version"));
  lcd.setCursor(0, 1);
  lcd.print(F(FW_VERSION));
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("PTT lead:"));
  lcd.print(pttLeadMillis);
  lcd.print(F("ms"));
  lcd.setCursor(0, 1);
  lcd.print(F("PTT tail:"));
  lcd.print(pttTailMillis);
  lcd.print(F("ms"));
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("PA lead:"));
  lcd.print(ptt_PA_LeadMillis);
  lcd.print(F("ms"));
  lcd.setCursor(0, 1);
  lcd.print(F("PA tail:"));
  lcd.print(ptt_PA_TailMillis);
  lcd.print(F("ms"));
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Baud: "));
  lcd.print(baudrate);
  lcd.setCursor(0, 1);
  lcd.print(F("Polarity: "));
  lcd.print(mark == HIGH ? F("H") : F("L"));
  delay(1500);

  lcd.clear();
}

/**
* OK2ZAW mod: refreshes LCD line 1 only. Columns 1-6 show the configured
* callsign (or "CALL" if none is set); column 7 is a blank separator.
* Columns 8-16 (9 columns) show the PTT source--sticky, per lastPttWasRts
* (not the transient rtsKeyed): whichever of RTS/serial was used most
* recently, staying on screen across RX periods until the other one is
* actually used again (see lastPttWasRts's declaration comment):
*   - TNC/serial-sourced (this board's own serial/Baudot engine, the [ ] \
*     commands): "FSK " + two-digit baud rate + "b " + polarity ("H" or
*     "L"), e.g. "FSK 45b H"--fills all 9 columns exactly.
*   - RTS (PTT_USB_RTS_PIN, an external TNC's RTS line): "RTS DIGI" (8
*     chars, column 16 left blank)--baud/polarity are this board's own
*     internal FSK generator settings and don't apply to an external TNC,
*     so "DIGI" (a generic digital-mode label) is shown instead.
* Used on its own (leaving line 2 untouched) when keying up, mid-lead-delay
* in setPTT()--line 2 keeps showing "RX"/"INHIBIT" until lcdResetTxLine()
* blanks it right before live TX text starts.
*/
void lcdRefreshLine1()
{
  const char* callsignField = (callsign[0] != '\0') ? callsign : "CALL";

  char line1[LCD_COLS + 1];
  byte pos = 0;
  for (byte i = 0; callsignField[i] != '\0' && pos < 6; i++) line1[pos++] = callsignField[i];
  while (pos < 7) line1[pos++] = ' ';  // column 7: blank separator

  if (lastPttWasRts)
  {
    const char* src = "RTS DIGI";  // columns 8-15 (8 chars); column 16 padded blank below
    for (byte i = 0; src[i] != '\0'; i++) line1[pos++] = src[i];
  }
  else
  {
    byte baudInt = (byte) baudrate;  // 45, 50, or 75--exactly two digits for all supported rates
    line1[pos++] = 'F';                          // column 8
    line1[pos++] = 'S';                          // column 9
    line1[pos++] = 'K';                          // column 10
    line1[pos++] = ' ';                           // column 11
    line1[pos++] = '0' + (baudInt / 10);          // column 12: baud tens digit
    line1[pos++] = '0' + (baudInt % 10);          // column 13: baud ones digit
    line1[pos++] = 'b';                           // column 14
    line1[pos++] = ' ';                           // column 15
    line1[pos++] = (mark == HIGH) ? 'H' : 'L';    // column 16: FSK polarity
  }
  while (pos < LCD_COLS) line1[pos++] = ' ';  // pad any remainder (RTS branch is 1 short)
  line1[LCD_COLS] = '\0';

  lcd.setCursor(0, 0);
  lcd.print(line1);
}

/**
* OK2ZAW mod: refreshes both LCD lines--line 1 via lcdRefreshLine1(), plus
* line 2 with the given status text (e.g. "RX" or "INHIBIT"). Remembers
* the last line2 passed in, so other updates (e.g. a callsign change) can
* redraw line 1 without needing to know the current RX/inhibit state.
*/
void lcdShowStatus(const char* line2)
{
  lcdLastLine2 = line2;
  lcdRefreshLine1();

  lcd.setCursor(0, 1);
  lcd.print(line2);
  for (byte i = strlen(line2); i < LCD_COLS; i++) lcd.print(' ');
}

/**
* OK2ZAW mod: clears line 2 and resets the live TX-text column (lcdTxCol)
* to the start of it. Called in two places:
*   1. Returning to RX, right after lcdShowStatus() has already written
*      "RX"/"INHIBIT" there (so the blank-out here is redundant but
*      harmless)--ready for the next TX's live text to start clean.
*   2. setPTT(true), right before real bit-banging starts, to blank out
*      whatever "RX"/"INHIBIT" was still showing, so getNextSendChar()'s
*      live text always starts from an empty line 2 at column 1 for every
*      new TX. At that point `ptt` is still false (it's set after this
*      function's caller returns), so this is before
*      processHalfBit() has anything to do--same safe-timing reasoning as
*      the lead-delay waits themselves, just adding a small, one-time,
*      fixed amount to the overall lead sequence (not a per-character/
*      per-bit cost).
*/
void lcdResetTxLine()
{
  lcdTxCol = 0;
  lcd.setCursor(0, 1);
  for (byte i = 0; i < LCD_COLS; i++) lcd.print(' ');
}

/**
* OK2ZAW mod: appends one character to LCD line 2 at the live TX-text
* cursor, wrapping back to column 1 (overwriting from the left again) once
* the line is full. Non-printable bytes render as a blank.
*
* Called only from getNextSendChar(), for every character actually sent to
* FSK_PIN (TNC-sourced only)--so line 2 shows exactly what's being
* transmitted, live, and only that (no pre-TX preview; see
* waitDrainingSerial() and lcdResetTxLine()). This IS inside the
* bit-timing path, and is a deliberate, measured, user-accepted tradeoff:
* ~1.9ms of I2C traffic per character (~3.8ms on a line-wrap character
* needing a setCursor()), which shortens whatever bit it lands on by
* roughly 9-28% (worst at 75 baud, worst on a wrap character). See the
* note on getNextSendChar() before changing how/how-often this is called--
* batching multiple characters into one flush makes the bound worse, not
* better (duration scales with however much text you batch, unlike a
* fixed one-character write).
*/
void lcdAppendTxChar(byte b)
{
  if (b < 0x20 || b > 0x7E) b = ' ';  // render non-printable as blank
  if (lcdTxCol == 0) lcd.setCursor(0, 1);
  lcd.print((char) b);
  lcdTxCol++;
  if (lcdTxCol >= LCD_COLS) lcdTxCol = 0;  // wrap--next char restarts at column 1
}

/**
* Echo to the serial port.  This will show up in the user's terminal
* if he or she is watching.
*/
void echo(byte b)
{
  Serial.write(b);
}
