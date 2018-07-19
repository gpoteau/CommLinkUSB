
#define SPECIAL_VERSION 0

////////////////////////////////////////////////////////////////////////////////
//
// CatFlap
//
////////////////////////////////////////////////////////////////////////////////
//
// CAETLA Communications Tool ported to Linux by Hkz
//
// (C) 1998 Ash Hogg, Intar Technologies Limited
//
////////////////////////////////////////////////////////////////////////////////
//
// Uses C++ classes for extra sweetness :)
//
////////////////////////////////////////////////////////////////////////////////
//
// Rev   Date     Comment
// ======================
// 1.01  14/10/98 First version
// 1.02  12/11/98 Added 'run exe' mode
// 1.03  16/11/98 Finally sorted keyboard re-direction! Server mode also sorted.
// 1.04  17/11/98 Added switches, many bugs fixed in CCaetla class!
// 1.05  18/11/98 A few tidy-ups, that's all :)
// 1.06  19/11/98 Fixed file-write bug in server mode
// 1.07  20/11/98 Improved the PC keyboard redirection / server mode
// 1.08  21/11/98 Added preliminary CPE support too!
// 1.09  25/11/98 Added PC executable support thru CMD: pseudo-device!
// 1.10  02/12/98 Organised command flow much better, non-filtering switches after EXE name
// 1.12  02/01/99 First revision of NT support finally completed
// 1.13  04/01/99 Relocated driver & install/uninstall code to the Caetla class
// 1.14  07/01/99 Added fix for weird EXEs - EZ-packed?
// 1.15  08/01/99 Added VRAM support
// 1.16  09/01/99 Added MemCard support
// 1.17  12/01/99 Much faster MemCard delete
// 1.18  12/01/99 Tidied up some error-trapping, MemCard status checks
// 1.20  26/01/99 Minor Revision update
// 1.21  28/01/99 Changed so that Address can be 0
// 1.22  20/03/99 Added EEPROM Flash function
// 1.23  27/03/99 Fixed EEPROM Flash under NT
// 1.24  04/05/99 Adding back the TIMEOUT break in Send8 seemed to sort out the
//                problem with sometimes being unable to get control of the PSX,
//                forcing a manual reset/retry. Nice, but not *too* sure why! :)
// 1.25  06/05/99 Added RAW console mode for just dumping text
// 1.26  28/07/99 Added CMD:QUIT as a special CatFlap command to exit console
// 1.27  29/07/99 Modified help screens to return to main help menu each time
// 1.28  30/07/99 Added log file support
// 1.29  04/08/99 Fixed log flush
// 1.30  05/08/99 VRAM fetch to BMP (detect "*.bmp")
// 1.31  09/08/99 First revision of key-binding/scripting system
// 1.32  03/09/99 Fixed F10 error in DOS Console of C++ class, added SysCall Method
// 1.33  18/10/99 Sensibly shows when a file can't be found :) Oops.
// 1.34  27/10/99 A little messy, but XPlorer support is now in there
// 1.35  28/10/99 Stupid port-setup bug (only 0x378) fixed
// 1.36  28/10/99 Added progress bar to SendData, currently used on SendEXE, not NT
// * MAJOR REVISION *
// 2.00  29/10/99 Major Revision change, all NT & XPlorer stuff works, all features OK!
// 2.01  03/11/99 Sorted PsyQ PCwrite(-1,...) bug
// 2.02  08/12/99 Decent progress indication on CPE upload
// 2.03  18/02/00 Altered Timeouts (with Sleeps), Option to run EXE in Console
// 2.04  21/02/00 New Port/Cart-Type settings, new ENV settings
// 2.05  22/02/00 Refined Timeout/Sleep a little
// 2.06  22/02/00 New Timeout system with GetTickCount()
// 2.07  22/02/00 Callback Hook for Keyboard Input in Console mode
// 2.08  22/02/00 Callback Hook for Print Output in Console mode, DateStamp moved to CCaetla
// 2.09  23/02/00 New Help System
// 2.10  09/03/00 Better timeouts
// 2.11  09/03/00 Mega Timeout sorting, plus Console Mode optimisation
// 2.12  10/03/00 Fixed really stupid bug on -k- which disabled Args rather than KB.
//                Some comms sorted out to fix some XPlorer problems.
// 2.13  16/03/00 Kickass new Kernel-Mode driver for NT4/Win2000
// 2.14  17/03/00 Some optimisations
// 2.15  17/03/00 Some minor tweaks and fixes
// 2.16  17/03/00 Added Caetla hook control on EXE execution
// 2.17  18/03/00 Change to RAWCON mode
// 2.18  20/03/00 Added some Debug-mode functions
// 2.19  21/03/00 Added some Debug-mode functions
// 2.20  22/03/00 Added some Debug-mode functions, minor revisions to console
// 2.21  22/03/00 Debug Functions, Added some Debug-mode functions, minor revisions to console
// 2.22  22/03/00 Further minor improvements
// 2.23  23/03/00 Streamlining uni/bi-directional low-level comms functions
// 2.24  23/03/00 Minor changes (help info)
// 2.25  27/03/00 Rewrote CPE loading to 'depack' into virtual buffer first
// 2.26  01/04/00 Added 'resume' command
// 2.27  01/04/00 Added 'resume' command to Help function :)
// 2.28  12/04/00 Fixed dual-mode up/down code for XP
// 2.29  12/04/00 Changed EXE execution as it was using m_FileSize always
// 2.30  13/04/00 Minor fixes, self-terminate detection won't work on XP so removed
// 2.31  13/04/00 New method of self-termination for XP
// 2.32  15/08/00 Cursor/Control keys working under DOS box? Fine on my system...
// 2.33  27/09/00 Found TIMEOUT errors on PCwrite mode, temp fixed but must finalise
// 2.34  27/09/00 Fixed stupid 'count' variable not incrementing and causing default timeouts
// 2.35  29/09/00 Added -as switch to disable commlink autosensing in the console mode
// 2.36  29/09/00 Modified CArgs to always build and utilise a local arguments buffer
// 2.36  10/12/10 Ported to Linux by Hkz
//
////////////////////////////////////////////////////////////////////////////////


#include    <stdio.h>
#include    <stdarg.h>
#include    <stdlib.h>
#include    <string.h>
#include    <fcntl.h>
#include    <sys/stat.h>
#include    <sys/io.h>
#include    <strings.h>
#include    <linux/parport.h>
#include    <linux/ppdev.h>
#include    <sys/io.h>
#include    <sys/ioctl.h>
#include    <unistd.h>
#include    "CArgs.h"
#include    "CCaetla.h"
#include    "common_defs.h"

// Internal
#define     VER     2
#define     REV     36

// Linux version
#define		LVER	0
#define		LREV	1

#define     PROGNAME    "CatFlap"



////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

int     main(int, char *[]);
void    usage(void);
void    cleanup(void);
//short ConKeyHook(void);
void    ConPrintHook(char *);
int     BindIni(char *);
void    DumpIni(void);
void    ParseLine(char *, char *);
int     ConHook(uint8, FILE *);
void    ConOut(FILE *file, const char *fmt, ...);
int     CallFunction(FILE *, char *, char *);
char    *NextLine(char *);
int     FileExists(char *);

////////////////////////////////////////////////////////////////////////////////

CArgs           m_Args;             // our Args-handler instance
CCaetla         m_Caetla;           // our Caetla-handler instance!

int             m_Verbose = 0;
int             ARPort = 0;
int             ARArgs = 0;
int             m_DoConsole = 1;
bool            m_ExeHooksOn = 1;
int             m_RawCon = 0;
int             m_CartType = CART_XP;
char            *m_Log = NULL;

char            *m_Ini = NULL;

char PPdevice[MAX_PPDEV_SIZE];

////////////////////////////////////////////////////////////////////////////////
// Multi-purpose Number parser :)

