
#ifndef _CCAETLADEFS_H_
#define _CCAETLADEFS_H_

#include "common_defs.h"

enum {
	STATUS_BAD,
	STATUS_OK,
};

enum {
	CART_PAR = 0,
	CART_XP,
	CART_MAX
};

enum {
	CAETLA_ERROR_OK = 0,
	CAETLA_ERROR_UNKNOWN,
	CAETLA_ERROR_BADSTATUS,
	CAETLA_ERROR_TIMEOUT,
	CAETLA_ERROR_PROTOCOL,
	CAETLA_ERROR_FILE,
	CAETLA_ERROR_FILEFORMAT,
	CAETLA_ERROR_FILENOTEXIST,
	CAETLA_ERROR_LOWRAM,
	CAETLA_ERROR_NTNODRIVER,
	CAETLA_ERROR_NTIO,
	CAETLA_ERROR_OUTOFRANGE,
	CAETLA_ERROR_MC_NOCARD,
	CAETLA_ERROR_MC_CARDFULL,
	CAETLA_ERROR_MC_NOTFORMATTED,
	CAETLA_ERROR_MC_FILEEXISTS,
};

typedef struct CaetlaErrorMsg {
	int     ErrNum;
	const char  *ErrMsg;
}   CAETLAERRORMSG;


enum CaetlaModes {
	CAETLA_MODE_MAIN = 0,
	CAETLA_MODE_MEMCARD,
	CAETLA_MODE_VRAM,
	CAETLA_MODE_03RESERVED,
	CAETLA_MODE_CD,
	CAETLA_MODE_05RESERVED,
	CAETLA_MODE_DEBUG,

};

enum CaetlaProtocols {
	CAETLA_NOP = 0,
	CAETLA_REQUESTPCCONTROL,
	CAETLA_RESUME,
	CAETLA_RESET,
	CAETLA_CONSOLEMODE,
	CAETLA_05,
	CAETLA_06,
	CAETLA_07,
	CAETLA_STATUS,
	CAETLA_EXESTATUS,
	CAETLA_0A,
	CAETLA_0B,
	CAETLA_0C,
	CAETLA_0D,
	CAETLA_0E,
	CAETLA_0F,
	CAETLA_VERSION,
	CAETLA_DOWNLOADEXE,
	CAETLA_12,
	CAETLA_LISTEN,
	CAETLA_UPLOAD,
	CAETLA_DOWNLOAD,
	CAETLA_EXECUTE,
	CAETLA_17,
	CAETLA_18,
};

enum CaetlaDebugProtocols {
	CAETLA_DEBUG_VERSION = 0x10,
	CAETLA_DEBUG_GETREGISTERS,
	CAETLA_DEBUG_SETREGISTER,
	CAETLA_DEBUG_GETCPCOND,
	CAETLA_DEBUG_FLUSHICACHE,
	CAETLA_DEBUG_SETHBP,
	CAETLA_DEBUG_DISABLEHBP,
	CAETLA_DEBUG_DOWNLOAD,
	CAETLA_DEBUG_UPLOAD,
	CAETLA_DEBUG_UPDATEBYTE,
	CAETLA_DEBUG_UPDATEWORD,
	CAETLA_DEBUG_READWORD,
};


enum CaetlaFBVProtocols {
	CAETLA_FBV_VERSION = 0x10,
	CAETLA_FBV_DOWNLOAD,
	CAETLA_FBV_UPLOAD,
	CAETLA_FBV_GETINFO,
	CAETLA_FBV_14,
	CAETLA_FBV_15,
	CAETLA_FBV_GETSTATUS,
	CAETLA_FBV_SETINFO,
};


enum CaetlaMCMProtocols {
	CAETLA_CARD_VERSION = 0x10,
	CAETLA_CARD_11,
	CAETLA_CARD_12,
	CAETLA_CARD_STATUS,
	CAETLA_CARD_READSECTOR,
	CAETLA_CARD_WRITESECTOR,
	CAETLA_CARD_16,
	CAETLA_CARD_CONTENTS,
	CAETLA_CARD_DOWNLOAD,
	CAETLA_CARD_UPLOAD,
	CAETLA_CARD_1A,
	CAETLA_CARD_FORMAT,
};

