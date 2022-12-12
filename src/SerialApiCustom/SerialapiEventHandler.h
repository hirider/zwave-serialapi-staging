#include <time.h>

#include "ZIP_Router.h"
#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif

#include "ZW_controller_api.h"
#include "Serialapi.h"

#include "ZW_controller_bridge_api.h"
#include "ZW_ZIPApplication.h"
#ifdef SAPI_SECURITY
#include "SerialapiSecurity.h"
#endif

#include "Serialapi_sqlite.h"

#define MAX_SIZE (ZW_MAX_NODES/8)
#define PAYLOAD_SINGLE 4
#define PAYLOAD_MULTI 8

#define MAX_SOCK_MSG_LEN 2046

//Global variables

// [0 Not defined] [1 Binary switch] [2 Multilevel Switch] [3 MultiChannel Switch]
volatile static int Flag = 0;

//SerialAPI structs
  // int state;
  // uint16_t desEpid;

typedef struct cmd_class_t{
  BYTE cmd_class;
  BYTE cmd_function;
  BYTE encap_cmd_class;
  BYTE encap_cmd_function;
  bool ismultichannel;
}cmd_class_t;

typedef struct SerApi_ctx_t
{
  BYTE function;
  uint16_t desNodeId;

  cmd_class_t cmd_classes;

  union{
    long int zw_data;
    long int DeviceType;
  };
  union{
    BYTE state;
    BYTE ConfigLen;
  };
  union{
    BYTE desEpid;
    BYTE parameter_number;
  };
} SerApi_ctx_t;

typedef struct SerApi_hex_data_t
{

  char ZwBinSwitchSingle[PAYLOAD_SINGLE];
  char ZwBinSwitchMulti[PAYLOAD_MULTI];
#ifdef SAPI_SECURITY
  char Zwm[22];
#endif
} SerApi_hex_data_t;

typedef struct zwave_ctx_t
{
    BYTE version;
    BYTE capabilities;
    BYTE length;
    BYTE NodeList[MAX_SIZE];
    BYTE ChipType;
    BYTE ChipVersion;
    uint8_t VirtualNodes[MAX_SIZE];
    BYTE PowerLevel;
    BYTE RFRegion;
    BYTE TxPowerLevel;
    BYTE *BackgroundRssi;

} zwave_ctx_t;

typedef struct set_values_4bt_t
{
  BYTE CmdClass;
  BYTE ClassSubFunc;
  BYTE state;
  BYTE duration; 

} set_values_4bt_t;

typedef struct set_values_8bt_t
{
  BYTE PCmdClass;
  BYTE SCmdClass;
  BYTE Epid;
  BYTE SClassSubFunc;
  BYTE PClassSubFunc;
  BYTE state;
  BYTE duration; 
  BYTE undecided;

} set_values_8bt_t;

typedef struct json_package_t
{
  char * status;
  uint16_t Nodeid;
  uint16_t endpoints;
  uint16_t *cc_arr;
  int arr_count;

}json_package_t;

typedef json_package_t db_data_ctx_t;

typedef struct ep_report_t
{
  int nodeid;
  int endpoints;

}ep_report_t;

#ifdef SAPI_SECURITY
typedef struct set_val_18b_t{
  BYTE CmdClass;
  BYTE ClassSubFunc;
  BYTE *ccmArray;
} set_val_18b_t;
#endif

typedef struct network_context_t
{
  int *NetworkInfo_Nodes;
  int NodeCount;

}network_context_t;

network_context_t info;

typedef struct program_state_t
{
  bool Addnode;
  bool Rmnode;

}program_state_t;

typedef enum send_type
{
  SEND_DATA_SINGLE = 1,
  SEND_DATA_MULTICHANNEL,
  SEND_DATA_SECURITY,
  SEND_DATA_MULTINODE,
}send_type;

typedef enum state_t{
  FAILURE,
  SUCCESS,
}state_t;