uint64 ParseNumber(char *num) {
	uint64 ret = 0;

// Hex?
	if (strncasecmp(num, "0x", 2) == 0) {
		if (sscanf(num + 2, "%lx", &ret) == 1) {
			return(ret);
		} else {
			printf("Error in parameter:  %s\n", num);
			return(ret);
		}
	}
// Dec?
	else {
		if (sscanf(num, "%ld", &ret) == 1) {
			return(ret);
		} else {
			printf("Error in parameter:  %s\n", num);
			return(ret);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Callback functions for Switch processing

void SwitchCallback_Null(char *arg) {
}

void SwitchCallback_PAR(char *arg) {
	m_CartType = CART_PAR;
}

void SwitchCallback_XP(char *arg) {
	m_CartType = CART_XP;
}

void SwitchCallback_K(char *arg) {
	if (*arg == '-') {
		m_Caetla.m_KeyIn = 0;
		if (m_Verbose) {
			printf("Keyboard Redirection disabled\n");
		}
	} else {
		int addr;
		addr = ParseNumber(arg);
		m_Caetla.m_KeyIn = addr;
		if (m_Verbose) {
			printf("Keyboard Buffer override - Set to 0x%x\n", m_Caetla.m_KeyIn);
		}
	}
}

void SwitchCallback_A(char *arg) {
	if (*arg == '-') {
		ARArgs = 0;
		if (m_Verbose) {
			printf("PSX Arguments disabled\n");
		}
	} else {
		int addr;
		addr = ParseNumber(arg);
		ARArgs = addr;
		if (m_Verbose) {
			printf("PSX Args Buffer override - Set to 0x%x\n", ARArgs);
		}
	}
}

void SwitchCallback_V(char *arg) {
	m_Verbose = 1;
}

void SwitchCallback_C(char *arg) {
	if (*arg == '-') {
		m_DoConsole = 0;
		if (m_Verbose) {
			printf("Console Mode disabled after program execution\n");
		}
	}
}

void SwitchCallback_P(char *arg) {
	int addr;

	addr = ParseNumber(arg);
	ARPort = addr;
	if (m_Verbose) {
		printf("Comms Port set to 0x%x\n", ARPort);
	}
}

void SwitchCallback_L(char *arg) {
	if (*arg == '=')
		m_Log = arg + 1;
	else
		m_Log = arg;
	if (m_Verbose) {
		printf("Logging Console output to file '%s'\n", m_Log);
	}
}


void SwitchCallback_RAWCON(char *arg) {
	m_RawCon = 1;
	if (m_Verbose) {
		printf("RAW Console Mode enabled\n");
	}
}

void SwitchCallback_H(char *arg) {
	m_ExeHooksOn = false;
	m_DoConsole = 0;
	if (m_Verbose) {
		printf("Caetla Hook disabled on program execution\n (Console Mode automatically disabled)\n");
	}
}

#define TIMEOUT_MINDEF  0.0f
#define TIMEOUT_MAXDEF  30.0f

void SwitchCallback_AS(char *arg) {
	if (m_Verbose) {
		printf("CommLink Status AutoSense disabled in Console Mode\n");
	}
	m_Caetla.m_AutoSense = false;
}


////////////////////////////////////////////////////////////////////////////////
// Switch Callback list

struct Arg_SwitchAssign Switches[] = {
	{   "v",        SwitchCallback_V,       ARG_CASE_INSENSITIVE    },
	{   "PAR",      SwitchCallback_PAR,     ARG_CASE_INSENSITIVE    },
	{   "XP",       SwitchCallback_XP,      ARG_CASE_INSENSITIVE    },
	{   "as",       SwitchCallback_AS,      ARG_CASE_INSENSITIVE    },
	{   "k",        SwitchCallback_K,       ARG_CASE_INSENSITIVE    },
	{   "a",        SwitchCallback_A,       ARG_CASE_INSENSITIVE    },
	{   "c",        SwitchCallback_C,       ARG_CASE_INSENSITIVE    },
	{   "p",        SwitchCallback_P,       ARG_CASE_INSENSITIVE    },
	{   "l",        SwitchCallback_L,       ARG_CASE_INSENSITIVE    },
	{   "h",        SwitchCallback_H,       ARG_CASE_INSENSITIVE    },
	{   "rawcon",   SwitchCallback_RAWCON,  ARG_CASE_INSENSITIVE    },
	{   NULL,       NULL,                   0   },
};


////////////////////////////////////////////////////////////////////////////////
// Commands parsing

enum {
	CMD_RUN,
	CMD_SEND,
	CMD_RECEIVE,
	CMD_RESET,
	CMD_RESUME,
	CMD_CONSOLE,
	CMD_HELP,
	CMD_INSTALLNT,
	CMD_UNINSTALLNT,
	CMD_VSEND,
	CMD_VGET,
	CMD_LISTCARDS,
	CMD_UPCARDF,
	CMD_DOWNCARDF,
	CMD_BACKUPCARD,
	CMD_RESTORECARD,
	CMD_FORMATCARD,
	CMD_DELETECARD,
	CMD_FLASHEPROM,
//	CMD_,
};

typedef struct Cmd_Definition {
	int     id;
	const char  *name;
	int (*func)(int argc, char *argv[]);
}   CMD_DEFINITION;




////////////////////////////////////////////////////////////////////////////////
// Run an EXE or CPE

int CmdCallback_Run(int argc, char *argv[]) {
	char *exename;

	exename = m_Args.GetNormalArg(2);
	if (!exename) {
		printf("Error - no EXE file specified.\n");
		return(EXIT_FAILURE);
	}

	if (FileExists(exename) == 0) {
		printf("Error - file '%s' not found.\n", exename);
		return(EXIT_FAILURE);
	}

// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

// Prepare argc,argv[] first! (if required)

	if (ARArgs) {
		char argbuff[4+(16*4)+256];    // ==324
		char *argp;
		uint32 *lp;
		int ac;

		int ai = m_Args.GetNormalArgIndex(2);
		int argc2 = m_Args.GetNumArgs() - ai;

		if (argc2 < 0)   argc2 = 0;
		if (argc2 > 16)  argc2 = 16;

		if (argc2) {
			lp = (uint32 *)&argbuff;
			lp[0] = argc2;
			for (ac = 0; ac < 16; ac++)
				lp[1+ac] = 0;

			argp = &argbuff[4+(16*4)];
			*argp = 0;

			for (ac = 0; ac < argc2; ac++) {
				lp[1+ac] = (uint32)((uint8 *)argp - (uint8 *)&argbuff[0] + ARArgs);
				strcpy((char *)argp, argv[ac+ai]);
				*(argp + strlen(argv[ac+ai])) = 0;
				argp += strlen((const char *)argp) + 1;
			}

			printf("Preparing PSX Runtime Arguments...\n");

			m_Caetla.Upload(&argbuff[0], ARArgs, sizeof(argbuff), false, false);

			if (m_Caetla.ShowError())      return(EXIT_FAILURE);

		}
	}

	printf("Executing [%s]...\n", exename);

	if (m_DoConsole)
		m_Caetla.RunExe(exename, false, m_ExeHooksOn);
	else
		m_Caetla.RunExe(exename, true, m_ExeHooksOn);

	if (m_Caetla.ShowError())      return(EXIT_FAILURE);

	if (m_DoConsole) {
		if (m_RawCon)
			m_Caetla.RawConsole();
		else
			m_Caetla.DosConsole();
	}

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Send a file

int CmdCallback_Send(int argc, char *argv[]) {
	int Addr;
	char *fname;
	char *add;

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	add = m_Args.GetNormalArg(3);
	if (!add) {
		printf("Error - no address specified.\n");
		return(EXIT_FAILURE);
	}

	if (FileExists(fname) == 0) {
		printf("Error - file '%s' not found.\n", fname);
		return(EXIT_FAILURE);
	}


#if 0
	if (Addr = ParseNumber(add))
#else
	Addr = ParseNumber(add);
	if (1)
#endif
	{

// Initialise our comms port

		printf("Initializing Caetla...\n");
		m_Caetla.Init(ARPort, m_CartType);

		if (m_Caetla.ShowError())  return(EXIT_FAILURE);

		printf("Sending File [%s] to 0x%x...\n", fname, Addr);

		m_Caetla.Detect();
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);
		m_Caetla.Listen(CAETLA_MODE_MAIN);
//	m_Caetla.Listen(CAETLA_MODE_DEBUG);
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);
		m_Caetla.SendFile(fname, Addr);
//	m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);

		return(EXIT_SUCCESS);
	} else {
		usage();
		return(EXIT_FAILURE);
	}
}


////////////////////////////////////////////////////////////////////////////////
// Receive a file

int CmdCallback_Receive(int argc, char *argv[]) {
	int Addr, Len;
	char *fname;
	char *add;

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	add = m_Args.GetNormalArg(3);
	if (!add) {
		printf("Error - no address specified.\n");
		return(EXIT_FAILURE);
	}

#if 0
	if (!(Addr = ParseNumber(add))) {
		usage();
		return(EXIT_FAILURE);
	}
#else
	Addr = ParseNumber(add);
#endif

	add = m_Args.GetNormalArg(4);
	if (!add) {
		printf("Error - no length specified.\n");
		return(EXIT_FAILURE);
	}

	if (Len = ParseNumber(add)) {
// Initialise our comms port

		printf("Initializing Caetla...\n");
		m_Caetla.Init(ARPort, m_CartType);

		if (m_Caetla.ShowError())  return(EXIT_FAILURE);

		printf("Receiving 0x%x (%d) bytes from 0x%x to File [%s]...\n", Len, Len, Addr, fname);

		m_Caetla.Detect();
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);

		m_Caetla.Listen(CAETLA_MODE_MAIN);
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);
//	m_Caetla.Listen(CAETLA_MODE_DEBUG);
		m_Caetla.ReceiveFile(fname, Addr, Len);
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);
//	m_Caetla.Resume();


		return(EXIT_SUCCESS);
	} else {
		usage();
		return(EXIT_FAILURE);
	}
}