enum {
	CARD_B_PRESENT = 0,
	CARD_B_FORMATTED,
	CARD_B_STATECHANGED,
	CARD_B_PCCOMMERR = 10,
	CARD_B_UNAVAILABLE,
	CARD_B_SAMENAME,
	CARD_B_CARDFAIL,
	CARD_B_LOCKSLOT,
	CARD_B_RUNTIME,
};

#define CARD_F_PRESENT      (1<<CARD_B_PRESENT)
#define CARD_F_FORMATTED    (1<<CARD_B_FORMATTED)
#define CARD_F_STATECHANGED (1<<CARD_B_STATECHANGED)
#define CARD_F_PCCOMMERR    (1<<CARD_B_PCCOMMERR)
#define CARD_F_UNAVAILABLE  (1<<CARD_B_UNAVAILABLE)
#define CARD_F_SAMENAME     (1<<CARD_B_SAMENAME)
#define CARD_F_CARDFAIL     (1<<CARD_B_CARDFAIL)
#define CARD_F_LOCKSLOT     (1<<CARD_B_LOCKSLOT)
#define CARD_F_RUNTIME      (1<<CARD_B_RUNTIME)



#define CARD_NUM_BLOCKS     16
#define CARD_NUM_FILES      (CARD_NUM_BLOCKS-1)
#define CARD_SECTOR_SIZE    128
#define CARD_BLOCK_SIZE     (CARD_SECTOR_SIZE*64)
#define CARD_SIZE           (CARD_BLOCK_SIZE*CARD_NUM_BLOCKS)

#define CARD_TOC_ENTRYFREE  0xa0
#define CARD_TOC_ENTRYUSED  0x50

typedef struct memcard_header {
	uint8       pad0[4];
	uint32      size;
	uint16      nextsector;
	char                name[20];
	uint8       pad2[4];
	char                comment[94];
}   MEMCARD_HEADER;

typedef struct memcard_status {
	struct memcard_header   files[CARD_NUM_FILES];
	uint32                      numfiles;
	uint32                      status;
}   MEMCARD_STATUS;


typedef struct memcard_image {
	uint8   image[CARD_SIZE];
}   MEMCARD_IMAGE;



typedef struct {
	char    id[8];
	sint32      text;
	sint32      data;
	sint32      pc0;
	sint32      gp0;
	sint32      t_addr;
	sint32      t_size;
	sint32      d_addr;
	sint32      d_size;
	sint32      b_addr;
	sint32      b_size;
	sint32      sp_addr;
	sint32      sp_size;
	sint32      SavedSP;
	sint32      SavedFP;
	sint32      SavedGP;
	sint32      SavedRA;
	sint32      SavedS0;
}   PSX_EXE_HEADER;


#define PSX_CPE_END         0x0
#define PSX_CPE_CHUNK       0x1
#define PSX_CPE_REGISTER    0x3
#define PSX_CPE_TARGET      0x8


// FIXME: are these actually "longs" or just other errors?
typedef struct {
	uint32  address;
	uint32  length;
//	unsigned char    bytes[0];
}   CPE_CHUNK_HEADER;

typedef struct {
	uint16  regreg;
	uint32  val;
}   CPE_REGISTER;

typedef struct {
	uint8   target;
}   CPE_TARGET;

#define MAX_PPDEV_SIZE 64

#define CAETLA_DOSCON_PROMPT    "% "
#define CAETLA_SOFTWARE_TIMEOUT 100000000


#define CAETLA_DEF_PPDEV    "/dev/parport0"
#define CAETLA_DEF_PORT     0x378
#define CAETLA_DEF_CART     CART_XP


#endif  //_CCAETLADEFS_H_
