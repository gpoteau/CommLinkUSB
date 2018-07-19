
#ifndef _CCAETLA_H_
#define _CCAETLA_H_

/////////////////////////////////////////////////////////////////////////////
// CCaetla Class - (C) 1998 Intar Technologies Limited

#include "CCaetlaDefs.h"


class CCaetla {
// Construction
public:
	CCaetla();
	virtual ~CCaetla();

	int             OpenLogFile(void);
	int             CloseLogFile(void);
	int             OpenNTDriver(void);
	int             CloseNTDriver(void);
	int             StartNTService(void);
	int             StopNTService(void);

	void            Init(int, int);

	int             GetVersion(void);

	int             Detect(void);
	int             QueryMode(void);
	int             ChooseMainOrDebug(void);
	int             ServerMode(int);
	int             Resume(void);
	int             RunExe(char *, bool, bool);
	int             RunExe(void *, bool, bool);
	int             SendFile(char *, uint32);
	int             ReceiveFile(char *, uint32, uint32);
	int             Reset(void);
	int             Listen(int);
	int             CheckExeStatus(void);

	int             UpdateByte(uint32, uint8);
	int             GetRegisters(void *);
	int             SetRegister(int, uint32);
	int             GetCpCond(void);
	int             FlushICache(void);
	int             SetHBP(uint32, uint32, uint32, uint32);
	int             DisableHBP(void);

	int             DownloadDbg(void *, uint32, uint32, bool, bool);
	int             UploadDbg(void *, uint32, uint32, bool, bool);

	uint8   Swap8(uint8);
	uint8   Send8(uint8);
	uint8   Get8(void);
	uint8   Get8s(void);

	int             DosConsole(void);
	int             InitWinConsole(void);
	int             WinConsole(void);
	int             EndWinConsole(void);

	int             ServerCommand(uint8);
	int             RawConsole(void);

	int             Download(void *, uint32, uint32, bool, bool);
	int             Upload(void *, uint32, uint32, bool, bool);

	int             ShowError(void);

	int             InstallNT(void);
	int             UninstallNT(void);

	int             ReceiveVRAM(char *, int, int, int, int, int);
	int             ReceiveVRAM(FILE *, int, int, int, int, int);
	int             ReceiveVRAMtoFile(int, int, int, int, int);
	int             SendVRAM(char *, int, int, int, int, int);
	int             SendVRAM(FILE *, int, int, int, int, int);

	int             DownloadVRAM(void *, int, int, int, int, int);
	int             UploadVRAM(void *, int, int, int, int, int);

	int             GetVRAMInfo(void *data);
	int             SetVRAMInfo(int, int, int, int, int);

	int             ScanMemCards(int);
	int             GetCardStatus(int);
	int             ReadMemCardFile(char *, int, int);
	int             DownloadMemCardFile(void *, int, int);
	int             SaveMemCardFile(char *, char *, int, int);
	int             UploadMemCardFile(char *, void *, int, int, int);
	int             DeleteMemCardFile(int, int);
	int             FormatMemCard(int);
	int             BackupCard(char *, int);
	int             RestoreCard(char *, int);

	int             ReadCardSectors(void *, int, int, int);
	int             WriteCardSectors(void *, int, int, int);

	int             SendUpgrade(void *, uint32, uint32, bool);
	int             FlashEPROM(char *);

	void            Say(const char *, ...);
	void            Dump(const char *, ...);


	int             m_ErrorCode;
	uint32  m_TimeOut;

	int             m_Port;
	int             m_CartType;
	char            *m_LogFile;
	FILE            *m_FpLog;

	int             m_KeyIn;
	int             m_KeyEnable;
	sint16          m_DirectKey;        // for Direct KeyCode Input

	int             m_FileSize;

	int             m_RunningAsDLL;
	bool            m_ForceQuitConsole;
	bool            m_QuitConsole;
	bool            m_InConsole;
	bool            m_ConsoleLinkStatus;
	uint32  m_ConsoleLastValidTime;
	int             m_XpFlagged;
	int             m_LastXpFlagged;

	MEMCARD_STATUS  m_MemCards[2];
	MEMCARD_IMAGE   m_MemCardImages[2];
	int             m_MemCardFileSize;

	int (*m_ConsoleHook)(uint8, FILE *);

	sint16(*m_ConsoleKeyHook)(void);
	void (*m_ConsolePrintHook)(char *);

	uint8   m_ProgressCharS;
	uint8   m_ProgressCharE;
	uint8   m_ProgressCharI;
	uint8   m_ProgressCharO;
	bool            m_ShowProgress;

	PSX_EXE_HEADER  m_ExeHeader;
	bool            m_ExePending;
	bool            m_ExeHooksOn;

	bool            m_RealTimeOut;
	bool            m_AutoSense;

protected:
//	int              m_Status;

	void            *m_FilePtr;

	FILE            *m_FILE;

	uint8   RawPortIn(int);
	void            RawPortOut(int, uint8);

	uint8   ByteIn(int);
	void            ByteOut(int, uint8);
	int             Handshake(int);

	void            CreateXPLookup(void);
	uint8   XpAck1(void);
	uint8   XpAck1_N(void);
	uint8   XpAck2(void);
//	uint8    XpAck2_N(void);

	uint8   Receive(void);
	uint16  Send16(uint16);
	uint32  Send32(uint32);
	uint16  Receive16(void);
	uint32  Receive32(void);
	int             SendData(void *, uint32, bool);

	int             ReceiveData(void *, uint32, bool);

	int             ReceiveString(char *);
	int             SendString(char *);

	int             FileSize(char *);
	int             FileLoad(char *);
	int             Execute(PSX_EXE_HEADER *);

	int             SendExe(void *);


	int             IssueCommand(uint8);

	uint8   m_XPLookups[256];

	void            InitProgressBar(void);
	void            UpdateProgressBar(int, int);
	void            EndProgressBar(void);

};

#endif  //_CCAETLA_H_
