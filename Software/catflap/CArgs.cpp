
////////////////////////////////////////////////////////////////////////////////
//
// CArgs Class
// - handles basic processing of commandline arguments
//
// (C) 1998 Ash Hogg,Intar Technologies Limited
//
////////////////////////////////////////////////////////////////////////////////

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "CArgs.h"

#define     complete(x) { m_ErrorCode=(void *)x; return(x); }

////////////////////////////////////////////////////////////////////////////////
// Constructor

CArgs::CArgs() {
	m_Status = ARGSTATUS_BAD;
	m_ErrorCode = 0;
	m_ArgC = 0;
	m_ArgV = NULL;
	m_ArgMin = -1;
	m_ArgMax = -1;
}

////////////////////////////////////////////////////////////////////////////////
// Destructor

CArgs::~CArgs() {
	if (m_ArgV)
		free(m_ArgV);
}


////////////////////////////////////////////////////////////////////////////////
// Init the CArgs class with an argc/argv[] from wherever

void CArgs::Create(int argc, char *argv[]) {
	if (m_ArgC) {
		m_ArgC = 0;
		if (m_ArgV) {
			free(m_ArgV);
		}
		m_Status = ARGSTATUS_BAD;
	}

	m_ArgC = argc;

	if (m_ArgC) {
		m_ArgV = (char **)malloc(sizeof(char *) * m_ArgC);
		for (int a = 0; a < m_ArgC; a++) {
			m_ArgV[a] = argv[a];
		}
		m_Status = ARGSTATUS_OK;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Set the range of Args for subsequent operations

void CArgs::SetRange(int min, int max) {
	if ((min < 0) || (min >= m_ArgC))
		m_ArgMin = -1;
	else
		m_ArgMin = min;

	if ((max < 0) || (max >= m_ArgC) || (m_ArgMin < m_ArgMin))
		m_ArgMax = -1;
	else
		m_ArgMax = max + 1;
}


////////////////////////////////////////////////////////////////////////////////
// Check for a particular switch - the switchname parameter must NOT begin with
//  a switch modifier (- or /)
//
// We have 2 versions - one which passes an Arg_SwitchAssign structure, and one
// which passes manual parameters - neat use of operator overloading.


// ...passing parameters via an Arg_SwitchAssign structure

void CArgs::ProcessSwitch(struct Arg_SwitchAssign *sw) {
	char    theswitch[256];
	int     switchlen = 0;
	int     lower, upper;

	if (m_Status != ARGSTATUS_OK)    return;

// Copy the switch to our temp array, truncate to 255 if necessary and 0-terminate
	strncpy(&theswitch[0], sw->Name, (strlen(sw->Name) >= 256) ? 255 : strlen(sw->Name));
	theswitch[(strlen(sw->Name)>=256)?255:strlen(sw->Name)] = 0;

	switchlen = strlen(theswitch);

	lower = (m_ArgMin == -1) ? 0 : m_ArgMin;
	upper = (m_ArgMax == -1) ? m_ArgC : m_ArgMax;

	for (int a = lower; a < upper; a++) {
		char *ArgV = m_ArgV[a];
		if (*(ArgV + 0) == '-') {
			int b = 0;
			for (b = 0; b < switchlen; b++) {
				char s, d;
				s = theswitch[b];
				d = (char) * (ArgV + 1 + b);
				if (sw->Mode == ARG_CASE_INSENSITIVE) {
					if ((s >= 'A') && (s <= 'Z')) s -= 'Z' - 'z';
					if ((d >= 'A') && (d <= 'Z')) d -= 'Z' - 'z';
				}
				if (s != d)  break;
			}
			if (b == switchlen) {
				sw->Callback((ArgV + 1 + switchlen));
				return;
			}
		}
	}
}


// ...passing parameters manually

void CArgs::ProcessSwitch(char *switchname, void (*Callback)(char *arg), int mode) {
	char    theswitch[256];
	int     switchlen = 0;
	int     lower, upper;

	if (m_Status != ARGSTATUS_OK)    return;

// Copy the switch to our temp array, truncate to 255 if necessary and 0-terminate
	strncpy(&theswitch[0], switchname, (strlen(switchname) >= 256) ? 255 : strlen(switchname));
	theswitch[255] = 0;
	theswitch[(strlen(switchname)>=256)?255:strlen(switchname)] = 0;

	switchlen = strlen(theswitch);

	lower = (m_ArgMin == -1) ? 0 : m_ArgMin;
	upper = (m_ArgMax == -1) ? m_ArgC : m_ArgMax;

	for (int a = lower; a < upper; a++) {
		char *ArgV = m_ArgV[a];
		if (*(ArgV + 0) == '-') {
			int b = 0;
			for (b = 0; b < switchlen; b++) {
				char s, d;
				s = theswitch[b];
				d = (char) * (ArgV + 1 + b);
				if (mode == ARG_CASE_INSENSITIVE) {
					if ((s >= 'A') && (s <= 'Z')) s -= 'Z' - 'z';
					if ((d >= 'A') && (d <= 'Z')) d -= 'Z' - 'z';
				}
				if (s != d)  break;
			}
			if (b == switchlen) {
				Callback((ArgV + 1 + switchlen));
				return;
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Return the Nth argument which is NOT a switch
// *NOTE* number 0 SHOULD return the executable's filename, 1 & above should be
// any parameters passed by the user

char *CArgs::GetNormalArg(int num) {
	int     lower, upper;

	if (m_Status != ARGSTATUS_OK)    complete(NULL);

	lower = (m_ArgMin == -1) ? 0 : m_ArgMin;
	upper = (m_ArgMax == -1) ? m_ArgC : m_ArgMax;

	for (int a = lower; a < upper; a++) {
		char *ArgV = m_ArgV[a];
		if (*(ArgV + 0) != '-') {
			if (num == 0) {
				complete(ArgV);
			} else {
				num--;
			}
		}
	}

	complete(NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Return the number of arguments (in total)

int CArgs::GetNumArgs(void) {
	if (m_Status != ARGSTATUS_OK)
		return(0);
	else
		return(m_ArgC);
}

////////////////////////////////////////////////////////////////////////////////
// Return the number of non-switch arguments

int CArgs::GetNumNormalArgs(void) {
	int num = 0;

	if (m_Status != ARGSTATUS_OK)
		return(0);
	else {
		for (int a = 0; a < m_ArgC; a++) {
			char *ArgV = m_ArgV[a];
			if ((*(ArgV + 0) != '/') && (*(ArgV + 0) != '-')) {
				num++;
			}
		}
	}
	return(num);
}


////////////////////////////////////////////////////////////////////////////////
// Return the index of the Nth argument which is NOT a switch

int CArgs::GetNormalArgIndex(int num) {
	int idx = 0;

	if (m_Status != ARGSTATUS_OK)
		return(-1);
	else {
		for (int a = 0; a < m_ArgC; a++) {
			char *ArgV = m_ArgV[a];
			if (*(ArgV + 0) != '-') {
				if (num == 0) {
					return(a);
				} else {
					num--;
				}
			}
		}
	}
	return(-1);
}