////////////////////////////////////////////////////////////////////////////////
// Reset the PSX

int CmdCallback_Reset(int argc, char *argv[]) {

// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Resetting PSX...\n");

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);
	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Resume control on the PSX

int CmdCallback_Resume(int argc, char *argv[]) {

// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

//	printf("Resetting PSX...\n");

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);
	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Console Mode

int CmdCallback_Console(int argc, char *argv[]) {

// Initialise our comms port

//testing XP cart...
	if (m_RawCon) {
		m_Caetla.RawConsole();
		return(0);
	}

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.DosConsole();

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Help screen

int CmdCallback_Help(int argc, char *argv[]) {
	fprintf(stdout, "CatFlap usage:\n"
	        " * GENERAL COMMANDS:\n"
	        "   run [filename] [runtime args, ...]\t\tRun a PSX EXE or CPE file\n"
	        "   reset\t\t\t\t\tReset the PSX\n"
	        "   resume\t\t\t\t\tContinue the execution on PSX if halted\n"
	        "   help\t\t\t\t\t\tView Help information\n"
	        "   console\t\t\t\t\tEnter Console mode\n"
	        "\n"
	        " * MEMORY TRANSFER COMMANDS:\n"
	        "   up [filename] [address]\t\t\tUpload file to PSX RAM\n"
	        "   down [filename] [address] [size]\t\tDownload file from PSX RAM\n"
	        "\n"
	        " * VRAM TRANSFER COMMANDS:\n"
	        "   vup [filename] [x] [y] [w] [h] [depth]\tUpload file to PSX VRAM\n"
	        "   vdown [filename] [x] [y] [w] [h] [depth]\tDownload file from PSX VRAM\n"
	        "\n"
	        " * MEMORY CARD COMMANDS:\n"
	        "   mclist [slot]\t\t\t\tList Contents of Memory Card\n"
	        "   mcup [filename] [slot] <optional name>\tUpload a Savegame file to Memory Card\n"
	        "   mcdown [filename] [slot] [file]\t\tDownload a Savegame file from Memory Card\n"
	        "   mcbackup [filename] [slot]\t\t\tBackup a whole Memory Card\n"
	        "   mcrestore [filename] [slot]\t\t\tRestore a whole Memory Card\n"
	        "   mcdelete [slot] [file]\t\t\tDelete a File from Memory Card\n"
	        "   mcformat [slot]\t\t\t\tFormat a Memory Card\n"
	        "\n");
	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Install Kernel-Mode I/O Driver for NT * USELESS IN LINUX *

int CmdCallback_InstallNT(int argc, char *argv[]) {
	m_Caetla.InstallNT();

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Remove Kernel-Mode I/O Driver for NT * USELESS IN LINUX *

int CmdCallback_UninstallNT(int argc, char *argv[]) {
	m_Caetla.UninstallNT();

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Send file to VRAM

int CmdCallback_SendVRAM(int argc, char *argv[]) {
	int x, y, w, h, depth;
	char *fname;
	char *param;

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	if (FileExists(fname) == 0) {
		printf("Error - file '%s' not found.\n", fname);
		return(EXIT_FAILURE);
	}

	param = m_Args.GetNormalArg(3);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	x = ParseNumber(param);

	param = m_Args.GetNormalArg(4);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	y = ParseNumber(param);

	param = m_Args.GetNormalArg(5);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	w = ParseNumber(param);

	param = m_Args.GetNormalArg(6);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	h = ParseNumber(param);

	param = m_Args.GetNormalArg(7);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	depth = ParseNumber(param);


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Uploading File [%s] to VRAM...\n", fname);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.SendVRAM(fname, x, y, w, h, depth);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Download VRAM to file

int CmdCallback_GetVRAM(int argc, char *argv[]) {
	int x, y, w, h, depth;
	char *fname;
	char *param;

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	param = m_Args.GetNormalArg(3);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	x = ParseNumber(param);

	param = m_Args.GetNormalArg(4);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	y = ParseNumber(param);

	param = m_Args.GetNormalArg(5);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	w = ParseNumber(param);

	param = m_Args.GetNormalArg(6);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	h = ParseNumber(param);

	param = m_Args.GetNormalArg(7);

	if (!param)    {
		printf("Error - missing parameter.\n");
		return(EXIT_FAILURE);
	}
	depth = ParseNumber(param);


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Receiving VRAM to File [%s] ...\n", fname);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.ReceiveVRAM(fname, x, y, w, h, depth);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// List Memory Cards

char JIS_129[] = {
//	 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 0
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 20
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 40
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ',', '.', ' ', ':', ';', '?', '!', ' ', ' ', ' ', ' ', ' ', '^', // 60
	'~', '_', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '/', ' ', ' ', ' ', '|', ' ', // 80
	'`', ' ', '\'', ' ', '"', '(', ')', ' ', ' ', '[', ']', '{', '}', '<', '>', ' ', ' ', ' ', ' ', ' ', // 100
	' ', ' ', ' ', '+', '-', ' ', ' ', ' ', ' ', '=', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 120
	' ', ' ', ' ', '\\', '$', ' ', ' ', '%', '#', '&', '*', '@', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 140
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 160
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 180
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 200
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 220
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 240
};

char JIS_130[] = {
//	 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 0
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 20
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 40
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '0', // 60
	'1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'A', 'B', 'C', 'D', // 80
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', // 100
	'Y', 'Z', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', // 120
	'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ', ' ', ' ', ' ', ' ', // 140
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 160
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 180
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 200
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 220
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 240
};



int CmdCallback_ListCards(int argc, char *argv[]) {
	int port;
	char *param;

	param = m_Args.GetNormalArg(2);
	if (!param) {
		printf("Error - Memory Card Port not specified.\n");
		return(EXIT_FAILURE);
	}

	port = (ParseNumber(param) - 1) & 1;


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.ScanMemCards(port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	if (m_Caetla.m_MemCards[port].numfiles > 0) {
		int i, len, j, k;
		uint8 *comment;

		printf("\n");
		printf("Contents of Memory Card Slot %d\n", port + 1);
		printf("------+------+----------------------------------------------\n");
		printf(" File | Blks | Name\n");
		printf("------+------+----------------------------------------------\n");

		for (i = 0; i < m_Caetla.m_MemCards[port].numfiles; i++) {
			printf("   %0d  |  %02d  | %s\n", i, m_Caetla.m_MemCards[port].files[i].size / 8192, (char *)&m_Caetla.m_MemCards[port].files[i].name[0]);

			comment = (uint8 *)&m_Caetla.m_MemCards[port].files[i].comment[0];
			len = strlen((char *)comment);
			for (j = 0, k = 0; j < len; j += 2, k++) {
				switch (comment[j]) {
				case    0x81:
					comment[k] = JIS_129[comment[j+1]];
					break;
				case    0x82:
					comment[k] = JIS_130[comment[j+1]];
					break;
				default:
					comment[k] = '#';
					break;
				}
			}
			comment[k] = 0;

			printf("      |      | %s\n", (char *)&m_Caetla.m_MemCards[port].files[i].comment[0]);
			if (i == m_Caetla.m_MemCards[port].numfiles - 1)
				printf("------+------+----------------------------------------------\n");
		}
	} else {
		printf("\nMemory Card in Slot %d is empty.\n", port + 1);
	}

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Download a Memory Card File

int CmdCallback_DownCardFile(int argc, char *argv[]) {
	int port, mfile;
	char *param, *fname;

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	param = m_Args.GetNormalArg(3);
	if (!param) {
		printf("Error - Memory Card Port not specified.\n");
		return(EXIT_FAILURE);
	}

	port = (ParseNumber(param) - 1) & 1;

	param = m_Args.GetNormalArg(4);
	if (!param) {
		printf("Error - Memory Card File not specified.\n");
		return(EXIT_FAILURE);
	}

	mfile = ParseNumber(param);


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.ScanMemCards(port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Reading Savegame to file [%s]...\n", fname);

	m_Caetla.ReadMemCardFile(fname, port, mfile);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////
// Upload a Memory Card File

int CmdCallback_UpCardFile(int argc, char *argv[]) {
	int port, mfile = 0;
	char *param, *fname, *oname;

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	if (FileExists(fname) == 0) {
		printf("Error - file '%s' not found.\n", fname);
		return(EXIT_FAILURE);
	}

	param = m_Args.GetNormalArg(3);
	if (!param) {
		printf("Error - Memory Card Port not specified.\n");
		return(EXIT_FAILURE);
	}

	port = (ParseNumber(param) - 1) & 1;

	oname = m_Args.GetNormalArg(4);


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.ScanMemCards(port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Uploading Savegame file [%s]...\n", fname);

	m_Caetla.SaveMemCardFile(oname, fname, port, mfile);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Backup a Memory Card

int CmdCallback_BackupCard(int argc, char *argv[]) {
	int port;
	char *param, *fname;

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	param = m_Args.GetNormalArg(3);
	if (!param) {
		printf("Error - Memory Card Port not specified.\n");
		return(EXIT_FAILURE);
	}

	port = (ParseNumber(param) - 1) & 1;


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.ScanMemCards(port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Backing up memory card to file [%s]...\n", fname);
	printf("Please wait as this operation can take a little while...\n");

	m_Caetla.BackupCard(fname, port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Restore a Memory Card

int CmdCallback_RestoreCard(int argc, char *argv[]) {
	int port;
	char *param, *fname;

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	if (FileExists(fname) == 0) {
		printf("Error - file '%s' not found.\n", fname);
		return(EXIT_FAILURE);
	}

	param = m_Args.GetNormalArg(3);
	if (!param) {
		printf("Error - Memory Card Port not specified.\n");
		return(EXIT_FAILURE);
	}

	port = (ParseNumber(param) - 1) & 1;


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.ScanMemCards(port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Restoring memory card from file [%s]...\n", fname);
	printf("Please wait as this operation can take a little while...\n");

	m_Caetla.RestoreCard(fname, port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Format a Memory Card

int CmdCallback_FormatCard(int argc, char *argv[]) {
	int port;
	char *param;

	param = m_Args.GetNormalArg(2);
	if (!param) {
		printf("Error - Memory Card Port not specified.\n");
		return(EXIT_FAILURE);
	}

	port = (ParseNumber(param) - 1) & 1;


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.ScanMemCards(port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Formatting memory card...\n");
	printf("Please wait as this operation can take a little while...\n");

	m_Caetla.FormatMemCard(port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Delete a Memory Card file

int CmdCallback_DeleteCard(int argc, char *argv[]) {
	int port, mfile;
	char *param;

	param = m_Args.GetNormalArg(2);
	if (!param) {
		printf("Error - Memory Card Port not specified.\n");
		return(EXIT_FAILURE);
	}

	port = (ParseNumber(param) - 1) & 1;

	param = m_Args.GetNormalArg(3);
	if (!param) {
		printf("Error - Memory Card File not specified.\n");
		return(EXIT_FAILURE);
	}

	mfile = ParseNumber(param);


// Initialise our comms port

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Detect();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.ScanMemCards(port);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	printf("Deleting Savegame on memory card...\n");

	m_Caetla.DeleteMemCardFile(port, mfile);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////////
// Flash an EEPROM image

int CmdCallback_FlashEPROM(int argc, char *argv[]) {
	char *fname;

	if ((ARPort != 0x300) && (ARPort != 0x310) && (ARPort != 0x320) && (ARPort != 0x330)) {
		printf("Error - EPROM flashing not yet supported in this mode.\n");
		return(EXIT_FAILURE);
	}

	fname = m_Args.GetNormalArg(2);
	if (!fname) {
		printf("Error - no filename specified.\n");
		return(EXIT_FAILURE);
	}

	if (FileExists(fname) == 0) {
		printf("Error - file '%s' not found.\n", fname);
		return(EXIT_FAILURE);
	}

	printf("Initializing Caetla...\n");
	m_Caetla.Init(ARPort, m_CartType);

	m_Caetla.FlashEPROM(fname);
	if (m_Caetla.ShowError())  return(EXIT_FAILURE);
	return(EXIT_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////

struct Cmd_Definition CFCommands[] = {
#if 0
	{   CMD_RUN,        "run",          CmdCallback_Run         },
	{   CMD_SEND,       "send",         CmdCallback_Send        },
	{   CMD_RECEIVE,    "receive",      CmdCallback_Receive     },
	{   CMD_RESET,      "reset",        CmdCallback_Reset       },
	{   CMD_CONSOLE,    "console",      CmdCallback_Console     },
	{   CMD_HELP,       "help",         CmdCallback_Help        },
	{   CMD_INSTALLNT,  "install",      CmdCallback_InstallNT   },
	{   CMD_UNINSTALLNT, "uninstall",    CmdCallback_UninstallNT },
	{   CMD_VSEND,      "vsend",        CmdCallback_SendVRAM    },
	{   CMD_VGET,       "vget",         CmdCallback_GetVRAM     },
	{   CMD_LISTCARDS,  "mclist",       CmdCallback_ListCards   },
	{   CMD_UPCARDF,    "mcup",         CmdCallback_UpCardFile  },
	{   CMD_DOWNCARDF,  "mcdown",       CmdCallback_DownCardFile    },
	{   CMD_BACKUPCARD, "mcbackup",     CmdCallback_BackupCard  },
	{   CMD_RESTORECARD, "mcrestore",    CmdCallback_RestoreCard },
	{   CMD_FORMATCARD, "mcformat",     CmdCallback_FormatCard  },
	{   CMD_DELETECARD, "mcdelete",     CmdCallback_DeleteCard  },
#else
	{   CMD_RUN,        "run",          CmdCallback_Run         },
	{   CMD_SEND,       "up",           CmdCallback_Send        },
	{   CMD_RECEIVE,    "down",         CmdCallback_Receive     },
	{   CMD_RESET,      "reset",        CmdCallback_Reset       },
	{   CMD_RESUME,     "resume",       CmdCallback_Resume      },
	{   CMD_CONSOLE,    "console",      CmdCallback_Console     },
	{   CMD_HELP,       "help",         CmdCallback_Help        },
	{   CMD_INSTALLNT,  "install",      CmdCallback_InstallNT   },
	{   CMD_UNINSTALLNT, "uninstall",    CmdCallback_UninstallNT },
	{   CMD_VSEND,      "vup",          CmdCallback_SendVRAM    },
	{   CMD_VGET,       "vdown",        CmdCallback_GetVRAM     },
	{   CMD_LISTCARDS,  "mclist",       CmdCallback_ListCards   },
	{   CMD_UPCARDF,    "mcup",         CmdCallback_UpCardFile  },
	{   CMD_DOWNCARDF,  "mcdown",       CmdCallback_DownCardFile    },
	{   CMD_BACKUPCARD, "mcbackup",     CmdCallback_BackupCard  },
	{   CMD_RESTORECARD, "mcrestore",    CmdCallback_RestoreCard },
	{   CMD_FORMATCARD, "mcformat",     CmdCallback_FormatCard  },
	{   CMD_DELETECARD, "mcdelete",     CmdCallback_DeleteCard  },
	{   CMD_FLASHEPROM, "flash",        CmdCallback_FlashEPROM  },
#endif

	{   0,              NULL,           NULL                }
};




#if 0
HHOOK KeyHook = NULL;

int keytest = 0;

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
	keytest++;

	if (code < 0) {
		return(CallNextHookEx(KeyHook, code, wParam, lParam));
	} else if ((code == HC_ACTION) || (code == HC_NOREMOVE)) {
		if (!(lParam & 0x80000000)) {
// '.'
			if (wParam == 0x6e) {
				return(TRUE);
			}
#if 0   //DISABLE
// '0'
			if ((wParam == 0x60) && (!Flag_0) && (pluspress == 0)) {
				return(TRUE);
			}
// '*' : stop playing
			if ((wParam == 0x6a) && (!Flag_Mul)) {
				SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON4, 0);
				return(TRUE);
			}
// '-' : shuffle toggle
			if ((wParam == 0x6d) && (!Flag_Sub)) {
				SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_FILE_SHUFFLE, 0);
				return(TRUE);
			}

// '+' : start/end track command
			if ((wParam == 0x6b) && (!Flag_Add)) {
				pluspress = pluspress ? 0 : 1;
				if (pluspress == 1) {
// we can type numbers in now
					tracknum = 0;
					return(TRUE);
				} else {
// get tracknum!
					switch (SendMessage(plugin.hwndParent, WM_USER, 0, IPC_ISPLAYING)) {
					case 3:
					case 1:
					case 0: {
						int trk, blah, cursong;

						trk = tracknum - 1;
						if (trk < 0) return(TRUE);

						SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON4, 0);
						shufmode = 0;
						for (blah = 0; blah < 5; blah++) {
// full-toggle shuffle-mode to upset it
							SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_FILE_SHUFFLE, 0);
							SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_FILE_SHUFFLE, 0);
							SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON1_CTRL, 0);
							SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON5, 0);
							cursong = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_WRITEPLAYLIST);
							if (cursong != 1) {
								shufmode = 1;
								SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_FILE_SHUFFLE, 0);
							}
						}

						SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON1_CTRL, 0);
						while (trk >= 10) {
							SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_JUMP10FWD, 0);
							trk -= 10;
						}
						while (trk--) {
							SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON5, 0);
						}
						SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON2, 0);
						if (shufmode) {
							SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_FILE_SHUFFLE, 0);
							shufmode = 0;
						}
						return(TRUE);
					}
					}
				}
			}
//			else
			if ((wParam >= 0x60) && (wParam <= 0x69) && (pluspress == 1)) {
				tracknum = (tracknum * 10) + (wParam - 0x60);
				return(TRUE);
			}

// trap digits
			if ((wParam == 0x60) && (Flag_0) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x61) && (Flag_1) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x62) && (Flag_2) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x63) && (Flag_3) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x64) && (Flag_4) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x65) && (Flag_5) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x66) && (Flag_6) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x67) && (Flag_7) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x68) && (Flag_8) && (!pluspress))
				return (TRUE);
			if ((wParam == 0x69) && (Flag_9) && (!pluspress))
				return (TRUE);

#endif  //DISABLE
		}
	}

	return(CallNextHookEx(KeyHook, code, wParam, lParam));
}

#endif

////////////////////////////////////////////////////////////////////////////////
// The MAIN function

int main(int argc, char *argv[]) {
	char                    *curArg;
	struct Cmd_Definition   *curCmd;
	int                     iRet = EXIT_SUCCESS;

// Show ubiquitous Copyright message or whatever

	// FIXME: need some checks here...
	iopl(3);

	printf("\n"
	       "+------------------------------------------------------------+-------+\n"
	       "| CatFlap for Linux           (Control for PSX & Caetla ROM) | v%d.%02d |\n"
	       "+------------------------------------------------------------+-------+\n\n"
	       , LVER, LREV);
// Init Args-handler

	m_Args.Create(argc, argv);

	m_Caetla.m_ConsoleHook = &ConHook;
//	m_Caetla.m_ConsoleKeyHook=&ConKeyHook;
	m_Caetla.m_ConsolePrintHook = &ConPrintHook;

	printf("Internal CCaetla Class version %d.%02d\n", m_Caetla.GetVersion() / 100, m_Caetla.GetVersion() % 100);

	BindIni(m_Args.GetNormalArg(0));
//	DumpIni();


// Initial simple check for args

	if (argc < 2) {
		usage();
		cleanup();
//		return(EXIT_FAILURE);
		iRet = EXIT_FAILURE;
		goto cleanexit;
	}


// Grab any environment settings we need

	{
		char    *EnvVal;


// Commslink Port

		ARPort = CAETLA_DEF_PORT;
		EnvVal = getenv("AR_PORT");
		if (EnvVal) {
			int Port;
			if (sscanf(EnvVal, "%x", &Port) == 1)
				ARPort = Port;
		}
		EnvVal = getenv("CF_PORT");
		if (EnvVal) {
			sscanf(EnvVal, "%s", PPdevice);
		} else {
			strncpy(PPdevice, CAETLA_DEF_PPDEV, MAX_PPDEV_SIZE);
		}

// Keyboard redirection

		m_Caetla.m_KeyIn = 0;
		EnvVal = getenv("AR_PSXKEYIN");
		if (EnvVal) {
			int KeyIn;
			if (sscanf(EnvVal, "%x", &KeyIn) == 1)
				m_Caetla.m_KeyIn = KeyIn;
		}
		EnvVal = getenv("CF_PSXKEYIN");
		if (EnvVal) {
			int KeyIn;
			if (sscanf(EnvVal, "%x", &KeyIn) == 1)
				m_Caetla.m_KeyIn = KeyIn;
		}

// Args redirection

		ARArgs = 0;
		EnvVal = getenv("AR_PSXARGS");
		if (EnvVal) {
			int Args;
			if (sscanf(EnvVal, "%x", &Args) == 1)
				ARArgs = Args;
		}
		EnvVal = getenv("CF_PSXARGS");
		if (EnvVal) {
			int Args;
			if (sscanf(EnvVal, "%x", &Args) == 1)
				ARArgs = Args;
		}

		EnvVal = getenv("CF_CARTTYPE");
		if (EnvVal) {
			if (strcasecmp(EnvVal, "PAR") == 0)
				m_CartType = CART_PAR;
			else if (strcasecmp(EnvVal, "XP") == 0)
				m_CartType = CART_XP;
		}

	}


// Scan for the command (1st non-switch argument)

	curArg = m_Args.GetNormalArg(1);

	if (!curArg) {
		printf("\nError: no command specified.\n");
		cleanup();
//		return(EXIT_FAILURE);
		iRet = EXIT_FAILURE;
		goto cleanexit;
	}

	curCmd = (struct Cmd_Definition *)CFCommands;

	for (curCmd = (struct Cmd_Definition *)CFCommands; curCmd->name; curCmd++) {
		if (strcasecmp(curArg, curCmd->name) == 0) {
// Found the command in the list

// Special case - if it's the RUN command, set the processing range of the CArgs
// handler to not parse past the PSX EXE filename

			if (curCmd->id == CMD_RUN) {
				int exename = m_Args.GetNormalArgIndex(2);
				if (exename == -1) {
					printf("Error - no EXE file specified.\n");
					cleanup();
//					return(EXIT_FAILURE);
					iRet = EXIT_FAILURE;
					goto cleanexit;
				}
				m_Args.SetRange(-1, exename);
			}

// Process the args for switches

			int sw = 0;
			while (Switches[sw].Name) {
// 29/9/00
// OK, this was messing up when you had more than one switch beginning the same, eg
// 'a' & 'as'. Even though both would be validated at some point, you would also get
// the 'a' callback being passed the 'as' part of the command line - each switch is
// checked against every part of the command line. So what I'm doing here is basically
// to remove a part of the command line from the CArgs switch list by zeroing the
// first character of it once we've found a match. Although this destroys the data
// held by CArgs, we don't actually need it after this and if we wanted to we can
// rebuild it with: m_Args.Create(argc,argv);

				m_Args.ProcessSwitch((struct Arg_SwitchAssign *)&Switches[sw++]);
			}

			m_Args.Create(argc, argv);


			if (m_Verbose) {
				printf("KEYIN Address: 0x%x\n", m_Caetla.m_KeyIn);
				printf(" ARGS Address: 0x%x\n", ARArgs);
			}

// Remember to set up the log file if asked for

			m_Caetla.m_LogFile = m_Log;

// Now call the function

			if (curCmd->func)  curCmd->func(argc, argv);

			cleanup();
//			return(EXIT_SUCCESS);
			iRet = EXIT_SUCCESS;
			goto cleanexit;

		}
	}


	usage();
//	cleanup();
//	return(EXIT_SUCCESS);

cleanexit:

//printf("%d presses\n",keytest);

	cleanup();
	return(iRet);
}


////////////////////////////////////////////////////////////////////////////////
// Show program usage/options etc

void usage(void) {
	printf("\nType CATFLAP HELP if you require assistance.\n");
}



void cleanup(void) {
	if (m_Ini) {
		free(m_Ini);
		m_Ini = NULL;
	}
}



////////////////////////////////////////////////////////////////////////////////
// Callback for Key Input in Caetla Console function
#if 0

short ConKeyHook(void) {
	unsigned char bKey;

	bKey = 0;

	if (kbhit()) {
		bKey = _getch();

		if (bKey == 0xe0) {
			ungetch(bKey);
			bKey = _getch();

//			printf("[%d]",bKey);

// 15/8/2000
// On new Athlon system with MS Natural Pro keyboard, all the cursors etc all seem
// to work again?
// 72 - cursor up
// 80 - cursor down
// 75 - cursor left
// 77 - cursor right
// 82 - insert
// 83 - delete
// 71 - home
// 79 - end
// 73 - page up
// 81 - page down


			switch (bKey) {
			case 72:        //CURSOR UP
				bKey = 0x91;
				break;
			case 80:        //CURSOR DOWN
				bKey = 0x92;
				break;
			case 75:        //CURSOR LEFT
				bKey = 0x93;
				break;
			case 77:        //CURSOR RIGHT
				bKey = 0x94;
				break;
			case 73:        //PAGE UP
				bKey = 0x95;
				break;
			case 81:        //PAGE DOWN
				bKey = 0x96;
				break;
			case 71:        //HOME
				bKey = 0x97;
				break;
			case 79:        //END
				bKey = 0x98;
				break;
			case 82:        //INSERT
				bKey = 0x99;
				break;
			case 83:        //DELETE
				bKey = 0x9a;
				break;


//				case 0x85:        //F11
			case 0x133:     //F11
				bKey = 0x8b;
				break;

//				case 0x86:        //F12
			case 0x134:     //F12
				bKey = 0x8c;
				break;

			default:
				;
				break;

			}
		} else if (bKey == 0x00) {
			bKey = _getch();
			ungetch(bKey);
			bKey = _getch();

			switch (bKey) {
// Special case for numeric pad cursors
#if 0   //DISABLE
			case 0x48:
				bKey = 24; //0x81;//VK_UP;
				break;

			case 0x50:
				bKey = 25; //0x82;//VK_DOWN;
				break;

			case 0x4b:
				bKey = 27; //0x83;//VK_LEFT;
				break;

			case 0x4d:
				bKey = 26; //0x84;//VK_RIGHT;
				break;
#else
			case 0x48:
				bKey = 0x91; //0x81;//VK_UP;
				break;

			case 0x50:
				bKey = 0x92; //0x82;//VK_DOWN;
				break;

			case 0x4b:
				bKey = 0x93; //0x83;//VK_LEFT;
				break;

			case 0x4d:
				bKey = 0x94; //0x84;//VK_RIGHT;
				break;
#endif  //DISABLE

// Special case for F1-F10
			case 0x3b:
				bKey = 0x81;
				break;

			case 0x3c:
				bKey = 0x82;
				break;

			case 0x3d:
				bKey = 0x83;
				break;

			case 0x3e:
				bKey = 0x84;
				break;

			case 0x3f:
				bKey = 0x85;
				break;

			case 0x40:
				bKey = 0x86;
				break;

			case 0x41:
				bKey = 0x87;
				break;

			case 0x42:
				bKey = 0x88;
				break;

			case 0x43:
				bKey = 0x89;
				break;

			case 0x44:
				bKey = 0x8a;
				break;

			}
		}
	}

	return((sint16)bKey);
}

#endif

////////////////////////////////////////////////////////////////////////////////
// Callback for Print Input in Caetla Console function

void ConPrintHook(char *string) {
	printf("%s", string);
}










int BK_Down(struct Binding *, FILE *);
int BK_Up(struct Binding *, FILE *);
int BK_VDown(struct Binding *, FILE *);
int BK_Reset(struct Binding *, FILE *);
int BK_Write8(struct Binding *, FILE *);
int BK_Write16(struct Binding *, FILE *);
int BK_Write32(struct Binding *, FILE *);
int BK_Fill8(struct Binding *, FILE *);
int BK_Print(struct Binding *, FILE *);
int BK_SysCall(struct Binding *, FILE *);




enum {
	BINDING_NONE = 0,
	BINDING_METHOD,
	BINDING_FUNCTION,
//	BINDING_,
};

typedef struct KeyMap {
	const char          *Name;
	uint8   Key;
} KeyMap;


typedef struct Binding {
	char            FullLine[256];
	char            ArgsLine[256];
	int             Type;
	uint8   Key;
	struct FuncMap  *FuncMap;
	char            CallFunc[64];
	int             Numer;
} Binding;


typedef struct FuncMap {
	const char          *Name;
	int (*Func)(struct Binding *, FILE *);
} FuncMap;

typedef struct FuncBind {
	char            Name[64];
	char            *IniJump;
} FuncBind;


#define         MAX_BINDINGS    16
#define         MAX_FUNCTIONS   16
struct Binding  Bindings[MAX_BINDINGS];
struct FuncBind Functions[MAX_FUNCTIONS];

int             m_NumBindings = 0;
int             m_NumFunctions = 0;
int             inFunc = 0;

struct FuncMap BindFuncs[] = {
	{ "DOWN", BK_Down },
	{ "UP", BK_Up },
	{ "VDOWN", BK_VDown },
	{ "Reset", BK_Reset },
	{ "Write8", BK_Write8 },
	{ "Write16", BK_Write16 },
	{ "Write32", BK_Write32 },
	{ "Fill8", BK_Fill8 },
	{ "print", BK_Print },
	{ "syscall", BK_SysCall },
	{ NULL, NULL }
};

struct KeyMap BindKeys[] = {
	{ "F1" , 0x81 },
	{ "F2" , 0x82 },
	{ "F3" , 0x83 },
	{ "F4" , 0x84 },
	{ "F5" , 0x85 },
	{ "F6" , 0x86 },
	{ "F7" , 0x87 },
	{ "F8" , 0x88 },
	{ "F9" , 0x89 },
	{ "F10" , 0x8a },
	{ "F11" , 0x8b },
	{ "F12" , 0x8c },
	{ NULL, 0 }
};




int BindIni(char *name) {
	return FALSE;
}


void DumpIni(void) {
	return;
}




void ParseLine(char *line, char *real) {
	char    key[64];
	char    type[64];
	char    method[64];
	char    myargs[256];
	struct KeyMap *km;
	struct Binding *bi;
	struct FuncMap *fm;
	int     sret;
	int     i;


//	printf("Line: '%s'\n",line);

	memset(&key[0], 0, 64);
	memset(&type[0], 0, 64);
	memset(&method[0], 0, 64);
	memset(&myargs[0], 0, 256);

	sret = sscanf((char *)line, " Key ( %[^)] ) = %[^ ^(] ( %[^ ^)] ) %[^\n] ", &key[0], &type[0], &method[0], &myargs[0]);

	if (sret) {
		km = &BindKeys[0];

		for (km = &BindKeys[0]; km->Name; km++) {
			if (strcasecmp(km->Name, &key[0]) == 0) {
				for (bi = Bindings; bi < Bindings + m_NumBindings; bi++) {
					if (km->Key == bi->Key) {
						printf("Duplicate KeyBinding: '%s'\n", line);
						return;
					}
				}

				if (strcasecmp(&type[0], "method") == 0) {
					for (fm = BindFuncs; fm->Name; fm++) {
						if (strcasecmp(fm->Name, &method[0]) == 0) {
							strcpy(&Bindings[m_NumBindings].FullLine[0], line);
							strcpy(&Bindings[m_NumBindings].ArgsLine[0], &myargs[0]);
							Bindings[m_NumBindings].Type = BINDING_METHOD;
							Bindings[m_NumBindings].Key = km->Key;
							Bindings[m_NumBindings].FuncMap = fm;
							Bindings[m_NumBindings].Numer = 0;
							memset(&Bindings[m_NumBindings].CallFunc[0], 0, 64);
//							printf("Got '%s'\n",fm->Name);
							m_NumBindings++;
							return;
						}
					}
					printf("Unknown Binding Function: %s\n", line);
					return;
				} else if (strcasecmp(&type[0], "function") == 0) {
					strcpy(&Bindings[m_NumBindings].FullLine[0], line);
					strcpy(&Bindings[m_NumBindings].ArgsLine[0], &myargs[0]);
					Bindings[m_NumBindings].Type = BINDING_FUNCTION;
					Bindings[m_NumBindings].Key = km->Key;
					Bindings[m_NumBindings].FuncMap = NULL;
					Bindings[m_NumBindings].Numer = 0;
					strcpy(&Bindings[m_NumBindings].CallFunc[0], &method[0]);
//					printf("Bound Function '%s'\n",&Bindings[m_NumBindings].CallFunc[0]);
					m_NumBindings++;
					return;
				} else {
					printf("Unknown Binding Type: %s\n", line);
					return;
				}

			}
		}

		if (strlen(&key[0]))
			printf("Invalid Key in Line: %s\n", line);
	}

	sret = sscanf((char *)line, " StartFunc %s ", &method[0]);

	if (sret == 1) {
		if (inFunc) {
			printf("Configuration Error: Function defined within Function.\n");
			return;
		}
		inFunc++;

		for (i = 0; i < m_NumFunctions; i++) {
			if (strcasecmp(&Functions[i].Name[0], &method[0]) == 0) {
				printf("Error: Function redefined: '%s'\n", &method[0]);
				return;
			}
		}

		strcpy(&Functions[m_NumFunctions].Name[0], &method[0]);
		Functions[m_NumFunctions].IniJump = real;

//		printf("Function definition: '%s' '%s'\n",&method[0]);
//		printf("Function definition: '%s' '%s'\n",&Functions[m_NumFunctions].Name[0],Functions[m_NumFunctions].IniJump);

		m_NumFunctions++;
		return;
	}

	sret = sscanf((char *)line, " %s ", &method[0]);

	if ((sret == 1) && (strcasecmp(&method[0], "EndFunc") == 0)) {
		if (!inFunc) {
			printf("Error: Spurious EndFunc\n");
			return;
		}
		inFunc--;
		return;
	}

	return;
}



int BK_Down(struct Binding *bind, FILE *file) {
	char            fnam[256];
	char            fnam2[256];
	char            addr[32];
	char            size[32];
	uint32   iaddr, isize;

	static          int m_Numer = 0;


	if (3 == sscanf((char *)&bind->ArgsLine, "%s %s %s", fnam, addr, size)) {
		iaddr = ParseNumber(addr);
		isize = ParseNumber(size);
//		sprintf(&fnam2,&fnam,bind->Numer++);
		sprintf(&fnam2[0], &fnam[0], m_Numer++);
		ConOut(file, "Data Download %d bytes to '%s' , from address 0x%x\n", isize, &fnam2, iaddr);

//		m_Caetla.Detect();
//		if  (m_Caetla.ShowError())  return(EXIT_FAILURE);

#if 1   //DISABLE
		m_Caetla.ReceiveFile(&fnam2[0], iaddr, isize);
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);
//		ConOut(file,"...SUCCESS...\n",CAETLA_DOSCON_PROMPT);

		m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(FALSE);
#endif  //DISABLE

#if 0   //DISABLE
		m_Caetla.SendFile(&fnam2[0], iaddr);
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);
//		ConOut(file,"...SUCCESS...\n",CAETLA_DOSCON_PROMPT);

		m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(FALSE);
#endif  //DISABLE

		ConOut(file, "%s", CAETLA_DOSCON_PROMPT);
		return(TRUE);
	}
	return(FALSE);

}


