#define USING_XP    0
#define DEBUG_XP    0


////////////////////////////////////////////////////////////////////////////////
//
// CCaetla Class
// - handles communication with the Caetla ROM on a PSX Action Replay
//
// (C) 1998-2000 Intar Technologies Limited
//
//
////////////////////////////////////////////////////////////////////////////////

#define     REV_MAJOR       2
#define     REV_MINOR       36

#define     SHOW_CON_DBG    0

#include    <stdio.h>
#include    <stdarg.h>
#include    <stdlib.h>
#include    <string.h>
#include    <fcntl.h>
#include    <sys/stat.h>
#include    <sys/io.h>
#include    <sys/ioctl.h>
#include    <sys/types.h>
#include    <sys/time.h>
#include    <unistd.h>
#include    <linux/parport.h>
#include    <linux/ppdev.h>

#include    "CCaetla.h"

#define     complete(x) { m_ErrorCode=x; return(x); }

#define     _SECONDS(x) ((int)((float)x*1000))

extern char PPdevice[MAX_PPDEV_SIZE];

static int          pp_fd;
static uint8   last_data;

void    InitPP(void);

uint32 GetTickCount(void) {
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}


void InitPP(void) {
	fprintf(stdout, "Initializing device %s ...\n", PPdevice);

	pp_fd = open(PPdevice, O_RDWR);
	if (pp_fd == -1) {
		perror("open");
		exit(1);
	}

	if (ioctl(pp_fd, PPCLAIM)) {
		perror("PPCLAIM");
		close(pp_fd);
		exit(1);
	}

	last_data = 0;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor

CCaetla::CCaetla() {

// Set up any variables

// m_Status is CURRENTLY UNUSED
//	m_Status=STATUS_BAD;
	m_ErrorCode = CAETLA_ERROR_OK;
	m_FilePtr = NULL;
	m_Port = CAETLA_DEF_PORT;
	m_CartType = CAETLA_DEF_CART;
	m_TimeOut = _SECONDS(5);
// m_RealTimeOut is a legacy. It is currently *always* true, phase out once
// 100% certain it's outstayed its welcome.
	m_RealTimeOut = true;
	m_InConsole = false;
	m_KeyIn = 0;
// m_KeyEnable is CURRENTLY UNUSED
	m_KeyEnable = 1;
	m_LogFile = NULL;
	m_FpLog = NULL;
	m_ConsoleHook = NULL;
	m_ConsoleKeyHook = NULL;
	m_ConsolePrintHook = NULL;
	m_ProgressCharS = ']';
	m_ProgressCharE = '[';
	m_ProgressCharO = '=';
	m_ProgressCharI = 0x10;
	m_ShowProgress = true;
// m_RunningAsDLL is CURRENTLY UNUSED
	m_RunningAsDLL = 0;
	m_AutoSense = true;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor

CCaetla::~CCaetla() {

// If we have a resident file, remove it

	if (m_FilePtr) {
		free(m_FilePtr);
		m_FilePtr = NULL;
	}

	if (m_FpLog) {
		fclose(m_FpLog);
		m_FpLog = NULL;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Open the LogFile session
// Return: 0==no file opened, non-0==file opened

int CCaetla::OpenLogFile(void) {
	CloseLogFile();

	if (m_LogFile) {
		FILE *fp;

		fp = fopen(m_LogFile, "a+b");
		if (fp) {
			m_FpLog = fp;
			fprintf(m_FpLog, "\n\n");
			fprintf(m_FpLog, "************************************************************************\n");
			fprintf(m_FpLog, "************************************************************************\n");
			fprintf(m_FpLog, "**\n");
			fprintf(m_FpLog, "** CatFlap Console Session Output Log\n");
			fprintf(m_FpLog, "**\n");
			fprintf(m_FpLog, "************************************************************************\n");
			fprintf(m_FpLog, "************************************************************************\n\n");
			fflush(m_FpLog);
			return(1);
		}
	}
	return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Close the LogFile session

int CCaetla::CloseLogFile(void) {
	if (m_FpLog) {
		fclose(m_FpLog);
		m_FpLog = NULL;
	}
	return(1);
}


////////////////////////////////////////////////////////////////////////////////
// Open the Kernel-Mode device driver for NT4/Win2000 Support

int CCaetla::OpenNTDriver(void) {
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Close the Kernel-Mode device driver

int CCaetla::CloseNTDriver(void) {
	return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Start the Kernel-Mode Service

int CCaetla::StartNTService(void) {
	return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Stop the Kernel-Mode Service

int CCaetla::StopNTService(void) {
	return 1;
}



////////////////////////////////////////////////////////////////////////////////
// Caetla Errors (& messages)

struct CaetlaErrorMsg CaetlaErrs[] = {
//	{ CAETLA_ERROR_OK            , "OK" },
	{ CAETLA_ERROR_UNKNOWN      , "Undefined Error" },
	{ CAETLA_ERROR_BADSTATUS    , "Error: CCaetla Class has Illegal Status" },
	{ CAETLA_ERROR_TIMEOUT      , "Error: Communications Timeout" },
	{ CAETLA_ERROR_PROTOCOL     , "Error: Communications Protocol Error" },
	{ CAETLA_ERROR_FILE         , "Error: File Error" },
	{ CAETLA_ERROR_FILEFORMAT   , "Error: Illegal File Format" },
	{ CAETLA_ERROR_FILENOTEXIST , "Error: File Not Found" },
	{ CAETLA_ERROR_LOWRAM       , "Error: Low Memory" },
	{ CAETLA_ERROR_NTNODRIVER   , "Error initialising NT I/O Driver" },
	{ CAETLA_ERROR_NTIO         , "Error performing NT I/O" },
	{ CAETLA_ERROR_OUTOFRANGE   , "Error: value out of range" },
	{ CAETLA_ERROR_MC_NOCARD    , "Error: no Memory Card present" },
	{ CAETLA_ERROR_MC_CARDFULL  , "Error: insufficient space on Memory Card" },
	{ CAETLA_ERROR_MC_NOTFORMATTED  , "Error: Memory Card is not formatted" },
	{ CAETLA_ERROR_MC_FILEEXISTS    , "Error: Filename already present on Memory Card" },
	{ -1, NULL },
};

////////////////////////////////////////////////////////////////////////////////
// Show last error code as a message

int CCaetla::ShowError(void) {
	struct CaetlaErrorMsg *list = &CaetlaErrs[0];

	while (list->ErrNum != -1) {
		if (list->ErrNum == m_ErrorCode) {
			Dump("%s\n", list->ErrMsg);
		}
		list++;
	}

	if (m_ErrorCode == CAETLA_ERROR_OK)
		return(0);
	else
		return(-1);
}


////////////////////////////////////////////////////////////////////////////////
// Init the CCaetla class, specifying a port address and cartridge type

void CCaetla::Init(int port, int cart) {
	m_ErrorCode = CAETLA_ERROR_OK;

	m_Port = port;

	if ((cart >= 0) && (cart < CART_MAX))
		m_CartType = cart;

	// Init ppdev
	InitPP();

	CloseLogFile();

	OpenLogFile();

	CreateXPLookup();
}


////////////////////////////////////////////////////////////////////////////////
// Return Version Number

int CCaetla::GetVersion(void) {
	return((REV_MAJOR * 100) + (REV_MINOR % 100));
}


////////////////////////////////////////////////////////////////////////////////
// Detect Caetla's presence

int CCaetla::Detect(void) {
	uint8 r;

//printf("d1");
	r = Swap8('C');
	if (m_ErrorCode != CAETLA_ERROR_OK)  return(m_ErrorCode);
//printf("d2");
	if (r != 'd') {
		m_ErrorCode = CAETLA_ERROR_PROTOCOL;
		return(m_ErrorCode);
	}
//printf("d3");

	r = Swap8('L');
	if (m_ErrorCode != CAETLA_ERROR_OK)  return(m_ErrorCode);
//printf("d4");
	if (r != 'o') {
		m_ErrorCode = CAETLA_ERROR_PROTOCOL;
		return(m_ErrorCode);
	}
//printf("d5");

	r = Swap8(0x08);
//	r=Swap8(0x0d);

//printf("d6");
//printf("err %d\n",m_ErrorCode);

#if 0   //DISABLE
	if (r) {
		m_ErrorCode = CAETLA_ERROR_PROTOCOL;
		return(m_ErrorCode);
	}
#endif  //DISABLE

//	IssueCommand(CAETLA_NOP);
//	Resume();

	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Query current Caetla mode

int CCaetla::QueryMode(void) {
	uint8 r, s;

//printf("q1");
	IssueCommand(CAETLA_REQUESTPCCONTROL);
//printf("q2");
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("q3");
	r = Swap8(0xff);
//printf("q4");
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("q5");
	switch (m_CartType) {
//printf("q6");
	case CART_XP:
//			r=ByteIn(m_Port);
		s = ByteIn(m_Port);
//			r=Swap8(0);
//printf("q7");
//			s=ByteIn(m_Port);
//			s=0;
//			s=Swap8(0);
//printf("q8");
		break;

	default:
	case CART_PAR:
		r = Swap8(0);
		s = Swap8(0);
		break;
	}
//printf("q9");

//	printf("Mode == %d [0x%02x]\n",r,s);

	if (s == 0)
		return(r);
	else
		complete(CAETLA_ERROR_UNKNOWN);
}

////////////////////////////////////////////////////////////////////////////////
// Force either Main or Debug mode

int CCaetla::ChooseMainOrDebug(void) {
	int curMode;

//printf("Q1");
	curMode = QueryMode();
//printf("Q2");
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("Q3");

	if (curMode != CAETLA_MODE_MAIN) {
//printf("Q4");
		Listen(CAETLA_MODE_DEBUG);
//printf("Q5");
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
//printf("Q6");

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Set Caetla PC-server mode

int CCaetla::ServerMode(int mode) {
	uint8 r;

	IssueCommand(CAETLA_CONSOLEMODE);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	if (mode)
		r = Send8(1);
	else
		r = Send8(0);
	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Resume Caetla/EXE execution

int CCaetla::Resume(void) {
	IssueCommand(CAETLA_NOP);
	if (m_ErrorCode != CAETLA_ERROR_OK)  return(m_ErrorCode);
	IssueCommand(CAETLA_RESUME);
	return(m_ErrorCode);
}




////////////////////////////////////////////////////////////////////////////////
// Build lookup table for XPlorer protocol

void CCaetla::CreateXPLookup(void) {
	int             dx;
//	uint8    byte,b1,b2,b3,b4;
//	uint16   w1;

	uint8   XpAs[] = {0x04, 0x05, 0x06, 0x07, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0a, 0x0b};

	for (dx = 0; dx < 256; dx++) {
		m_XPLookups[dx] = XpAs[dx/16];
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Abstract functions to read/write a raw byte to a port address
//
// These are the lowest level we can go, and cover us against PAR,XP,NT
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Read a raw port byte

uint8 CCaetla::RawPortIn(int port) {
	uint8 read_data = 0;

	switch (port) {
	case 0x378: // Data
		return last_data;
		break;
	case 0x379: // Status
		ioctl(pp_fd, PPRSTATUS, &read_data);
		break;
	case 0x37A: // Control
		ioctl(pp_fd, PPRCONTROL, &read_data);
		break;
	};

	return read_data;
}

////////////////////////////////////////////////////////////////////////////////
// Write a byte

void CCaetla::RawPortOut(int port, uint8 data) {
	int direction = 0;

	switch (port) {
	case 0x378: // Data
		last_data = data;
		ioctl(pp_fd, PPDATADIR, &direction);
		ioctl(pp_fd, PPWDATA, &data);
		break;
	case 0x379: // Status
		// no writes here...
		break;
	case 0x37A: // Control
		ioctl(pp_fd, PPWCONTROL, &data);
		break;
	};
}


////////////////////////////////////////////////////////////////////////////////
// XPlorer handshaking routines
// These seem a bit messy, but appear to work OK...

uint8 CCaetla::XpAck1(void) {
	uint8   ina, inb, inc;
	uint32   oldTime;

	if (m_RealTimeOut) {
		oldTime = GetTickCount();

		do {
			ina = RawPortIn(m_Port + 1);
			inb = m_XPLookups[ina];
			if ((inb & 0x08) != 0) {
				inc = RawPortIn(m_Port + 1);
				if (inc == ina)
					return(inc);
			}
		} while (GetTickCount() < oldTime + m_TimeOut);

		m_ErrorCode = CAETLA_ERROR_TIMEOUT;
		return(inc);
	} else {
		oldTime = m_TimeOut;

		do {
			ina = RawPortIn(m_Port + 1);
			inb = m_XPLookups[ina];
			if ((inb & 0x08) != 0) {
				inc = RawPortIn(m_Port + 1);
				if (inc == ina)
					return(inc);
			}
		} while (oldTime--);

		m_ErrorCode = CAETLA_ERROR_TIMEOUT;
		return(inc);
	}

}

uint8 CCaetla::XpAck1_N(void) {
	uint8   ina, inb, inc;
	uint32   oldTime;

	if (m_RealTimeOut) {
		oldTime = GetTickCount();

		do {
			ina = RawPortIn(m_Port + 1);
			inb = m_XPLookups[ina];
			if ((inb & 0x08) == 0) {
				inc = RawPortIn(m_Port + 1);
				if (inc == ina)
					return(inc);
			}
		} while (GetTickCount() < oldTime + m_TimeOut);

		m_ErrorCode = CAETLA_ERROR_TIMEOUT;
		return(inc);
	} else {
		oldTime = m_TimeOut;

		do {
			ina = RawPortIn(m_Port + 1);
			inb = m_XPLookups[ina];
			if ((inb & 0x08) == 0) {
				inc = RawPortIn(m_Port + 1);
				if (inc == ina)
					return(inc);
			}
		} while (oldTime--);

		m_ErrorCode = CAETLA_ERROR_TIMEOUT;
		return(inc);
	}

}

uint8 CCaetla::XpAck2(void) {
	uint8   ina, inb, inc;
	uint32   oldTime;

	if (m_RealTimeOut) {
		oldTime = GetTickCount();

		do {
			ina = RawPortIn(m_Port + 1);
			inb = m_XPLookups[ina];
			if (inb != 0x08) {
				inc = RawPortIn(m_Port + 1);
				if (inc == ina)
					return(inc);
			}
		} while (GetTickCount() < oldTime + m_TimeOut);

		m_ErrorCode = CAETLA_ERROR_TIMEOUT;
		return(inc);
	} else {
		oldTime = m_TimeOut;

		do {
			ina = RawPortIn(m_Port + 1);
			inb = m_XPLookups[ina];
			if (inb != 0x08) {
				inc = RawPortIn(m_Port + 1);
				if (inc == ina)
					return(inc);
			}
		} while (oldTime--);

		m_ErrorCode = CAETLA_ERROR_TIMEOUT;
		return(inc);
	}

}

#if 0   //DISABLE
uint8 CCaetla::XpAck2_N(void) {
	uint8   ina, inb, inc;

getit:
	ina = RawPortIn(m_Port + 1);
	inb = m_XPLookups[ina];
#if ( USING_XP && DEBUG_XP )
	Dump("_2 0x%x (0x%x)\n", inb, ina);
#endif
	if ((inb & 0x08) != 0) goto    getit;

	inc = RawPortIn(m_Port + 1);
	if (inc != ina)  goto    getit;

	return(inc);
}
#endif  //DISABLE



////////////////////////////////////////////////////////////////////////////////
// Get a Byte from I/O

uint8 CCaetla::ByteIn(int port) {
	switch (m_CartType) {
	case CART_XP: {
		uint8 in1, in2, in3, tmp;

		in1 = XpAck1();
		RawPortOut(port + 2, RawPortIn(port + 2) & 0xf7);
		in2 = XpAck1_N();
		RawPortOut(port + 2, RawPortIn(port + 2) | 0x08);
		in3 = XpAck1();
		RawPortOut(port + 2, RawPortIn(port + 2) & 0xf7);
		XpAck1_N();
		RawPortOut(port + 2, RawPortIn(port + 2) | 0x08);
//			XpAck2_N();
		tmp = (((in1 & 0x30) << 2) | (((~in2) & 0x80) >> 2) | ((in2 & 0x30) >> 1) | (((~in3) & 0x80) >> 5) | ((in3 & 0x30) >> 4));
		return(tmp);
	}
	break;

	default:
	case CART_PAR:
		return(RawPortIn(port));
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Put a Byte to I/O

void CCaetla::ByteOut(int port, uint8 data) {
	switch (m_CartType) {
	case CART_XP: {
		RawPortOut(port, data);
		RawPortOut(port + 2, RawPortIn(port + 2) & 0xf7);
		XpAck1();
		RawPortOut(port + 2, RawPortIn(port + 2) | 0x08);
		XpAck2();
	}
	break;

	default:
	case CART_PAR:
		RawPortOut(port, data);
		break;
	}
}



////////////////////////////////////////////////////////////////////////////////
// Wait for handshake

int CCaetla::Handshake(int port) {
	switch (m_CartType) {
	case CART_XP:
		return(0);
		break;

	default:
	case CART_PAR:
		return(RawPortIn(port + 2) & 0x01);
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Exchange Byte
// Used for both Send & Receive, as each mode ignores the inappropriate data

uint8 CCaetla::Swap8(uint8 data) {
	uint32 timer = m_TimeOut;

	m_ErrorCode = CAETLA_ERROR_OK;


	ByteOut(m_Port, data);

	if (m_RealTimeOut) {
		uint32   oldTick;
		oldTick = GetTickCount();
		while (Handshake(m_Port)) {
			if (GetTickCount() > oldTick + m_TimeOut) {
				timer = m_TimeOut;
				m_ErrorCode = CAETLA_ERROR_TIMEOUT;
				RawPortOut(m_Port, 0);
				RawPortOut(m_Port + 2, 0);
				return 0;
			}
		}
	} else {
		while (Handshake(m_Port)) {
			if (timer && --timer == 0) {
				timer = m_TimeOut;
				m_ErrorCode = CAETLA_ERROR_TIMEOUT;
				RawPortOut(m_Port, 0);
				RawPortOut(m_Port + 2, 0);
				return 0;
			}
		}
	}

	return ByteIn(m_Port);

}


////////////////////////////////////////////////////////////////////////////////
// Send Byte
// NO input retrieval for X-Plorer

uint8 CCaetla::Send8(uint8 data) {
	uint32 timer = m_TimeOut;

	m_ErrorCode = CAETLA_ERROR_OK;


	ByteOut(m_Port, data);

	if (m_RealTimeOut) {
		uint32   oldTick;
		oldTick = GetTickCount();
		while (Handshake(m_Port)) {
			if (GetTickCount() > oldTick + m_TimeOut) {
				timer = m_TimeOut;
				m_ErrorCode = CAETLA_ERROR_TIMEOUT;
				RawPortOut(m_Port, 0);
				RawPortOut(m_Port + 2, 0);
				return 0;
			}
		}
	} else {
		while (Handshake(m_Port)) {
			if (timer && --timer == 0) {
				timer = m_TimeOut;
				m_ErrorCode = CAETLA_ERROR_TIMEOUT;
				RawPortOut(m_Port, 0);
				RawPortOut(m_Port + 2, 0);
				return 0;
			}
		}
	}

//		return ByteIn(m_Port);
	switch (m_CartType) {
	case CART_XP:
		return(0);
		break;

	default:
	case CART_PAR:
		return ByteIn(m_Port);
		break;
	}

}

////////////////////////////////////////////////////////////////////////////////
// Exchange half-word (16-bits)

uint16 CCaetla::Send16(uint16 data) {
	return ((Send8((data >> 8) & 0xff) << 8) | (Send8(data & 0xff)));
}

////////////////////////////////////////////////////////////////////////////////
// Exchange word (32-bits)

uint32 CCaetla::Send32(uint32 data) {
	return ((Send16((uint16)(data >> 16) & 0xffff) << 16) | (Send16((uint16)data & 0xffff)));
}


////////////////////////////////////////////////////////////////////////////////
// Fetch byte from port

uint8 CCaetla::Get8(void) {
	switch (m_CartType) {
	case CART_XP:
		return ByteIn(m_Port);
		break;

	default:
	case CART_PAR:
		return(Swap8(0));
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Fetch byte from port, with NO prior send (primarily for exported low-level
// functionality such as required by CatFlap DLL)

uint8 CCaetla::Get8s(void) {
	return ByteIn(m_Port);
}

////////////////////////////////////////////////////////////////////////////////
// Fetch half-word (16-bits)

uint16 CCaetla::Receive16(void) {
	switch (m_CartType) {
	case CART_XP: {
		uint8 b1, b2;
		b1 = ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  return(0);
		b2 = ByteIn(m_Port);
		return ((b1 << 8) | (b2));
	}
	break;

	default:
	case CART_PAR:
		return ((Swap8(1) << 8) | (Swap8(2)));
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// Fetch word (32-bits)

uint32 CCaetla::Receive32(void) {
//	return ( (Swap8(1)<<24) | (Swap8(2)<<16) | (Swap8(3)<<8) | (Swap8(4)) );
//	return ( (Swap8(0)<<24) | (Swap8(0)<<16) | (Swap8(0)<<8) | (Swap8(0)) );
	switch (m_CartType) {
	case CART_XP: {
		uint8 b1, b2, b3, b4;
		b1 = ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  return(0);
		b2 = ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  return(0);
		b3 = ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  return(0);
		b4 = ByteIn(m_Port);
		return ((b1 << 24) | (b2 << 16) | (b3 << 8) | (b4));
	}
	break;

	default:
	case CART_PAR:
		return ((Swap8(0) << 24) | (Swap8(0) << 16) | (Swap8(0) << 8) | (Swap8(0)));
		break;
	}

}


////////////////////////////////////////////////////////////////////////////////
// Request Caetla to enter PC-Control mode

int CCaetla::Listen(int mode) {
//NEW simpler version works best on XP?

	uint8   r;

retry:
	IssueCommand(CAETLA_REQUESTPCCONTROL);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	r = Swap8(mode);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	if ((r == 0xff) || (r == 0xfe)) {
		complete(CAETLA_ERROR_PROTOCOL);
//		goto retry;
	}

	if (r == 0x01) {
//		printf("retry\n");
		goto retry;
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Get current execution status

int CCaetla::CheckExeStatus(void) {
	uint8   r;

	IssueCommand(CAETLA_EXESTATUS);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	if (m_CartType == CART_XP)
		r = ByteIn(m_Port);
	else
		r = Swap8(0);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	Resume();
	return(r);
}


////////////////////////////////////////////////////////////////////////////////
// Get size of a file

int CCaetla::FileSize(char *name) {
	FILE *file;
	uint32   size;

	if (!(file = fopen(name, "rb"))) {
		m_ErrorCode = CAETLA_ERROR_FILENOTEXIST;
		return(0);
	} else {
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);
		fclose(file);
		return(size);
	}
}

////////////////////////////////////////////////////////////////////////////////
// Load a file

int CCaetla::FileLoad(char *name) {
	FILE *file;

	if (!(file = fopen(name, "rb"))) {
		complete(CAETLA_ERROR_FILENOTEXIST);
	} else {
		if (m_FilePtr) free(m_FilePtr);
		fseek(file, 0, SEEK_END);
		m_FileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		if ((m_FilePtr = malloc(m_FileSize)) == 0) {
			fclose(file);
			complete(CAETLA_ERROR_LOWRAM);
		}
		if ((int)fread(m_FilePtr, 1, m_FileSize, file) != m_FileSize) {
			fclose(file);
			complete(CAETLA_ERROR_FILENOTEXIST);
		}
		fclose(file);
		complete(CAETLA_ERROR_OK);
	}
}


////////////////////////////////////////////////////////////////////////////////
// Run an EXE file (from filename)

int CCaetla::RunExe(char *exename, bool run, bool hooksOn) {
	if (FileLoad(exename) != CAETLA_ERROR_OK) {
		return(m_ErrorCode);
	}

	SendExe((void *)m_FilePtr);

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	m_ExeHooksOn = hooksOn;

	if (run) {
		Execute(&m_ExeHeader);
		m_ExePending = false;
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Run an EXE file (from memory)

int CCaetla::RunExe(void *exe, bool run, bool hooksOn) {
	SendExe(exe);

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	m_ExeHooksOn = hooksOn;

	if (run) {
		Execute(&m_ExeHeader);
		m_ExePending = false;
	}

	return(m_ErrorCode);
}


int CCaetla::SendExe(void *pdata) {
	PSX_EXE_HEADER  *exe_header;

	uint32   addr, length;

// First detect if it's a CPE rather than an EXE - must tidy this up at some point

	{
		uint8 *cpe = (uint8 *)pdata;

#if 0   //DISABLE
		uint32   count;
		short           percent, lastpercent;
#endif  //DISABLE


		if ((cpe[0] == 'C') && (cpe[1] == 'P') && (cpe[2] == 'E') && (cpe[3] == 1)) {

			uint8   *pCPE;
			uint32   minCPE, maxCPE;

			PSX_EXE_HEADER  cpe_exe_header;

			int snr = 0;
			int cpedone = 0;

			pCPE = (uint8 *)malloc(1024 * 1024 * 2);
			if (!pCPE) {
				complete(CAETLA_ERROR_LOWRAM);
			}

			cpe += 4;

			minCPE = 0x8fffffff;
			maxCPE = 0x00000000;

			memset((void *)&cpe_exe_header, 0, sizeof(PSX_EXE_HEADER));
			cpe_exe_header.d_addr = 0x801fff00;

			Listen(CAETLA_MODE_MAIN);

			if (m_ErrorCode != CAETLA_ERROR_OK) {
				free(pCPE);
				complete(m_ErrorCode);
			}

#if 0   //DISABLE
			InitProgressBar();
			percent = lastpercent = 0;
#endif  //DISABLE

			while (!cpedone) {
				switch (*cpe) {

				case PSX_CPE_CHUNK: {
					cpe++;
					addr = (cpe[3] << 24) | (cpe[2] << 16) | (cpe[1] << 8) | (cpe[0]);
					cpe += 4;
					length = (cpe[3] << 24) | (cpe[2] << 16) | (cpe[1] << 8) | (cpe[0]);
					cpe += 4;
//					Upload((void *)cpe,(uint32)addr,length,true,false);
//					Upload((void *)cpe,(uint32)addr,length,false,false);

					memcpy(pCPE + (addr&~0x80000000), cpe, length);
					if (addr < minCPE)           minCPE = addr;
					if (addr + length > maxCPE)    maxCPE = addr + length;

					cpe += length;

//					if   (m_ErrorCode!=CAETLA_ERROR_OK)  { free(pCPE); complete(m_ErrorCode); }

				}
				break;

				case PSX_CPE_REGISTER: {
					uint16 reg;
					cpe++;
					reg = (cpe[1] << 8) | (cpe[0]);
					cpe += 2;
					addr = (cpe[3] << 24) | (cpe[2] << 16) | (cpe[1] << 8) | (cpe[0]);
					cpe += 4;
					cpe_exe_header.pc0 = addr;
				}
				break;

				case PSX_CPE_TARGET: {
					cpe += 2;
				}
				break;

				default: {
					Dump("> Unknown Chunk in CPE file: 0x%02x\n", cpe[0]);
					cpe++;
				}
				break;

				case PSX_CPE_END: {
					cpedone = 1;
				}
				break;

				}

#if 0   //DISABLE
				count = (cpe - (uint8 *)pdata);

				percent = (short)((count * 50) / m_FileSize);
				if (percent > lastpercent) {
					UpdateProgressBar(percent, lastpercent);
					lastpercent = percent;
				}
#endif  //DISABLE

			}

#if 0   //DISABLE
			EndProgressBar();
#endif  //DISABLE

//		Execute(&cpe_exe_header);
			memcpy(&m_ExeHeader, &cpe_exe_header, sizeof(PSX_EXE_HEADER));
			m_ExePending = true;

			Dump("\n>>> CPE: 0x%x  MinCPE: 0x%x  MaxCPE: 0x%x [%d]\n\n", pCPE, minCPE, maxCPE, maxCPE - minCPE);

			Upload(pCPE + (minCPE&~0x80000000), minCPE | 0x80000000, maxCPE - minCPE, true, false);

			free(pCPE);

			complete(CAETLA_ERROR_OK);

		}

	}


	exe_header = (PSX_EXE_HEADER *)pdata;

	if (memcmp(exe_header->id, "PS-X EXE", 8)) {
		Dump("Error: Not a PSX executable file\n");
		complete(CAETLA_ERROR_FILEFORMAT);
	}

	addr = exe_header->t_addr;

	Listen(CAETLA_MODE_MAIN);

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}


// Check if the filesize & the t_size in the EXE header are wildly contradictory
// If so, use the filesize to send the EXE, otherwise use the header details

#if 0   //DISABLE
	if (exe_header->t_size != (m_FileSize - 0x800)) {
// Using m_FileSize
		Upload((void *)((uint32)pdata + 2048), (uint32)addr, m_FileSize - 2048, true, true);
	} else {
// Using header
		Upload((void *)((uint32)pdata + 2048), (uint32)addr, exe_header->t_size, true, true);
	}
#else

// OK, is the EXE pointer the same as our Class's loaded file?
	if (pdata == m_FilePtr) {
// Yes, so assume we're trying to execute a file loaded by the Class, and
// check if the actual size is way different from that in the header
		if (exe_header->t_size != (m_FileSize - 0x800)) {
// Using m_FileSize
			Upload(((uint8 *)pdata + 2048), (uint32)addr, m_FileSize - 2048, true, true);
		} else {
// Using header
			Upload(((uint8 *)pdata + 2048), (uint32)addr, exe_header->t_size, true, true);
		}
	} else {
// Using header
		Upload(((uint8 *)pdata + 2048), (uint32)addr, exe_header->t_size, true, true);
	}

#endif  //DISABLE

// Skip verify for the moment

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

//	Execute(exe_header);
	memcpy(&m_ExeHeader, exe_header, sizeof(PSX_EXE_HEADER));
	m_ExePending = true;

	complete(CAETLA_ERROR_OK);
}


////////////////////////////////////////////////////////////////////////////////
// Execute EXE

int CCaetla::Execute(PSX_EXE_HEADER *exe_header) {
	uint32     i;
	uint32    AR_TCB = 4;
	uint32    AR_EVENT = 16;
	uint32    AR_SP = 0x801ffff0;

//Say("0");
	IssueCommand(CAETLA_EXECUTE);
//Say("1");
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//Say("2");
	Send32(AR_TCB);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//Say("3");
	Send32(AR_EVENT);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//Say("4");
	Send32(exe_header->sp_addr);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	for (i = 0; i < 15; i++) {
		Send32(*((uint32 *)exe_header + (0x10 / 4) + i));
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
//Say("5");

//	Swap8(0);

	if (m_ExeHooksOn) {
//		Say("<<HOOKS ENABLED>>");
		Send8(0);
	} else {
//		Say("<<HOOKS DISABLED>>");
		Send8(1);
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Send a File

int CCaetla::SendFile(char *filename, uint32 addr) {
	if (FileLoad(filename) != CAETLA_ERROR_OK)
		return((int)m_ErrorCode);

//	Upload((void *)m_FilePtr,addr,m_FileSize,false,true);
	Upload((void *)m_FilePtr, addr, m_FileSize, true, true);

	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Receive a File

int CCaetla::ReceiveFile(char *fname, uint32 addr, uint32 len) {
	FILE    *fp;
	void    *data;

	if (!(fp = fopen(fname, "wb"))) {
		complete(CAETLA_ERROR_FILENOTEXIST);
	}

	if ((data = malloc(len)) == 0) {
		fclose(fp);
		complete(CAETLA_ERROR_LOWRAM);
	}

//	Download(data,addr,len,false,false);
	Download(data, addr, len, true, false);

	if (m_ErrorCode != CAETLA_ERROR_OK) {
		free(data);
		fclose(fp);
		return(m_ErrorCode);
	}

	fwrite(data, len, 1, fp);
	fclose(fp);
	free(data);

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Upload data to PSX

int CCaetla::Upload(void *pdata, uint32 addr, uint32 length, bool showprogress, bool verbose) {
	uint8 ck;

//temp
	{
		int mode;
//printf("[1]\n");
		ChooseMainOrDebug();
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
//printf("[2]\n");
		mode = QueryMode();
//printf("[3]\n");
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		if (mode == CAETLA_MODE_DEBUG) {
			return(UploadDbg(pdata, addr, length, showprogress, verbose));
		}
	}
//temp

	IssueCommand(CAETLA_UPLOAD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	Send32(addr);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(length);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	SendData(pdata, length, showprogress);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP:
		ck = ByteIn(m_Port);
		break;

	default:
	case CART_PAR:
		ck = Swap8(0);
		break;
	}

	if (verbose)
		Dump("Sent 0x%x (%d) bytes to 0x%x\n", length, length, addr);

	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Download data from PSX

int CCaetla::Download(void *pdata, uint32 addr, uint32 length, bool showprogress, bool verbose) {
	uint8 ck;

//temp
	{
		int mode;
		ChooseMainOrDebug();
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		mode = QueryMode();
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		if (mode == CAETLA_MODE_DEBUG) {
			return(DownloadDbg(pdata, addr, length, showprogress, verbose));
		}
	}
//temp

	IssueCommand(CAETLA_DOWNLOAD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	Send32(addr);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(length);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	ReceiveData(pdata, length, showprogress);
//	ReceiveData(pdata,length,1);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP:
		ck = ByteIn(m_Port);
		break;

	default:
	case CART_PAR:
		ck = Swap8(0);
		break;
	}

	if (verbose)
		Dump("Received 0x%x (%d) bytes from 0x%x\n", length, length, addr);

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Reset the PSX

int CCaetla::Reset(void) {
	IssueCommand(CAETLA_RESET);
	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Send data to PSX

int CCaetla::SendData(void *pdata, uint32 length, bool showprogress) {
	uint8 *data = (uint8 *)pdata;
	uint32 count;
	sint16 percent, lastpercent;

	if (showprogress) {
		InitProgressBar();
		percent = lastpercent = 0;

		count = 0;
		while ((count < length) && (m_ErrorCode == CAETLA_ERROR_OK)) {
			Send8(data[count]);

			percent = (sint16)((count * 50) / length);
			if (percent > lastpercent) {
				UpdateProgressBar(percent, lastpercent);
				lastpercent = percent;
			}
			count++;
		}
		percent = (sint16)((count * 50) / length);
		if (percent > lastpercent) {
			UpdateProgressBar(percent, lastpercent);
			lastpercent = percent;
		}
		EndProgressBar();
	} else {
		count = 0;
		while ((count < length) && (m_ErrorCode == CAETLA_ERROR_OK))
			Send8(data[count++]);
	}

	return(m_ErrorCode);

}

////////////////////////////////////////////////////////////////////////////////
// Get data from PSX

int CCaetla::ReceiveData(void *pdata, uint32 length, bool showprogress) {
	uint8 *data = (uint8 *)pdata;
	uint32 count;

	sint16   percent, lastpercent;

	if (showprogress) {
		InitProgressBar();
		percent = lastpercent = 0;

		count = 0;
		while ((count < length) && (m_ErrorCode == CAETLA_ERROR_OK)) {
			switch (m_CartType) {
			case CART_XP:
				*(data++) = ByteIn(m_Port);
				break;

			default:
			case CART_PAR:
				*(data++) = Swap8(0);
				break;
			}

			percent = (sint16)((count * 50) / length);
			if (percent > lastpercent) {
				UpdateProgressBar(percent, lastpercent);
				lastpercent = percent;
			}
			count++;
		}
		percent = (sint16)((count * 50) / length);
		if (percent > lastpercent) {
			UpdateProgressBar(percent, lastpercent);
			lastpercent = percent;
		}
		EndProgressBar();
	} else {
		count = 0;
		while ((count < length) && (m_ErrorCode == CAETLA_ERROR_OK)) {
			switch (m_CartType) {
			case CART_XP:
				*(data++) = ByteIn(m_Port);
				break;

			default:
			case CART_PAR:
				*(data++) = Swap8(0);
				break;
			}
//ASH - this was added on 27/9/00 - REALLY STUPID, should have been here for ages
			count++;
		}
	}

	return(m_ErrorCode);

//	Dump("\rReceived Data OK\n");
}

////////////////////////////////////////////////////////////////////////////////
// Get a string from PSX

int CCaetla::ReceiveString(char *x) {
	uint8 c;

	do  {
		switch (m_CartType) {
		case CART_XP:
			c = ByteIn(m_Port);
			break;

		default:
		case CART_PAR:
			c = Swap8(0);
			break;
		}

		*x++ = c;
	} while ((c) && (m_ErrorCode == CAETLA_ERROR_OK));

	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Send a string to PSX

int CCaetla::SendString(char *x) {
	while ((*x) && (m_ErrorCode == CAETLA_ERROR_OK)) {
		Send8(*x++);
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Update a byte on PSX (in Debug mode)

int CCaetla::UpdateByte(uint32 addr, uint8 data) {
	IssueCommand(CAETLA_DEBUG_UPDATEBYTE);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	Send32(addr);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send8(data);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Get Registers (Debug Mode)

int CCaetla::GetRegisters(void *raddr) {
	uint32 *addr = (uint32 *)raddr;

	if (!addr)
		complete(CAETLA_ERROR_UNKNOWN);

	IssueCommand(CAETLA_DEBUG_GETREGISTERS);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	for (int r = 0; r < 36; r++) {
//		uint32 reg=Receive32();
//		*addr++=reg;
		uint8 b1, b2, b3, b4;
		b1 = Swap8(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		b2 = Swap8(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		b3 = Swap8(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		b4 = Swap8(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		*addr++ = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4) ;
	}

	complete(CAETLA_ERROR_OK);
}


////////////////////////////////////////////////////////////////////////////////
// Download from PS (Debug Mode)

int CCaetla::DownloadDbg(void *pdata, uint32 addr, uint32 length, bool showprogress, bool verbose) {
	uint8 ck;

	IssueCommand(CAETLA_DEBUG_DOWNLOAD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	if (m_CartType == CART_XP) {
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	} else {
		Swap8(1);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Swap8(2);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Swap8(3);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Swap8(4);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}

//printf("@1\n");
	Send32(addr);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(length);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("@2\n");

	ReceiveData(pdata, length, showprogress);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("@3\n");

	if (m_CartType == CART_XP)
		ck = ByteIn(m_Port);
	else
		ck = Swap8(0);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("@4\n");

	Send32(addr);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(0);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("@5\n");

	if (m_CartType == CART_XP)
		ck = ByteIn(m_Port);
	else
		ck = Swap8('O');
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("@6\n");
	if (m_CartType == CART_XP)
		ck = ByteIn(m_Port);
	else
		ck = Swap8('K');
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("@7\n");
	complete(CAETLA_ERROR_OK);
}


////////////////////////////////////////////////////////////////////////////////
// Upload to PS (Debug Mode)

int CCaetla::UploadDbg(void *pdata, uint32 addr, uint32 length, bool showprogress, bool verbose) {
//printf("#1\n");
	IssueCommand(CAETLA_DEBUG_UPLOAD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("#2\n");

	Send32(addr);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("#3\n");
	Send32(length);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("#4\n");

	Send8(0);
//printf("#5\n");

	SendData(pdata, length, showprogress);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("#6\n");

	complete(CAETLA_ERROR_OK);
}


////////////////////////////////////////////////////////////////////////////////
// Set a Register (Debug Mode)

int CCaetla::SetRegister(int reg, uint32 data) {
	IssueCommand(CAETLA_DEBUG_SETREGISTER);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(reg);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(data);
	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Get CP Condition (Debug Mode)

int CCaetla::GetCpCond(void) {
	uint8 cp;

	IssueCommand(CAETLA_DEBUG_GETCPCOND);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	cp = Get8();
	return(cp);
}

////////////////////////////////////////////////////////////////////////////////
// Flush the I-Cache (Debug Mode)

int CCaetla::FlushICache(void) {
	IssueCommand(CAETLA_DEBUG_FLUSHICACHE);
	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Set Hardware BreakPoint (Debug Mode)

int CCaetla::SetHBP(uint32 dat1, uint32 dat2, uint32 dat3, uint32 dat4) {
	IssueCommand(CAETLA_DEBUG_SETHBP);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(dat1);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(dat2);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(dat3);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send32(dat4);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	return(m_ErrorCode);

#if 0   //DISABLE
	-Procedure

	(1) OUT(4B): data to cop0 register $03, $05, $06
		(2) OUT(4B): data to cop0 register $07
		(3) OUT(4B): data to cop0 register $09
		(4) OUT(4B): data to cop0 register $0B

		- Function

		Configure H.B.P. by using cop0.
		Same as $0C of PS - PAR protocol.

		(1) Break point address
		(2) Watching condtion
		No            :
		E0800000            On Write      :
		EA800000
		On Execute    :
		E1800000            Write + Execute :
		EB800000
		(3) Mask of data address
		(4) Mask of executiong address
#endif  //DISABLE

	}

////////////////////////////////////////////////////////////////////////////////
// Disable Hardware BreakPoint (Debug Mode)

int CCaetla::DisableHBP(void) {
	IssueCommand(CAETLA_DEBUG_DISABLEHBP);
	return(m_ErrorCode);
}




////////////////////////////////////////////////////////////////////////////////
// Send Caetla a command

int CCaetla::IssueCommand(uint8 command) {
	uint8 r = 0;

#if 0   //DISABLE
//	if   ((r=Swap8('C'))!='d')   complete(CAETLA_ERROR_PROTOCOL);
	rep = 16;
	while (((r = Swap8('C')) != 'd') && (rep--)) {
//		Say("Rep=%d\n",rep);
//		printf("Rep %d\n",rep);
	}
#endif  //DISABLE

	while ((r != 'd') && (m_ErrorCode == CAETLA_ERROR_OK)) {
		r = Swap8('C');
	}

	if (m_ErrorCode != CAETLA_ERROR_OK)  complete(m_ErrorCode);

	if (r != 'd')    complete(CAETLA_ERROR_PROTOCOL);

	r = Swap8('L');
	if (m_ErrorCode != CAETLA_ERROR_OK)  return(m_ErrorCode);
	if (r != 'o') {
		complete(CAETLA_ERROR_PROTOCOL);
	}

//	r=Swap8(command);
	r = Send8(command);
	if (r) {
		complete(CAETLA_ERROR_PROTOCOL);
	}

#if ( USING_XP && DEBUG_XP )
	Dump("Command %d issued ok\n", command);
#endif
	complete(CAETLA_ERROR_OK);
}


////////////////////////////////////////////////////////////////////////////////
// The DOS console

int CCaetla::DosConsole(void) {
	InitWinConsole();
	while ((!m_ForceQuitConsole) && (!m_QuitConsole))
//	do
	{
		WinConsole();
	}
//	while((!m_ForceQuitConsole)&&(!m_QuitConsole));
	EndWinConsole();

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Initialise the console system

int CCaetla::InitWinConsole(void) {
	IssueCommand(CAETLA_NOP);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	IssueCommand(CAETLA_CONSOLEMODE);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	Send8(1);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

//	Dump("\n<<< Entering Caetla Console >>>\n\n%s",CAETLA_DOSCON_PROMPT);
	Dump("<<< Entering Caetla Console >>>\n%s", CAETLA_DOSCON_PROMPT);

	m_InConsole = true;
	m_QuitConsole = false;
	m_ForceQuitConsole = false;

	if (m_ExePending) {
		Execute(&m_ExeHeader);
		m_ExePending = false;
	}

	m_DirectKey = 0;

	m_ConsoleLastValidTime = GetTickCount();
	m_LastXpFlagged = 0;
	m_XpFlagged = 0;

	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Single iteration of the console system

int CCaetla::WinConsole(void) {
	static int              toggle = 0;
//	static int               XpFlagged=0;
	static uint8    ina, inb; //,inc,tmpa,tmpb,tmpc;
	uint8           x;
	uint32                     timer;

// Initially, assume that the link is still up
	m_ConsoleLinkStatus = true;

// Special case for server-mode:
// Instead of using the regular Swap8 command in the first instance during
// the constant polling, access the comms link directly with a much smaller
// timeout. This ensures that during Caetla server inactivity, the PC keyboard
// polling can occur much more frequently and give us better feedback.

	if ((m_QuitConsole) || (m_ForceQuitConsole))
		return(m_ErrorCode);

// This isn't really used any more, was for dodgy software-wait loops
	timer = 1000;

// First off, do the magic thing and monitor the fileserver hook for incoming
// commands.
// For X-Plorer carts this is a little complicated and involves a few different
// non-blocking stages, effectively because the handshaking and byte transfer is
// a little more involved.

	x = 0;

	switch (m_CartType) {
	case CART_XP: {
		m_LastXpFlagged = m_XpFlagged;
		if (toggle) {
			switch (m_XpFlagged) {
			case 0:
				RawPortOut(m_Port, 'P');
				RawPortOut(m_Port + 2, RawPortIn(m_Port + 2) & 0xf7);
				m_XpFlagged++;
				break;

			case 1:
				ina = RawPortIn(m_Port + 1);
				inb = m_XPLookups[ina];
				if ((inb & 0x08) != 0) {
					ByteOut(m_Port, 'P');
					m_XpFlagged++;
				}
				break;

			case 2:
				ina = RawPortIn(m_Port + 1);
				inb = m_XPLookups[ina];
				if ((inb & 0x08) != 0) {
					x = ByteIn(m_Port);
					m_XpFlagged = 0;
				}
				break;
			}

			toggle = 0;
		} else {
			toggle = 1;
			x = 0;
		}
	}
	break;

	default:
	case CART_PAR: {
#if 1   //DISABLE
		uint32 startTime, curTime;

		ByteOut(m_Port, 'P');

		startTime = GetTickCount();

		do  {
			curTime = GetTickCount();
		} while ((RawPortIn(m_Port + 2) & 0x01) && (curTime < startTime + 1));

		if (curTime > startTime + 1) {
			x = 0;
		} else {
			x = ByteIn(m_Port);
		}
#else
		while ((RawPortIn(m_Port + 2) & 0x01) && (timer--));
		if (timer)
			x = ByteIn(m_Port);
		else
			x = 0;
#endif  //DISABLE
	}
	break;
	}

// OK, so now that the magic is all done, we have our first feedback from Caetla.
// As specified in the protocol, we should get a 'd' back from our 'P' sent.

	if (x == 'd') {
		if (Swap8('R') == 'o') {
			switch (m_CartType) {
			case CART_XP:
				x = ByteIn(m_Port);
				break;

			default:
			case CART_PAR:
				x = Swap8(0);
				break;
			}

// We've got the server command now, go and do whatever we have to do with it

			if (m_ErrorCode == CAETLA_ERROR_OK)
				ServerCommand(x);

		}
	} else {
		sint16 bKey;     // Our keystroke code

// OK, we're not on a roll with incoming commands from Caetla, so this is idle time.
// Might as well do some fancy stuff then.

		bKey = 0;

// Hookback call to receive user-input keystrokes. The hookback is welcome to return
// any 16-bit value it wants, so long as it is non-zero it will be stored to the
// keycode address on PSX.

		if (m_ConsoleKeyHook) {
			bKey = m_ConsoleKeyHook();
		}

// Direct-Access KeyCode which can be 'POKE'd to the class without the callback

		if (m_DirectKey)
			bKey = m_DirectKey;

		if (bKey) {

// Another Hookback, this time allowing some pre-processing with the keystrokes.
// Now that the console system has been opened up this is probably of more importance
// to the enclosed ::DosConsole() function which is blocking within the class from
// start to finish.

			m_DirectKey = 0; // Reset Direct Access KeyCode

			if (m_ConsoleHook) {
				m_ConsoleHook((uint8)bKey, m_FpLog);
			}

// OK, we got a valid keystroke. Let's do it.
// Now, this part of the console system has become smarter and more aware of what's
// going on around it. So far it's proving reliable...

			if (m_KeyIn) {

#if 0   //DISABLE
// First off, check if the console mode is running and quit if it isn't.
// THIS IS APPARENTLY NOT 100% RELIABLE YET

				int iSt = CheckExeStatus();

				if (m_ErrorCode == CAETLA_ERROR_OK) {
					if (!(iSt & 0x04)) {
						m_XpFlagged = 0;
						m_ForceQuitConsole = true;
						return(m_ErrorCode);
					}
				}
#endif  //DISABLE

// Right, check that the comms link is still fine, get Caetla to listen to us in
// Debug Mode, then send the bytes to where we want.
// CatFlap originally used only an 8-bit keystroke value, but sending 2 bytes isn't
// that big a deal, also doing it as 2 bytes means we can still use odd addresses.

				for (int retry = 50; retry; retry--) {
					if (Detect() == CAETLA_ERROR_OK) {
						Listen(CAETLA_MODE_DEBUG);

						UpdateByte(m_KeyIn, (uint8)bKey);

						if (m_ErrorCode != CAETLA_ERROR_OK) {
							m_ConsoleLinkStatus = false;
							m_QuitConsole = true;
							Dump("\n<<< LOST COMMS LINK >>>\n");
							break;
						}

						UpdateByte(m_KeyIn + 1, (uint8)(bKey >> 8));
						Resume();

						if (m_ErrorCode != CAETLA_ERROR_OK) {
							m_ConsoleLinkStatus = false;
							m_QuitConsole = true;
							Dump("\n<<< LOST COMMS LINK >>>\n");
						}
						break;
					} else if (retry == 1) {
						m_ConsoleLinkStatus = false;
						m_QuitConsole = true;
						Dump("\n<<< LOST COMMS LINK >>>\n");
					}
				}

// Always reset the XPlorer state
				m_XpFlagged = 0;
			}

// Use 0x1b (ESCAPE) as quit value. Unsure if we should keep using this seeing as
// we can now let Hookbacks give us any 16-bit value.

			if (bKey == 0x1b)
				m_QuitConsole = true;
		}

	}


// OK, this is another 'intelligent' bit. Every once in a while do a quick check
// on the commslink just to see if we can still communicate OK.

	if (m_AutoSense) {
		if (m_XpFlagged != m_LastXpFlagged) {
			m_ConsoleLastValidTime = GetTickCount();
		}



// Only if we haven't asked to quit already

		if ((!m_QuitConsole) && (!m_ForceQuitConsole)) {
			uint32 curTime;

// Get current timestamp and see how long has elapsed since the last valid stamp
// we had in the console. If it's over the limit then do the status check.

			curTime = GetTickCount();
			if (curTime > m_ConsoleLastValidTime + 5000)
//		if  ( (curTime>m_ConsoleLastValidTime+1000) &&
//			(!((m_CartType==CART_XP)&&(m_XpFlagged!=0))) )
			{

#if 0   //DISABLE
// First off, check if the console mode is running and quit if it isn't.
// THIS IS APPARENTLY NOT 100% RELIABLE YET
				int iSt = CheckExeStatus();

				if (m_ErrorCode == CAETLA_ERROR_OK) {
					if (!(iSt & 0x04)) {
						m_XpFlagged = 0;
						m_ForceQuitConsole = true;
						return(m_ErrorCode);
					}
				}
#endif  //DISABLE

// Detect the commslink, and either validate this timestamp if it's OK or ask the
// console to quit.

				for (int retry = 50; retry; retry--) {
					if (Detect() == CAETLA_ERROR_OK) {
						m_ConsoleLastValidTime = GetTickCount();
						usleep(10);
						break;
					} else if (retry == 1) {
						m_ConsoleLinkStatus = false;
						m_QuitConsole = true;
						Dump("\n<<< LOST COMMS LINK >>>\n");
					}
				}

				m_XpFlagged = 0;

			}
		}

	}   //m_AutoSense


	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Terminate the console system

int CCaetla::EndWinConsole(void) {
	if (m_ConsoleLinkStatus) {
		IssueCommand(CAETLA_CONSOLEMODE);
		Send8(0);
	}

	Dump("\n<<< Caetla Console Mode Ended >>>\n");

	m_InConsole = false;

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Process a server command from the console

int CCaetla::ServerCommand(uint8 cmd) {
	static uint8 ServerBuff[1024+4];
	static uint8 OutputBuff[256];
	static uint16 outring = 0, commanding = 0;
	static uint8 CommandBuff[256];

	switch (cmd) {
	case 1: { //putchar
		uint8 letter;

#if DEBUG_XP
		Say("PutChar\n");
#endif

		switch (m_CartType) {
		case CART_XP:
			letter = ByteIn(m_Port);
			break;

		default:
		case CART_PAR:
			letter = Swap8(0);
			break;
		}

		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

		OutputBuff[outring] = letter;
		outring = (outring + 1) & 0xff;

//			putchar(letter);
		Dump("%c", letter);
		if (letter == 0x0a)
			Dump("%s", CAETLA_DOSCON_PROMPT);
		fflush(stdout);

		if (!commanding) {
			if ((OutputBuff[(outring-4)&0xff] == 'C') &&
			        (OutputBuff[(outring-3)&0xff] == 'M') &&
			        (OutputBuff[(outring-2)&0xff] == 'D') &&
			        (OutputBuff[(outring-1)&0xff] == ':')) {
				commanding = 1;
			}
		} else {
			CommandBuff[commanding++-1] = letter;
			if ((letter == 0x0a) || (letter == 0) || (commanding >= 255)) {
				CommandBuff[commanding] = 0;
				commanding = 0;

				if (strncasecmp((char *)CommandBuff, "quit", 4) == 0)
					m_ForceQuitConsole = true;
//						goto    quit;

				if (system((char *)CommandBuff)) {
					Dump("CMD Error: %s\n%s", CommandBuff, CAETLA_DOSCON_PROMPT);
				}

// Force PSX to accept a CR
				if (m_KeyIn) {
					Listen(CAETLA_MODE_DEBUG);
					if (m_ErrorCode != CAETLA_ERROR_OK)  {
						complete(m_ErrorCode);
					}
					UpdateByte(m_KeyIn, 0x0a);
					if (m_ErrorCode != CAETLA_ERROR_OK)  {
						complete(m_ErrorCode);
					}
					Resume();
					if (m_ErrorCode != CAETLA_ERROR_OK)  {
						complete(m_ErrorCode);
					}
				}

			}
		}
	}
	break;

	case 2: { //getchar
#if DEBUG_XP
		Say("GetChar\n");
#endif  //DISABLE
		Swap8(getchar());
	}
	break;

	case 3: { //open
		uint8   mode;
		int rc;

#if DEBUG_XP
		Say("Open\n");
#endif  //DISABLE

		ReceiveString((char *)&ServerBuff);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

		switch (m_CartType) {
		case CART_XP:
			mode = ByteIn(m_Port);
			break;

		default:
		case CART_PAR:
			mode = Swap8(0);
			break;
		}

		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

		rc = open((char *)ServerBuff, O_RDWR);
// Try just read-only if unable to open as read/write
		if (rc < 0)
			rc = open((char *)ServerBuff, O_RDONLY);
#if (SHOW_CON_DBG)
		Dump("[Request to open file ""%s"" : RC==%x]\n", &ServerBuff[0]);
#endif
		Send8(rc < 0 ? 0xff : 0x00);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Send16(rc);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	case 4: { //create
		uint8 mode;
		int rc;

#if DEBUG_XP
		Say("Create\n");
#endif  //DISABLE

		ReceiveString((char *)&ServerBuff);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		rc = creat((char *)&ServerBuff, S_IRWXU);
#if (SHOW_CON_DBG)
		Dump("[Request to create file ""%s"" : RC==%x]\n", &ServerBuff[0]);
#endif
		switch (m_CartType) {
		case CART_XP: {
			mode = ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			mode = ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			mode = Swap8(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			mode = Swap8(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;
		}

		rc = open((char *)ServerBuff, O_RDWR);
#if (SHOW_CON_DBG)
		Dump("[Request to open file ""%s"" : RC==%x]\n", &ServerBuff[0]);
#endif
		Send8(rc < 0 ? 0xff : 0x00);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Send16(rc);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

	}
	break;

	case 5: { //read
		int fd, r;
		uint32 size;
		uint8 x = 0;

#if DEBUG_XP
		Say("Read\n");
#endif  //DISABLE

		switch (m_CartType) {
		case CART_XP: {
			fd = ByteIn(m_Port) << 8;
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			fd |= ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			size = (ByteIn(m_Port) << 24) | (ByteIn(m_Port) << 16) | (ByteIn(m_Port) << 8) | ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			fd = Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			size = Send32(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;
		}


#if (SHOW_CON_DBG)
		Dump("[Request to read %d bytes from file 0x%x]\n", size, fd);
#endif

		while (size) {
			r = size > 1024 ? 1024 : size;
			r = read(fd, (char *)ServerBuff, r);
			if (r < 0) {
				x = 0;
				break;
			}

			Send32(r);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}

			SendData(&ServerBuff, r, false);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}

			switch (m_CartType) {
			case CART_XP:
				ByteIn(m_Port);
				break;

			default:
			case CART_PAR:
				Swap8(0);
				break;
			}


			size -= r;
			x = 0;
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}

		Send32(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Send8(x);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

	}
	break;

	case 6: { //write
		int fd, r;
		uint32 size;
		uint8 x = 0;

#if DEBUG_XP
		Say("Write\n");
#endif  //DISABLE

		switch (m_CartType) {
		case CART_XP: {
			fd = ByteIn(m_Port) << 8;
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			fd |= ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			size = (ByteIn(m_Port) << 24) | (ByteIn(m_Port) << 16) | (ByteIn(m_Port) << 8) | ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			fd = Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			size = Send32(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;
		}

#if (SHOW_CON_DBG)
		Dump("[Request to write %d bytes to file 0x%x]\n", size, fd);
#endif

		while (size) {
			r = size > 1024 ? 1024 : size;
			Send32(r);
#if (SHOW_CON_DBG)
			printf("[r=%d]\n", r);

			ShowError();
#endif  //DISABLE
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ReceiveData(&ServerBuff, r, false);
#if (SHOW_CON_DBG)
			printf("[received]\n", r);

			ShowError();
#endif  //DISABLE

//27/9/00 - TIMEOUT here - WHY?! (temp fix)
			m_ErrorCode = CAETLA_ERROR_OK;
//27/9/00 - TIMEOUT here - WHY?! (temp fix)

			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}

#if 0   //DISABLE
			switch (m_CartType) {
			case CART_XP:
				ByteIn(m_Port);
				break;

			default:
			case CART_PAR:
				Swap8(0);
				break;
			}
#endif  //DISABLE

#if (SHOW_CON_DBG)
			printf("[here]\n", r);

			ShowError();
#endif  //DISABLE
			Get8();
#if (SHOW_CON_DBG)
			printf("[here]\n", r);

			ShowError();
#endif  //DISABLE

//27/9/00 - temp fix for TIMEOUT?!              if  (m_ErrorCode!=CAETLA_ERROR_OK)  { complete(m_ErrorCode); }

			if ((fd == -1) || (fd == 0xffff)) {
				ServerBuff[r] = 0;
				Dump("%s", &ServerBuff[0]);
			} else {
				int size = write(fd, (char *)&ServerBuff, r);
#if (SHOW_CON_DBG)
				Dump("[Written %d bytes to file 0x%x]\n", r, fd);
#endif
			}
			size -= r;
			x = 0;
		}
		Send32(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Send8(x);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	case 7: { //seek
		uint32 rc;
		int fd, off, mode;

#if DEBUG_XP
		Say("Seek\n");
#endif  //DISABLE

		switch (m_CartType) {
		case CART_XP: {
			fd = ByteIn(m_Port) << 8;
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			fd |= ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			off = (ByteIn(m_Port) << 24) | (ByteIn(m_Port) << 16) | (ByteIn(m_Port) << 8) | ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			mode = ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			fd = Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			off = Send32(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			mode = Swap8(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;
		}


#if (SHOW_CON_DBG)
		Dump("[Request to seek in file 0x%x]\n", fd);
#endif

		rc = lseek(fd, off, mode);
		Send8(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Send32(rc);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	case 8: { //close
		int fd;

#if DEBUG_XP
		Say("Close\n");
#endif  //DISABLE

		switch (m_CartType) {
		case CART_XP: {
			fd = ByteIn(m_Port) << 8;
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			fd |= ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			fd = Send16(0);
		}
		break;
		}


		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

		close(fd);
		Send8(0);

#if (SHOW_CON_DBG)
		Dump("[Request to close file 0x%x]\n", fd);
#endif

	}
	break;

	case 9: { //delete
		Dump("[Caetla-Server: Delete] Not implemented\n%s", CAETLA_DOSCON_PROMPT);
	}
	break;

	case 10: {  //rename
		Dump("[Caetla-Server: Rename] Not implemented\n%s", CAETLA_DOSCON_PROMPT);
	}
	break;

	case 11: {  //files
		int i;

		Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

		ReceiveString((char *)&ServerBuff);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
//			for (i=0;i<22;i++) Swap8(0);
		for (i = 0; i < 22; i++) {
			Swap8(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		Swap8(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	case 12: {  //nfiles
		int i;

//			for (i=0;i<22;i++) Swap8(0);
		for (i = 0; i < 22; i++) {
			Swap8(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		Swap8(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	case 13: {  //chdir
		ReceiveString((char *)&ServerBuff);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		if (chdir((char *)&ServerBuff) < 0)
			Swap8(0xff);
		else
			Swap8(0x00);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	case 14: {  //curdir
		Dump("[Caetla-Server: CurDir] Not implemented\n%s", CAETLA_DOSCON_PROMPT);
	}
	break;

	case 15: {  //chdrv
		Dump("[Caetla-Server: ChDrv] Not implemented\n%s", CAETLA_DOSCON_PROMPT);
	}
	break;

	case 16: {  //curdrv
		Dump("[Caetla-Server: CurDrv] Not implemented\n%s", CAETLA_DOSCON_PROMPT);
	}
	break;

	case 0:
	default:
		break;
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Raw Console mode. Only works with PAR.

int CCaetla::RawConsole(void) {
	uint8   c;


	if ((m_CartType != CART_PAR)) {
		Dump("Error - RAW Console not supported for this cartridge type\n");
		return(0);
	}


	Dump("\n<<< Entering RAW Console (CTRL-C to exit) >>>\n\n%s", CAETLA_DOSCON_PROMPT);

	if (m_ExePending) {
		Execute(&m_ExeHeader);
		m_ExePending = false;
	}

	while (1) {
		c = Swap8(0);
		if (m_ErrorCode == CAETLA_ERROR_OK)
			Dump("%c", c);
	}


	return(m_ErrorCode);
}




////////////////////////////////////////////////////////////////////////////////
// Install the NT device driver file & registry settings

int CCaetla::InstallNT(void) {
	return(EXIT_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////
// Remove the NT device driver file & registry settings

int CCaetla::UninstallNT(void) {
	return(EXIT_SUCCESS);
}



////////////////////////////////////////////////////////////////////////////////
// Save VRAM to Filename

int CCaetla::ReceiveVRAM(char *fname, int x, int y, int w, int h, int depth) {
	//char ext[_MAX_EXT];
	char ext[1024 * 1024 * 4];

	if (!(m_FILE = fopen(fname, "wb"))) {
		complete(CAETLA_ERROR_FILE);
	}

// Validate region

	if (x < 0)       x = 0;
	if (x > 1023)    x = 1023;
	if (y < 0)       y = 0;
	if (y > 511)     y = 511;
	if (w < 1)       w = 1;
	if (h < 1)       h = 1;
	if (x + w > 1024)  w = 1024 - x;
	if (y + h > 512)   h = 512 - y;


	//_splitpath(fname,NULL,NULL,NULL,ext);

	//if    ((strcasecmp(ext,"bmp")==0)||(_stricmp(ext,".bmp")==0))
	//  ReceiveVRAMBMPtoFile(x,y,w,h,depth);
	//else
	ReceiveVRAMtoFile(x, y, w, h, depth);

	fclose(m_FILE);

	return(m_ErrorCode);
}


int CCaetla::ReceiveVRAM(FILE *file, int x, int y, int w, int h, int depth) {
	m_FILE = file;

// Validate region

	if (x < 0)       x = 0;
	if (x > 1023)    x = 1023;
	if (y < 0)       y = 0;
	if (y > 511)     y = 511;
	if (w < 1)       w = 1;
	if (h < 1)       h = 1;
	if (x + w > 1024)  w = 1024 - x;
	if (y + h > 512)   h = 512 - y;

	ReceiveVRAMtoFile(x, y, w, h, depth);

	fclose(m_FILE);

	return(m_ErrorCode);
}


int CCaetla::ReceiveVRAMtoFile(int x, int y, int w, int h, int depth) {
	uint8   *vram;
	int             d;

	d = (depth == 16) ? 2 : 3;

	if ((vram = (uint8 *)malloc(w * h * d)) == NULL) {
		complete(CAETLA_ERROR_LOWRAM);
	}

	DownloadVRAM((void *)vram, x, y, w, h, depth);

	if ((int)fwrite(vram, 1, w * h * d, m_FILE) != (w * h * d)) {
		free(vram);
		complete(CAETLA_ERROR_FILE);
	}

	free(vram);

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Save VRAM to RAM

int CCaetla::DownloadVRAM(void *data, int x, int y, int w, int h, int depth) {
//	int              sx,sy,sw,sh,sd;
	int             i;
	uint16  pixel;
	uint8   *vcur;

	int             pc, lpc;
	pc = lpc = 0;


//Validate region - ????? wtf?

	if (x < 0)       x = 0;
	if (x > 1023)    x = 1023;
	if (y < 0)       y = 0;
	if (y > 511)     y = 511;
	if (w < 1)       w = 1;
	if (h < 1)       h = 1;
	if (x + w > 1024)  w = 1024 - x;
	if (y + h > 512)   h = 512 - y;

	depth = (depth == 16) ? 2 : 3;

#if 0   //DISABLE
	switch (m_CartType) {
	case CART_XP: {
		uint8 r;
		IssueCommand(CAETLA_REQUESTPCCONTROL);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		r = Swap8(CAETLA_MODE_VRAM);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		r = Swap8(r);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR: {
		Listen(CAETLA_MODE_VRAM);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;
	}
#endif  //DISABLE
	Listen(CAETLA_MODE_VRAM);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

#if 0   //DISABLE
	IssueCommand(CAETLA_FBV_SETTINGS);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP: {
		sx = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sy = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sw = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sh = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sd = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR: {
		sx = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sy = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sw = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sh = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sd = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;
	}
#endif  //DISABLE

	IssueCommand(CAETLA_FBV_DOWNLOAD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(x);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(y);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(w);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(h);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	vcur = (uint8 *)data;

	InitProgressBar();

//	for(i=0;i<w*h;i++)
	i = 0;
	while ((i < w * h) && (m_ErrorCode == CAETLA_ERROR_OK)) {

		switch (m_CartType) {
		case CART_XP:
			pixel = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
			break;

		default:
		case CART_PAR:
			pixel = Send16(0);
			break;
		}

		pixel = ((pixel & 0xff) << 8) | ((pixel >> 8) & 0xff);
		if (depth == 3) {
			*vcur++ = ((pixel) & 0x1f) << 3;
			*vcur++ = ((pixel >> 5) & 0x1f) << 3;
			*vcur++ = ((pixel >> 10) & 0x1f) << 3;
		} else {
			*vcur++ = (pixel) & 0xff;
			*vcur++ = (pixel >> 8) & 0xff;
		}

		pc = (i * 50) / (w * h);
		if (pc > lpc) {
			UpdateProgressBar(pc, lpc);
			lpc = pc;
		}

		i++;
	}

	EndProgressBar();

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Load VRAM from Filename

int CCaetla::SendVRAM(char *fname, int x, int y, int w, int h, int depth) {
	if (FileLoad(fname) != CAETLA_ERROR_OK) {
		return((int)m_ErrorCode);
	}

	UploadVRAM((void *)m_FilePtr, x, y, w, h, depth);

	return(m_ErrorCode);
}


int CCaetla::SendVRAM(FILE *file, int x, int y, int w, int h, int depth) {
	uint8   *vram;
	int             d;

	d = (depth == 16) ? 2 : 3;

	if ((vram = (uint8 *)malloc(w * h * d)) == NULL) {
		complete(CAETLA_ERROR_LOWRAM);
	}

	if ((int)fread(vram, 1, w * h * d, file) != (w * h * d)) {
		free(vram);
		complete(CAETLA_ERROR_FILE);
	}

	UploadVRAM((void *)vram, x, y, w, h, depth);

	free(vram);

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Send RAM to VRAM

int CCaetla::UploadVRAM(void *data, int x, int y, int w, int h, int depth) {
	int             i;
	uint16  pixel;
	uint8   *vcur, r, g, b;

	int             pc, lpc;
	pc = lpc = 0;

	depth = (depth == 16) ? 2 : 3;

#if 0   //DISABLE
	switch (m_CartType) {
	case CART_XP: {
		uint8 r;
		IssueCommand(CAETLA_REQUESTPCCONTROL);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		r = Swap8(CAETLA_MODE_VRAM);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		r = Swap8(r);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR: {
		Listen(CAETLA_MODE_VRAM);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;
	}
#endif  //DISABLE

	Listen(CAETLA_MODE_VRAM);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	IssueCommand(CAETLA_FBV_UPLOAD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(x);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(y);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(w);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(h);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	vcur = (uint8 *)data;

	InitProgressBar();

//	for(i=0;i<w*h;i++)
	i = 0;
	while ((i < w * h) && (m_ErrorCode == CAETLA_ERROR_OK)) {
		if (depth == 3) {
			r = *vcur++;
			g = *vcur++;
			b = *vcur++;
			pixel = (((b >> 3) & 0x1f) << 10) | (((g >> 3) & 0x1f) << 5) | (((r >> 3) & 0x1f));
		} else {
			r = *vcur++;
			g = *vcur++;
			pixel = ((g << 8) & 0xff) | (r);
		}

		pixel = ((pixel & 0xff) << 8) | ((pixel >> 8) & 0xff);
		Send16(pixel);

		pc = (i * 50) / (w * h);
		if (pc > lpc) {
			UpdateProgressBar(pc, lpc);
			lpc = pc;
		}
		i++;
	}

	EndProgressBar();

	return(m_ErrorCode);
}



////////////////////////////////////////////////////////////////////////////////
// Get VRAM manager's settings

int CCaetla::GetVRAMInfo(void *data) {
	sint16   *pShort = (sint16 *)data;
	sint16   sx, sy, sw, sh, sd;

	int mode = QueryMode();
	if (m_ErrorCode)   complete(m_ErrorCode);

	Listen(CAETLA_MODE_VRAM);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	IssueCommand(CAETLA_FBV_GETINFO);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

#if 0   //DISABLE
	switch (m_CartType) {
	case CART_XP: {
		sx = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sy = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sw = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sh = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sd = (ByteIn(m_Port) << 8) | ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR: {
		sx = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sy = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sw = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sh = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		sd = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;
	}
#endif  //DISABLE
	sx = Receive16();
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	sy = Receive16();
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	sw = Receive16();
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	sh = Receive16();
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	sd = Receive16();
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	*pShort++ = sx;
	*pShort++ = sy;
	*pShort++ = sw;
	*pShort++ = sh;
	*pShort++ = (sd == 1) ? 24 : 16;

	complete(CAETLA_ERROR_OK);
}

int CCaetla::SetVRAMInfo(int x, int y, int w, int h, int depth) {
	int mode = QueryMode();
	if (m_ErrorCode)   complete(m_ErrorCode);
//printf("V");
	Listen(CAETLA_MODE_VRAM);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("V");

	IssueCommand(CAETLA_FBV_SETINFO);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("V");

	Send16(x);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(y);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(w);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(h);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16((depth == 24) ? 1 : 0);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//printf("V");

	complete(CAETLA_ERROR_OK);
}


////////////////////////////////////////////////////////////////////////////////

// Rather a luxurious waste of RAM :)  Should *really* do it in code, it's not
// exactly complex

uint8 BlankMemCard[] = {
	0x4d, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


////////////////////////////////////////////////////////////////////////////////
// Scan a Memory Card port (0 or 1)

// Also stores the status code in the m_MemCards[] structures, from which you
// can determine if the card is present, formatted, etc

int CCaetla::ScanMemCards(int port) {
	uint16 ret;

	Listen(CAETLA_MODE_MEMCARD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	IssueCommand(0x11);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//	Swap8(port);
	Send8(port);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP: {
		ret = ByteIn(m_Port) << 8;
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ret |= ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR:
		ret = Send16(0);
		break;
	}

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	IssueCommand(CAETLA_CARD_CONTENTS);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//	Swap8(port);
	Send8(port);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP: {
		m_MemCards[port].numfiles = ByteIn(m_Port) << 8;
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		m_MemCards[port].numfiles |= ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR:
		m_MemCards[port].numfiles = Send16(0);
		break;
	}

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}


	if (m_MemCards[port].numfiles > 0) {
		int c, i;
		uint8 *data;

		for (c = 0; c < m_MemCards[port].numfiles; c++) {
			data = (uint8 *) & (m_MemCards[port].files[c]);

			for (i = 0; i < 128; i++) {
				switch (m_CartType) {
				case CART_XP:
					data[i] = ByteIn(m_Port);
					break;

				default:
				case CART_PAR:
					data[i] = Swap8(0);
					break;
				}
				if (m_ErrorCode != CAETLA_ERROR_OK)  {
					complete(m_ErrorCode);
				}
			}
		}

		switch (m_CartType) {
		case CART_XP: {
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			m_MemCards[port].status = ByteIn(m_Port) << 8;
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			m_MemCards[port].status |= ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			m_MemCards[port].status = Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;
		}
	} else {
		switch (m_CartType) {
		case CART_XP: {
//				m_MemCards[port].status=(ByteIn(m_Port)<<8)|ByteIn(m_Port);
			m_MemCards[port].status = ByteIn(m_Port) << 8;
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			m_MemCards[port].status |= ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR:
			m_MemCards[port].status = Send16(0);
			break;
		}

		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Get status of a Memory Card port (0 or 1)

int CCaetla::GetCardStatus(int port) {
	IssueCommand(CAETLA_CARD_STATUS);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		return(0);
	}

	switch (m_CartType) {
	case CART_XP: {
		Send8(port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			return(0);
		}
		m_MemCards[port].status = ByteIn(m_Port) << 8;
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			return(0);
		}
		m_MemCards[port].status |= ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			return(0);
		}
	}
	break;

	default:
	case CART_PAR: {
		Swap8(port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			return(0);
		}
		m_MemCards[port].status = Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			return(0);
		}
	}
	break;
	}

//Dump("Card Status: 0x%x\n",m_MemCards[port].status);
	return(m_MemCards[port].status);
}

////////////////////////////////////////////////////////////////////////////////
// Transfer a memcard file to a file

// Requires that ScanMemCards() has been previously called, and subsequent check
// for lack of card etc should be performed after that, it's not done here

int CCaetla::ReadMemCardFile(char *name, int port, int mfile) {
	FILE    *file;
	char    header[54];

	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}
	if (!(m_MemCards[port].status & CARD_F_FORMATTED)) {
		Say("Memory Card in slot %d is not formatted\n", port + 1);
		complete(CAETLA_ERROR_MC_NOTFORMATTED);
	}

	DownloadMemCardFile((void *)&m_MemCardImages[port], port, mfile);

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	if (m_MemCardFileSize) {
		if (!(file = fopen(name, "wb"))) {
			complete(CAETLA_ERROR_FILE);
		}
		memset(&header[0], 0, 54);
		strcpy(&header[0], &m_MemCards[port].files[mfile].name[0]);
		fwrite(&header[0], 1, 54, file);
		fwrite(&m_MemCardImages[port], 1, m_MemCardFileSize, file);
		fclose(file);
	}


	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Download a memcard file to RAM

int CCaetla::DownloadMemCardFile(void *dest, int port, int mfile) {
	int             numblk, i;
	uint8   *cdata;

	cdata = (uint8 *)dest;

	Listen(CAETLA_MODE_MEMCARD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}
	if (!(m_MemCards[port].status & CARD_F_FORMATTED)) {
		Say("Memory Card in slot %d is not formatted\n", port + 1);
		complete(CAETLA_ERROR_MC_NOTFORMATTED);
	}

	IssueCommand(CAETLA_CARD_DOWNLOAD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP: {
		Send8(port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Send8(mfile);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		numblk = ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR: {
		Swap8(port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Swap8(mfile);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		numblk = Swap8(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;
	}

	m_MemCardFileSize = numblk * CARD_BLOCK_SIZE;

	if (numblk) {

		int pc, lpc;

		InitProgressBar();
		pc = lpc = 0;

//		for (i=0;i<(numblk*CARD_BLOCK_SIZE);i++)
		i = 0;
		while ((i < numblk * CARD_BLOCK_SIZE) && (m_ErrorCode == CAETLA_ERROR_OK)) {
			switch (m_CartType) {
			case CART_XP:
				*cdata++ = ByteIn(m_Port);
				break;

			default:
			case CART_PAR:
				*cdata++ = Swap8(0);
				break;
			}

			pc = (i * 50) / (numblk * CARD_BLOCK_SIZE);
			if (pc > lpc) {
				UpdateProgressBar(pc, lpc);
				lpc = pc;
			}
			i++;
		}

		EndProgressBar();

		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

		switch (m_CartType) {
		case CART_XP: {
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;
		}

	} else {
		Say("Memory Card File %d in slot %d not found\n", mfile, port + 1);

		switch (m_CartType) {
		case CART_XP: {
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;
		}
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Transfer a file to memcard

// 'oname' is an optional name for the file in the memcard directory structure,
// usually this is set to a game's product code, like BESCES-...., and this original
// name is preserved in memcard saves. So oname is only really used if for some reason
// you simply have to provide your own unique name...

int CCaetla::SaveMemCardFile(char *oname, char *fname, int port, int mfile) {
	char            header[54];
	uint8   *cdata;

	if (FileLoad(fname) != CAETLA_ERROR_OK) {
		return(m_ErrorCode);
	}

	if ((m_FileSize < (CARD_BLOCK_SIZE + 54)) || ((m_FileSize % CARD_BLOCK_SIZE) != 54)) {
		Say("File [%s] doesn't seem to be a valid size for a memory card image.\n", fname);
		complete(CAETLA_ERROR_FILE);
	}

	cdata = (uint8 *)m_FilePtr;

	memset(&header[0], 0, 54);
	strcpy(&header[0], (char *)cdata);

	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}
	if (!(m_MemCards[port].status & CARD_F_FORMATTED)) {
		Say("Memory Card in slot %d is not formatted\n", port + 1);
		complete(CAETLA_ERROR_MC_NOTFORMATTED);
	}

	if (oname)
		UploadMemCardFile(oname, (void *)((uint8 *)m_FilePtr + 54), port, mfile, m_FileSize - 54);
	else
		UploadMemCardFile(&header[0], (void *)((uint8 *)m_FilePtr + 54), port, mfile, m_FileSize - 54);

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Upload a memcard file from RAM

int CCaetla::UploadMemCardFile(char *name, void *data, int port, int mfile, int size) {
	int             numblk, i, freeblk;
	uint8   *cdata;

	cdata = (uint8 *)data;

	Listen(CAETLA_MODE_MEMCARD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	ScanMemCards(port);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

// Quickly check for either non-presence of memcard, or insufficient room

	for (i = freeblk = 0; i < m_MemCards[port].numfiles; i++)
		freeblk += m_MemCards[port].files[i].size / CARD_BLOCK_SIZE;
	freeblk = CARD_NUM_FILES - freeblk;

	if (!(m_MemCards[port].status & CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}
	if (!(m_MemCards[port].status & CARD_F_FORMATTED)) {
		Say("Memory Card in slot %d is not formatted\n", port + 1);
		complete(CAETLA_ERROR_MC_NOTFORMATTED);
	}

	if ((freeblk <= 0) || (freeblk < (size / CARD_BLOCK_SIZE))) {
		Say("No room on memory card.\n");
		complete(CAETLA_ERROR_MC_CARDFULL);
	}

	IssueCommand(CAETLA_CARD_UPLOAD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//	Swap8(port);
	Send8(port);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

#if 0   //DISABLE
	for (i = 0; name[i]; i++)
		if (m_XPlorer)
			Send8(name[i]);
		else
			Swap8(name[i]);
#endif  //DISABLE
	i = 0;
	while ((name[i]) && (m_ErrorCode == CAETLA_ERROR_OK)) {
		Send8(name[i]);
		i++;
	}
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//	Swap8(0);
	Send8(0);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	Send32(size);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP:
		numblk = ByteIn(m_Port);
		break;

	default:
	case CART_PAR:
		numblk = Swap8(0);
		break;
	}

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	if (numblk) {
		int pc, lpc;

		InitProgressBar();
		pc = lpc = 0;

//		for (i=0;i<size;i++)
		i = 0;
		while ((i < size) && (m_ErrorCode == CAETLA_ERROR_OK)) {

			Send8(cdata[i]);

			pc = (i * 50) / size;
			if (pc > lpc) {
				UpdateProgressBar(pc, lpc);
				lpc = pc;
			}
			i++;
		}
		EndProgressBar();
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

		switch (m_CartType) {
		case CART_XP: {
			Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR: {
			m_MemCards[port].status = Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			Send16(0);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;
		}
	}

	else {
		Say("Error - Unable to upload Memory Card file\n");

		switch (m_CartType) {
		case CART_XP: {
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
			ByteIn(m_Port);
			if (m_ErrorCode != CAETLA_ERROR_OK)  {
				complete(m_ErrorCode);
			}
		}
		break;

		default:
		case CART_PAR:
			Send16(0);
			break;
		}

		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Delete a Memory Card file
// Requires that ScanMemCards() has previously been called

int CCaetla::DeleteMemCardFile(int port, int mfile) {
	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}
	if (!(m_MemCards[port].status & CARD_F_FORMATTED)) {
		Say("Memory Card in slot %d is not formatted\n", port + 1);
		complete(CAETLA_ERROR_MC_NOTFORMATTED);
	}

	if (m_MemCards[port].numfiles) {
#if 0

// this is the NUKE version, actually nullifies all sectors/blocks, relocates
// following data, etc

// Really redundant, don't need to go this far...

		MEMCARD_HEADER  mchead;
		MEMCARD_HEADER  *pfile;
		uint8   *cdata;
		int             i, blkoffset;

		if (mfile >= m_MemCards[port].numfiles) {
			complete(CAETLA_ERROR_OUTOFRANGE);
		}

		memset((void *)&mchead, 0, sizeof(MEMCARD_HEADER));
		mchead.pad0[0] = 0xa0;
		mchead.pad1 = 0xffff;

		ReadCardSectors((void *)&m_MemCardImages[port].image, port, 0, CARD_SIZE);

//		pfile=(MEMCARD_HEADER *)&m_MemCards[port].files[mfile];

		pfile = (MEMCARD_HEADER *)&m_MemCardImages[port].image[CARD_SECTOR_SIZE];

		for (i = blkoffset = 0; i < mfile; i++) {
			blkoffset += (pfile->size / CARD_BLOCK_SIZE);
			pfile += (pfile->size / CARD_BLOCK_SIZE);
		}

//		Say("blkoffset==%d, size=%d\n",blkoffset,m_MemCards[port].files[mfile].size);

		cdata = (uint8 *)&m_MemCardImages[port].image[(blkoffset+1)*CARD_BLOCK_SIZE];

// Move savegame data blocks & savegame dir entry down to occupy the space

		for (i = 0; i < 15 - blkoffset - (m_MemCards[port].files[mfile].size / CARD_BLOCK_SIZE); i++, cdata += CARD_BLOCK_SIZE, pfile++) {
//			memcpy((void *)(cdata+CARD_BLOCK_SIZE*m_MemCards[port].files[mfile]),(void *)cdata,CARD_BLOCK_SIZE);
			memcpy((void *)cdata, (void *)(cdata + m_MemCards[port].files[mfile].size), CARD_BLOCK_SIZE);
			memcpy((void *)pfile, (void *)(pfile + (m_MemCards[port].files[mfile].size / CARD_BLOCK_SIZE)), CARD_SECTOR_SIZE);
		}

		for (i = 0; i < (m_MemCards[port].files[mfile].size / CARD_BLOCK_SIZE); i++, cdata += CARD_BLOCK_SIZE, pfile++) {
			memset((void *)cdata, 0xff, CARD_BLOCK_SIZE);
			memcpy((void *)pfile, (void *)&mchead, sizeof(MEMCARD_HEADER));
		}

		WriteCardSectors((void *)&m_MemCardImages[port].image, port, 0, CARD_SIZE);
#else

// Normal everyday (and faster) version... :)

		MEMCARD_HEADER  *pfile;

		if (mfile >= m_MemCards[port].numfiles) {
			Say("Error - Memory Card file not found\n");
			complete(CAETLA_ERROR_OUTOFRANGE);
		}

// Read in memcard TOC

		ReadCardSectors((void *)&m_MemCardImages[port].image, port, 0, CARD_SECTOR_SIZE * 16);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}

// Start scanning at the first sector so our pre-increment seek works OK.
// This is OK as this sector is the MC header and the pre-increment will immediately
// take our first check into the first actual directory entry anyway.

		pfile = (MEMCARD_HEADER *)&m_MemCardImages[port].image[0];

		while (mfile >= 0) {
			while ((++pfile)->pad0[0] != (CARD_TOC_ENTRYUSED | 0x01)) ;
			mfile--;
		}

		while (1) {
			pfile->pad0[0] = (pfile->pad0[0] & 0x0f) | (CARD_TOC_ENTRYFREE);
			if (pfile->nextsector == 0xffff)
				break;
			else
				pfile = (MEMCARD_HEADER *)(&m_MemCardImages[port].image[CARD_SECTOR_SIZE] + (pfile->nextsector * CARD_SECTOR_SIZE));
		}

		WriteCardSectors((void *)&m_MemCardImages[port].image, port, 0, CARD_SECTOR_SIZE * 16);

#endif
	}
	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Format a Memory Card

int CCaetla::FormatMemCard(int port) {
	Listen(CAETLA_MODE_MEMCARD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}

	memset(&m_MemCardImages[port].image, 0, CARD_SIZE);
	memcpy(&m_MemCardImages[port].image, &BlankMemCard, sizeof(BlankMemCard));
	WriteCardSectors((void *)&m_MemCardImages[port].image, port, 0, CARD_SIZE);

	return(m_ErrorCode);
}

////////////////////////////////////////////////////////////////////////////////
// Backup a Memory Card

int CCaetla::BackupCard(char *fname, int port) {
	FILE    *file;

	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}

	ReadCardSectors((void *)&m_MemCardImages[port].image, port, 0, CARD_SIZE);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	file = fopen(fname, "wb");
	if (file) {
		fwrite(&m_MemCardImages[port].image, 1, CARD_SIZE, file);
		fclose(file);
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Restore a Memory Card

int CCaetla::RestoreCard(char *fname, int port) {
	if (FileLoad(fname) != CAETLA_ERROR_OK) {
		return(m_ErrorCode);
	}

	if (m_FileSize != CARD_SIZE) {
		Say("Filesize of [%s] is invalid for a Memory Card image.\n", fname);
		complete(CAETLA_ERROR_FILEFORMAT);
	}

	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}

	WriteCardSectors((void *)m_FilePtr, port, 0, CARD_SIZE);

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Read sectors from Memory Card

int CCaetla::ReadCardSectors(void *data, int port, int start, int len) {
	int             i;
	uint8   *cdata;

	cdata = (uint8 *)data;

	start = (start + CARD_SECTOR_SIZE - 1) & (~(CARD_SECTOR_SIZE - 1));
	len = (len + CARD_SECTOR_SIZE - 1) & (~(CARD_SECTOR_SIZE - 1));

	if ((start < 0) || (start > (CARD_SIZE - CARD_SECTOR_SIZE)) || (start + len < CARD_SECTOR_SIZE) || (start + len > CARD_SIZE) || (start + len <= start)) {
		complete(CAETLA_ERROR_OUTOFRANGE);
	}

	Listen(CAETLA_MODE_MEMCARD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}

	IssueCommand(CAETLA_CARD_READSECTOR);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//	Swap8(port);
	Send8(port);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(start / CARD_SECTOR_SIZE);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	Send32(len);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	{
		int pc, lpc;

		InitProgressBar();
		pc = lpc = 0;

//	for  (i=0;i<len;i++)
		i = 0;
		while ((i < len) && (m_ErrorCode == CAETLA_ERROR_OK)) {

			switch (m_CartType) {
			case CART_XP:
				cdata[i+start] = ByteIn(m_Port);
				break;

			default:
			case CART_PAR:
				cdata[i+start] = Swap8(0);
				break;
			}

			pc = (i * 50) / len;
			if (pc > lpc) {
				UpdateProgressBar(pc, lpc);
				lpc = pc;
			}
			i++;
		}

		EndProgressBar();
	}

	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP: {
		//      Send16(0);
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR: {
		Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;
	}

	return(m_ErrorCode);
}


////////////////////////////////////////////////////////////////////////////////
// Write sectors to Memory Card

int CCaetla::WriteCardSectors(void *data, int port, int start, int len) {
	int             i;
	uint8   *cdata;

	cdata = (uint8 *)data;

	start = (start + CARD_SECTOR_SIZE - 1) & (~(CARD_SECTOR_SIZE - 1));
	len = (len + CARD_SECTOR_SIZE - 1) & (~(CARD_SECTOR_SIZE - 1));

	if ((start < 0) || (start > (CARD_SIZE - CARD_SECTOR_SIZE)) || (start + len < CARD_SECTOR_SIZE) || (start + len > CARD_SIZE) || (start + len <= start)) {
		complete(CAETLA_ERROR_OUTOFRANGE);
	}

	Listen(CAETLA_MODE_MEMCARD);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	if (!(GetCardStatus(port)&CARD_F_PRESENT)) {
		Say("No Memory Card present in slot %d\n", port + 1);
		complete(CAETLA_ERROR_MC_NOCARD);
	}

	IssueCommand(CAETLA_CARD_WRITESECTOR);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
//	Swap8(port);
	Send8(port);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}
	Send16(start / CARD_SECTOR_SIZE);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	Send32(len);
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	{
		int pc, lpc;

		InitProgressBar();
		pc = lpc = 0;

//	for  (i=0;i<len;i++)
		i = 0;
		while ((i < len) && (m_ErrorCode == CAETLA_ERROR_OK)) {
			Send8(*cdata++);

			pc = (i * 50) / len;
			if (pc > lpc) {
				UpdateProgressBar(pc, lpc);
				lpc = pc;
			}
			i++;
		}
		EndProgressBar();
	}
	if (m_ErrorCode != CAETLA_ERROR_OK)  {
		complete(m_ErrorCode);
	}

	switch (m_CartType) {
	case CART_XP: {
		Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		ByteIn(m_Port);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;

	default:
	case CART_PAR: {
		Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
		Send16(0);
		if (m_ErrorCode != CAETLA_ERROR_OK)  {
			complete(m_ErrorCode);
		}
	}
	break;
	}

	return(m_ErrorCode);
}




////////////////////////////////////////////////////////////////////////////////
//
// PSX AR Reflash program

//#include "flasher.c"
#include "NagFlash.c"


int CCaetla::SendUpgrade(void *pdata, uint32 addr, uint32 length, bool rom) {
	uint16  checksum;
	uint8   *data, r;
	uint32   count;
	int             pc, lpc;

	if (m_CartType != CART_PAR)
		return(0);

//	m_TimeOut=0;
	m_TimeOut = _SECONDS(10);

	while (Swap8('W') != 'R') ;

	while (Swap8('B') != 'W');

	if (rom) {
		Swap8('U');
	} else {
		Swap8('X');
	}

	Send32(addr);
	Send32(length);

	checksum = 0;

	pc = lpc = 0;
	InitProgressBar();

	for (count = 0, data = (uint8 *)pdata; count < length; ++count) {
		r = data[count];
		checksum += (uint16)r;
		Swap8(r);

		pc = (count * 50) / length;
		if (pc > lpc) {
			UpdateProgressBar(pc, lpc);
			lpc = pc;
		}
	}

	EndProgressBar();

	Send16(checksum & 0x0fff);

	if (Swap8(0) != 'O') {
		Say("CheckSum Bad\n");
		complete(CAETLA_ERROR_PROTOCOL);
	}
	if (Swap8(0) != 'K') {
		Say("CheckSum Bad\n");
		complete(CAETLA_ERROR_PROTOCOL);
	}

	complete(CAETLA_ERROR_OK);
}


int CCaetla::FlashEPROM(char *romname) {
	const char  *lic = "Licensed by Sony Computer Entertainment Inc.";

	if (FileLoad(romname) != CAETLA_ERROR_OK) {
		return(m_ErrorCode);
	}

	if (strncmp((char *)m_FilePtr + 4, lic, strlen(lic))) {
		Say("'%s' is not a valid ROM image.\n", romname);
		return(m_ErrorCode);
	}

	Say("Initiating ROM Reflash - press RESET on your PlayStation now...\n");
//	SendUpgrade(&FlashProg[0],0x80020000,sizeof(FlashProg),false);
	SendUpgrade(&NagFlash[0], 0x80020000, sizeof(NagFlash), false);
	Say("Sending ROM image...");
	SendUpgrade(m_FilePtr, 0x80040000, m_FileSize, true);
	Say("Done.\n");
	Say("Press RESET on your PlayStation now.\n");

	return(0);
}






////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
//// SOME USEFUL HELPING-HAND FUNCTIONS!
////
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// 'Say' a message to either stdout or a hooked function

void CCaetla::Say(const char *fmt, ...) {
	char    dump[2048];

	va_list args;

	va_start(args, fmt);
	vsprintf(dump, fmt, args);
	va_end(args);

	if (m_ConsolePrintHook) {
		m_ConsolePrintHook(&dump[0]);
	} else
		printf("%s", dump);
}


////////////////////////////////////////////////////////////////////////////////
// 'Dump' a message to either stdout or a hooked function, plus the LogFile

void CCaetla::Dump(const char *fmt, ...) {
	char    dump[2048];

	va_list args;

	va_start(args, fmt);
	vsprintf(dump, fmt, args);
	va_end(args);

	if (m_FpLog) {
		fprintf(m_FpLog, "%s", dump);
		fflush(m_FpLog);
	}

	if (m_ConsolePrintHook) {
		m_ConsolePrintHook(&dump[0]);
	} else
		printf("%s", dump);

}



////////////////////////////////////////////////////////////////////////////////
// Progress Bar drawing

void CCaetla::InitProgressBar(void) {
	int i;

	if (!m_ShowProgress)   return;

	Say("\r%c", m_ProgressCharS);
	for (i = 0; i < 50; i++)
		Say("%c", m_ProgressCharO);
	Say("%c\r%c", m_ProgressCharE, m_ProgressCharS);
}

void CCaetla::UpdateProgressBar(int percent, int lastpercent) {
	int count;

	if (!m_ShowProgress)   return;

	count = percent - lastpercent;

	while (count--)
		Say("%c", m_ProgressCharI);
}

void CCaetla::EndProgressBar(void) {
	if (!m_ShowProgress)   return;

	Say("\r                                                    \r");
}


