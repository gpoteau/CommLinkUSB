
/////////////////////////////////////////////////////////////////////////////
// CArgs Class - (C) 1998 Ash Hogg,Intar Technologies Limited


enum ArgsCaseMode {
	ARG_CASE_SENSITIVE,
	ARG_CASE_INSENSITIVE,
};

enum ArgsStatus {
	ARGSTATUS_BAD,
	ARGSTATUS_OK,
};



typedef struct Arg_SwitchAssign {
	const char  *Name;
	void (*Callback)(char *arg);
	int     Mode;
}   ARG_SWITCHASSIGN;



class CArgs {
// Construction
public:
	CArgs();
	virtual ~CArgs();

	void Create(int, char *[]);
	void SetRange(int, int);
	void ProcessSwitch(struct Arg_SwitchAssign *);
	void ProcessSwitch(char *, void (*Func)(char *arg), int mode);
	char *GetNormalArg(int);
	int GetNormalArgIndex(int);
	int GetNumArgs(void);
	int GetNumNormalArgs(void);

	void *m_ErrorCode;


protected:
	int         m_ArgC;
	char        **m_ArgV;
	int         m_Status;
	int         m_ArgMin;
	int         m_ArgMax;

};