int BK_Up(struct Binding *bind, FILE *file) {
	char            fnam[256];
	char            fnam2[256];
	char            addr[32];
	char            size[32];
	uint32   iaddr, isize;

	static          int m_Numer = 0;


	if (2 == sscanf((char *)&bind->ArgsLine, "%s %s", fnam, addr)) {
		iaddr = ParseNumber(addr);
//		sprintf(&fnam2,&fnam,bind->Numer++);
		sprintf(&fnam2[0], &fnam[0], m_Numer++);
		ConOut(file, "Data Upload from file '%s', to address 0x%x\n", &fnam2, iaddr);

//		m_Caetla.Detect();
//		if  (m_Caetla.ShowError())  return(EXIT_FAILURE);

#if 0   //DISABLE
		m_Caetla.ReceiveFile(&fnam2[0], iaddr, isize);
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);
//		ConOut(file,"...SUCCESS...\n",CAETLA_DOSCON_PROMPT);

		m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(FALSE);
#endif  //DISABLE

#if 1   //DISABLE
		m_Caetla.SendFile(&fnam2[0], iaddr);
		if (m_Caetla.ShowError())  return(EXIT_FAILURE);
//		ConOut(file,"...SUCCESS...\n",CAETLA_DOSCON_PROMPT);

		m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(FALSE);
#endif  //DISABLE

		ConOut(file, "%s", CAETLA_DOSCON_PROMPT);
		return(TRUE);
	}
	return(FALSE);

}