enum preferences
{
  SAPI_ADD_NODE = 1,
  SAPI_REMOVE_NODE,
  SAPI_SEND_SWITCH,
  SAPI_SEND_LEVEL,
  SAPI_SEND_MULTI_DEVICE,
  SAPI_GET_NODE_INFO,
  SAPI_GET_CONTROLLER_CAP,
  SAPI_SET_LEARN_MODE,
  SAPI_GET_NETWORK_INFO,
  SAPI_ENCRYPT,
  SAPI_BROADCAST_NODE_INFO,
  SAPI_NONCE_GET,
  SAPI_UNIT_TEST,
  SAPI_SOFT_RESET,
  SAPI_HARD_RESET,
  SAPI_CONFIGURE_PARAMETER,
  SAPI_PRIORITY_ROUTE,
  SAPI_NETWORK_UPDATE,
  SAPI_GET_ROUTE,
};

enum FlagDefs
{
  SWITCH,
  ADD_NODE,
  REMOVE_NODE,
  ENDPOINT_REPORT,
};


// utils
/**
 * @brief creates software delay for the given number of seconds
 *
 * @param number_of_seconds
 */
void SAPI_delay(int number_of_seconds);

char* itoa(int val, int base);

/// @brief Main event wrapper for all process in Serialgateway events. appart from a few indepedent processes all other functions are called from here
/// @param input SerApi_ctx_t struct which hold the necessary data for the called process.
void SAPI_ApplicationHandler(SerApi_ctx_t input);

void SAPI_SetPreferences(unsigned long long *SAPIHexArray);


/// @brief adds node to zwave network
/// @param input contains add node information if data bye is set to 1 then this will stop add node process
void SAPI_AddNode(SerApi_ctx_t input);

/// @brief removes node from network
/// @param input contains remove node information if data bye is set to 1 then this will stop remove node process
void SAPI_RemoveNode(SerApi_ctx_t input);

/// @brief sends out packets for a single channel module, wrapper function to SAPI_SendTransport
/// @param input contains node fucntion information
void SAPI_SendDataSingle(SerApi_ctx_t input);

/// @brief sends out packets for a multi channel module, also a wrapper function to SAPI_SendTransport
/// @param input contains node fucntion information
void SAPI_SendDataMChannel(SerApi_ctx_t input);

void SAPI_SendDataMulti();

/// @brief sends a get request right after sending the command, resposible for retriving notifcation
/// @param input contains node fucntion information
void SAPI_GetResponse(SerApi_ctx_t input);

void SAPI_GetResponse_Test(BYTE ComandClass, BYTE SubCommand, uint16_t nid);

/// @brief sends a command to retrive node information from endpoint
/// @param rcc node id for the packet generation
void SAPI_GetNodesInfo(uint16_t rcc);

/// @brief responsible for getting nodeinfo from a function in ZW_ZIPApplication.c
/// @param NodeId Node Id
/// @param Bridge the node data which is passed
/// @param len Bridge length 
void SAPI_NodeInfoBridge(BYTE NodeId, BYTE *Bridge, BYTE len);

/// @brief processes node info and structures it to be sent to zwave gear
/// @param NodeId node Id
/// @param nodeinfo data 
/// @param Len data length
void SAPI_ProcessNode(int NodeId, BYTE * nodeinfo, int Len);

void SAPI_RequestCommandClassInfo(int i);//BYTE CCNodeId);

void SAPI_NodeInfoGetter();

network_context_t SAPI_GetAllNodes(bool display);

void SAPI_SetSendBytesMulti(SerApi_hex_data_t *sb,const set_values_8bt_t *rec);

/// @brief wrapper for Serial API function which sends data via RF
/// @param sb array for creating data packet
/// @param Node node Id
/// @param Datalen data length
/// @param type transmisson type
/// @return true if successful else false
BOOL SAPI_SendTransport(SerApi_hex_data_t sb, uint16_t Node, BYTE Datalen, BYTE type);

/// @brief responsible to change manufacture specific configurations inside the nodes
/// @param Desnode destination node id
/// @param config data frame recieved from zwave gear
/// @param param parameter number
/// @param len data length
void SAPI_ConfigureParameter(BYTE Desnode, long int config, BYTE param, BYTE len);

