#ifndef CONSTANTS_H__
#define CONSTANTS_H__

#define BACKLOG 10 // no of sockets that can be created in one go ...

#define NAME_LEN 255
#define HEADER_LEN 27
#define SHA_DIGEST_LEN 20
#define UOID_LEN 20
#define FILE_BUFF_SIZE 8192

#define SOC_TIME_OUT_SEC  3
#define SOC_TIME_OUT_USEC 0

#define SD_STRQ_N -1	// by cmd_th()
#define SD_KPAV -2		// by keep_alive_th()
#define SD_CKRQ -3		// by dispatch_th()
#define SD_STOR -4		// by cmd_th()
#define SD_SHRQ -5		// by cmd_th()
#define SD_GTRQ -6		// by cmd_th()
#define SD_DEL -7		// by cmd_th()

#ifndef min
#define min(A,B) ((A) > (B) ? (B) : (A))
#endif

const bool DEBUG = false;
const bool DEBUG_DIS_TH = false;
const bool DEBUG_PRO_EV = false;
const bool DEBUG_CMD_WR = false;
const bool DEBUG_PR_EV  = false;
const bool DEBUG_LOGMSG = false;
const bool DEBUG_KILL = false;
const bool DEBUG_DUPMSG = false;
const bool DEBUG_MAIN = false;
const bool LOGFILE_PRINTS_PORT = false;
const bool DEBUG_INDICE = false;
const bool DEBUG_RC1 = false;
const bool DEBUG_RM = false;
const bool DEBUG_GOOD = true;
#endif