int BK_VDown(struct Binding *bind, FILE *file) {
	char    fnam[256];
	char    fnam2[256];
	char    xt[32];
	char    yt[32];
	char    wt[32];
	char    ht[32];
	char    dt[32];
	int     x, y, w, h, d;

	static  int m_Numer = 0;

	if (6 == sscanf((char *)&bind->ArgsLine, "%s %s %s %s %s %s", fnam, xt, yt, wt, ht, dt)) {
		x = ParseNumber(xt);
		y = ParseNumber(yt);
		w = ParseNumber(wt);
		h = ParseNumber(ht);
		d = ParseNumber(dt);
		sprintf(&fnam2[0], &fnam[0], m_Numer++);
		ConOut(file, "VRAM Download to '%s' , %d,%d %dx%d %d-bit\n", &fnam2, x, y, w, h, d);
		m_Caetla.ReceiveVRAM(&fnam2[0], x, y, w, h, d);
		if (m_Caetla.ShowError())  return(FALSE);
//		m_Caetla.Resume();
//		if  (m_Caetla.ShowError())  return(FALSE);
		ConOut(file, "%s", CAETLA_DOSCON_PROMPT);
		return(TRUE);
	}
	return(FALSE);
}


int BK_Reset(struct Binding *bind, FILE *file) {
	m_Caetla.Reset();
	if (m_Caetla.ShowError())  return(FALSE);
	m_Caetla.Resume();
	if (m_Caetla.ShowError())  return(FALSE);
	return(TRUE);
}