/// @brief responsible for setting priority route between nodes
/// @param routeinfo nodeid byte array
void SAPI_decodeRoute(long int * routeinfo);

/// @brief responsble to delete all data from the database if the chip is reset. this prevents unique ID conficlt as primary key
/// @return 1 if success 0 if fail
bool SAPI_reset_db(void);

#ifdef SAPI_SECURITY
void SAPI_SetSendBytesMulti2(SerApi_hex_data_t *sb, const set_val_18b_t *rec, const uint8_t *t2enc);
#endif

void SAPI_SetSendBytesSingle(SerApi_hex_data_t *sb,const set_values_4bt_t *rec);

void SAPI_UnitTestZW();

void SAPI_ResponseHandler(ts_param_t *p, ZW_APPLICATION_TX_BUFFER *pCmd,
    WORD cmdLength) CC_REENTRANT_ARG;

char * SAPI_JsonCreate(json_package_t input);

/// @brief sends a data frame requesting endpoint information
/// @param nodeid node id
void get_ep(int nodeid);

//void SAPI_ResponseHandler_ver2(ResponseBridge_t rb) CC_REENTRANT_ARG;

/// @brief writes data to network info table. the data should be added in networkinfo_ctx.write before calling this function
/// @return 1 if success 0 if fail
uint8_t SAPI_DbWriteNetworkInfo();

/// @brief writes data to cmd class info table. the data should be added in cmdclassinfo_ctx.write before calling this function
/// @return 1 if success 0 if fail
uint8_t SAPI_DbWriteCmdClassInfo();

/// @brief updates state, updated_at column in cmdclassinfo table
/// @return 1 if success 0 if fail
uint8_t SAPI_DbUpdateCmdClassInfo();

/// @brief deletes data in cmdclassinfo table. nodeid should be provided in cmdclassinfo_ctx.delete 
/// @return 1 if success 0 if fail
uint8_t SAPI_DbDeleteCmdClassInfo();

int SAPI_DbGetIntAttributes(BYTE attribute, BYTE value);

char * SAPI_DbGetCharAttributes(BYTE attribute);

//DB-APIs

/// @brief wrapper function for all DB write functions. once add or remove node is done this will be called
/// @param endpoints no of endpoints
void Db_Write(BYTE endpoints);

/// @brief writes data to SupportedCmdClass table. the data should be added in supportedcmdclass_ctx.write before calling this function
/// @return 1 if success 0 if fail
uint8_t SAPI_DbWriteSupportedCmdClass();

/// @brief deletes data to a perticular node id. node id should be provided in supportedcmdclass_ctx.delete before calling this function
/// @return 1 if success 0 if fail
uint8_t SAPI_DbDeleteSupportedCmdClass();

/// @brief deletes data to a perticular node id. node id should be provided in networkinfo_ctx.delete before calling this function
/// @return 1 if success 0 if fail
uint8_t SAPI_DbDeleteNetworkInfo();

//Routines

/// @brief called after the add node is completed
/// @param state tells if the add node was successful or not
/// @param rcc node id
void SAPI_PostAddNodeRoutine(BYTE state,int rcc);

void SAPI_GetActionCommand(BYTE cmd_class, BYTE state);

// Callback functions

/// @brief used to call a function after receivng a certain value from response handler
/// @param type enum type to differentiate between functions
/// @param ep endpoints
void RegisterCallback(int type, ep_report_t ep);

/// @brief callback for hardrest 
void CbSetDefault();

/// @brief callback for addnode event
/// @param inf callback data
static void CbAddNodeStatusUpdate(LEARN_INFO *inf);

/// @brief callback for removenode event
/// @param inf callback data
static void CbRemoveNodeStatusUpdate(LEARN_INFO *inf);

/// @brief callback for send single event
/// @param val cb data 1
/// @param val2 cb data 2
static void CbSendDataSingle(BYTE val, TX_STATUS_TYPE *val2);

static void CbSendDataMulti(BYTE val);

/// @brief callback for node info event
/// @param val callback data
static void CbNodeInfo(BYTE val);

static void CbGetResponse(BYTE val, TX_STATUS_TYPE *val2);