int BK_Write8(struct Binding *bind, FILE *file) {
	char    addr[32];
	char    val[32];
	int     ad, uc;

	if (2 == sscanf((char *)&bind->ArgsLine, "%s %s", addr, val)) {
		ad = ParseNumber(addr);
		uc = ParseNumber(val);
		m_Caetla.Listen(CAETLA_MODE_DEBUG);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.UpdateByte(ad, uc & 0xff);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(FALSE);
//		ConOut(file,"Write 0x%x to 0x%x\n%s",uc,ad,CAETLA_DOSCON_PROMPT);
		return(TRUE);
	}
	return(FALSE);
}

int BK_Write16(struct Binding *bind, FILE *file) {
	char    addr[32];
	char    val[32];
	int     ad, uc;

	if (2 == sscanf((char *)&bind->ArgsLine, "%s %s", addr, val)) {
		ad = ParseNumber(addr);
		uc = ParseNumber(val);
		m_Caetla.Listen(CAETLA_MODE_DEBUG);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.UpdateByte(ad, uc & 0xff);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.UpdateByte(ad + 1, (uc >> 8) & 0xff);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(FALSE);
//		ConOut(file,"Write 0x%x to 0x%x\n%s",uc,ad,CAETLA_DOSCON_PROMPT);
		return(TRUE);
	}
	return(FALSE);
}

int BK_Write32(struct Binding *bind, FILE *file) {
	char    addr[32];
	char    val[32];
	int     ad, uc;

	if (2 == sscanf((char *)&bind->ArgsLine, "%s %s", addr, val)) {
		ad = ParseNumber(addr);
		uc = ParseNumber(val);
		m_Caetla.Listen(CAETLA_MODE_DEBUG);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.UpdateByte(ad, uc & 0xff);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.UpdateByte(ad + 1, (uc >> 8) & 0xff);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.UpdateByte(ad + 2, (uc >> 16) & 0xff);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.UpdateByte(ad + 3, (uc >> 24) & 0xff);
		if (m_Caetla.ShowError())  return(FALSE);
		m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(FALSE);
//		ConOut(file,"Write 0x%x to 0x%x\n%s",uc,ad,CAETLA_DOSCON_PROMPT);
		return(TRUE);
	}
	return(FALSE);
}


int BK_Fill8(struct Binding *bind, FILE *file) {
	char    addr[32];
	char    addr2[32];
	char    val[32];
	int     ad, ad2, uc;

	if (3 == sscanf((char *)&bind->ArgsLine, "%s %s %s", addr, addr2, val)) {
		ad = ParseNumber(addr);
		ad2 = ParseNumber(addr2);
		uc = ParseNumber(val);
		m_Caetla.Listen(CAETLA_MODE_DEBUG);
		if (m_Caetla.ShowError())  return(FALSE);
		while (ad < ad2) {
			m_Caetla.UpdateByte(ad++, uc & 0xff);
			if (m_Caetla.ShowError())  return(FALSE);
		}
		m_Caetla.Resume();
		if (m_Caetla.ShowError())  return(FALSE);
//		ConOut(file,"Write 0x%x to 0x%x\n%s",uc,ad,CAETLA_DOSCON_PROMPT);
		return(TRUE);
	}
	return(FALSE);
}

int BK_Print(struct Binding *bind, FILE *file) {
	ConOut(file, "%s\n%s", &bind->ArgsLine[0], CAETLA_DOSCON_PROMPT);
	return(TRUE);
}

int BK_SysCall(struct Binding *bind, FILE *file) {
	int res = system(&bind->ArgsLine[0]);
	ConOut(file, "\n%s", CAETLA_DOSCON_PROMPT);
	return(TRUE);
}


int ConHook(uint8 key, FILE *file) {
	struct Binding *bi;

	for (bi = Bindings; bi < Bindings + m_NumBindings; bi++) {
		if (bi->Key == key) {
			switch (bi->Type) {
			case BINDING_METHOD:
				return(bi->FuncMap->Func(bi, file));
				break;

			case BINDING_FUNCTION:
//					return(bi->FuncMap->Func(bi,file));
				CallFunction(file, &bi->CallFunc[0], &bi->ArgsLine[0]);
//					ConOut(file,"Call Function '%s'\n%s",&bi->CallFunc[0],CAETLA_DOSCON_PROMPT);
				break;
			}
		}
	}
	return(FALSE);
}




void ConOut(FILE *file, const char *fmt, ...) {
	char    dump[2048];

	va_list args;

	va_start(args, fmt);
	vsprintf(dump, fmt, args);
	va_end(args);

	printf("%s", dump);

	if (file) {
		fprintf(file, "%s", dump);
		fflush(file);
	}
}



int CallFunction(FILE *file, char *func, char *args) {
	int f;
	int sret;
	struct FuncBind *fb;
	char    *line;
	char    temp[256];
	char    type[256];
	char    method[256];
	char    myargs[256];

//	printf("Calling %s\n",func);

	for (f = 0; f < m_NumFunctions; f++) {
#if 1
		if (strcasecmp(&Functions[f].Name[0], func) == 0) {
			fb = &Functions[f];
//			line=Functions[f].IniJump;
//printf("Got '%s'\n",line);
//			line=NextLine(line);
//printf("Got '%s'\n",line);
//return(TRUE);
			line = NextLine(fb->IniJump);
			while (1) {
//				ConOut(file,": %s\n",line);

				sret = sscanf(line, " %s ", &temp[0]);

//				ConOut(file,": %s\n",&temp[0]);

				if (strcasecmp(&temp[0], "endfunc") == 0) {
					return(TRUE);
				}

				memset(&type[0], 0, 256);
				memset(&method[0], 0, 256);
				memset(&myargs[0], 0, 256);

				sret = sscanf((char *)line, " %[^ ^(] ( %[^ ^)] ) %[^\n] ", &type[0], &method[0], &myargs[0]);

//ConOut(file,"?? '%s'\n",&type[0]);

				if (sret) {
					if (strcasecmp(&type[0], "method") == 0) {
						struct FuncMap *fm;
						for (fm = BindFuncs; fm->Name; fm++) {
							if (strcasecmp(fm->Name, &method[0]) == 0) {
								struct Binding tempbind;
								memset(&tempbind, 0, sizeof(tempbind));
//								strcpy(&tempbind.FullLine[0],line);
								strcpy(&tempbind.ArgsLine[0], &myargs[0]);
								tempbind.Type = BINDING_METHOD;
								tempbind.Key = 0;
								tempbind.FuncMap = fm;
								tempbind.Numer = 0;
								memset(&tempbind.CallFunc[0], 0, 64);
								fm->Func(&tempbind, file);
								break;
							}
						}
//						printf("Unknown Binding Function: %s\n",line);
//						return;
					} else if (strcasecmp(&type[0], "function") == 0) {
						CallFunction(file, &method[0], &myargs[0]);
					}
				}

				line = NextLine(line);
			}
		}
#else
		printf("Func: '%s', %x\n", &Functions[f].Name[0], Functions[f].IniJump);
#endif
	}
	return(FALSE);
}



char *NextLine(char *in) {
	char    line[256];

	memset(&line[0], 0, 256);
	sscanf(in, "%[^\r]", &line[0]);
//	printf("Line %4d: '%s'\n",linenum,&line[0]);
	in += strlen(&line[0]);
	sscanf(in, "%[\r]", &line[0]);
	in += strlen(&line[0]);
	sscanf(in, "%[\n]", &line[0]);
	in += strlen(&line[0]);
	return(in);
}




int FileExists(char *fname) {
	FILE    *fp;
	if (fp = fopen(fname, "rb")) {
		fclose(fp);
		return(1);
	} else {
		return(0);
	}
}


