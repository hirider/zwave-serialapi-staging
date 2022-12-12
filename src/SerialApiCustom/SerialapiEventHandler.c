#include "SerialapiEventHandler.h"
#include "SerialapiProcess.h"
#include "Serialapi_routing.h"
#include "ZW_classcmd.h"
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <../cJSON/cJSON.h>

#define MSX_MSG_SIZE 256
#define BUFFER_SIZE (MSX_MSG_SIZE + 10)

#define SINGLE_ENDPOINT 1
#define PRE_CONFIG 4

#define P2
//#define TEST
uint16_t SAPI_SourceId = 0xFF;
BYTE SAPI_TxOption = TRANSMIT_OPTION_ACK;
// int level_flag=1;

#ifdef TEST
SerApi_hex_data_t SendBytes;
#endif

ep_report_t endpoint_val;
db_data_ctx_t DbData;
SerApi_ctx_t GlobalRecieved;

BYTE output;
int SecurityStatusFlag = 0;

program_state_t ps;

pthread_mutex_t flag_mtx = PTHREAD_MUTEX_INITIALIZER;


typedef union networkinfo_ctx_t{

  SAPI_datastore_networkinfo_t write;
  SAPI_datastore_networkinfo_t delete;

}networkinfo_ctx_t;

typedef union{

  SAPI_datastore_cmdclassinfo_t write;
  SAPI_datastore_cmdclassinfo_t update;
  SAPI_datastore_cmdclassinfo_t delete;

}cmdclassinfo_ctx_t;

typedef union{

  SAPI_datastore_supported_cmdclass_t write;
  SAPI_datastore_supported_cmdclass_t delete;

}supported_cmdclass_ctx_t;

networkinfo_ctx_t networkinfo_ctx;
cmdclassinfo_ctx_t cmdclassinfo_ctx;
supported_cmdclass_ctx_t supported_commandclass_ctx;


//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Driver functions ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


void SAPI_ApplicationHandler(SerApi_ctx_t input)
{ 
  GlobalRecieved = input;

  ps.Addnode =  false;
  
  SAPI_LockRoute();

  switch (input.function)
  {
    case SAPI_ADD_NODE:
    {
      SAPI_AddNode(input);
    }
    break;
    case SAPI_REMOVE_NODE:
    {
      SAPI_RemoveNode(input);
    }
    break;
    case SAPI_SEND_SWITCH: case SAPI_SEND_LEVEL:
    {
      input.cmd_classes.ismultichannel == true ? SAPI_SendDataMChannel(input) : SAPI_SendDataSingle(input);
      // level_flag=2;
    }
    break;

#ifndef P2
    case SAPI_SEND_MULTI_DEVICE:
    {
      SAPI_SendDataMulti();
    }
    break;
#endif

    case SAPI_GET_NODE_INFO:
    {
      SAPI_GetNodesInfo(input.desNodeId);
    }
    break;
    case SAPI_GET_CONTROLLER_CAP:
    {
      ZW_GetControllerCapabilities();
    }
    break;

    case SAPI_SET_LEARN_MODE:
    {
      ZW_SetLearnMode(TRUE, NULL);
    }
    break;
    case SAPI_GET_NETWORK_INFO:
    {
      SAPI_GetAllNodes(true);
    }
    break;
#ifdef SAPI_SECURITY
    case SAPI_ENCRYPT:
    {
      SAPISecurityInit(&SAPI_SourceId, &rec.desNodeId,&SAPI_TxOption);
      ProxySecurity();
    }
    break;
#endif
    case SAPI_BROADCAST_NODE_INFO:
    {
      ZW_SendNodeInformation(input.desNodeId, TRANSMIT_OPTION_ACK, NULL);
    }
    break;
#ifdef SAPI_SECURITY
    case SAPI_NONCE_GET:
    {
      SAPISecurityInit(&SAPI_SourceId, &rec.desNodeId,&SAPI_TxOption);
      GetNonce();
    }
    break;
#endif
#ifdef TEST
    case SAPI_UNIT_TEST:
    {
      SAPI_UnitTestZW();
    }
    break;
#endif   
    case SAPI_SOFT_RESET:
    {
      LOG_PRINTF("Soft-resetting controller\n");
      void ZW_SoftReset();
    }
    break;

    case SAPI_HARD_RESET:
    {
      LOG_PRINTF("Removing network information..\n");
      LOG_PRINTF("Hard-resetting controller\n");
      ZW_SetDefault(CbSetDefault);

      if(SAPI_reset_db()){
        LOG_PRINTF("DB reset successfull\n");
      }
    }
    break;

    case SAPI_CONFIGURE_PARAMETER:
    {
      SAPI_ConfigureParameter(input.desNodeId,input.zw_data,input.parameter_number,input.ConfigLen);
    }
    break;

    case SAPI_PRIORITY_ROUTE:
    {
      SAPI_PriorityRoute(&input.zw_data,input.desNodeId);
    }
    break;

    case SAPI_NETWORK_UPDATE:
    {
      if(ZW_RequestNetWorkUpdate(NULL)){
        LOG_PRINTF("network update started..\n");
      }
      else{
        LOG_PRINTF("network update failed!\n");
      }
    }
    break;

    case SAPI_GET_ROUTE:
    {
      SAPI_GetPriorityRoute(input.desNodeId);
    }
    break;

    default:
      printf("\nInvalid selection\n");
  }
}

void
SAPI_ResponseHandler(ts_param_t *p, ZW_APPLICATION_TX_BUFFER *pCmd,
    WORD cmdLength) CC_REENTRANT_ARG
{
  SAPI_datastore_supported_cmdclass_t * val;

  switch(Flag)
  {
    char SendBuff[MAX_SOCK_MSG_LEN];

    case SWITCH:
    { 
      switch(pCmd->ZW_Common.cmdClass)
      {
        case COMMAND_CLASS_SWITCH_BINARY:

          val = SAPI_DbQuerySupportedCmdClass(p->snode,COMMAND_CLASS_MULTI_CHANNEL_V2);

          if(val->cmd_class == COMMAND_CLASS_MULTI_CHANNEL_V2)
          {
            SAPI_data_mem_free(val);
            break;
          }
          else
          {
            if(p->sendpoint > 0 && p->sendpoint < 255){
              LOG_PRINTF("Binary switch node id = %d, endpoint =  %d, state = %d\n",\
              p->snode,p->sendpoint,pCmd->ZW_SwitchBinaryReportV2Frame.currentValue);

              snprintf(SendBuff,sizeof(SendBuff),"cb:bin_sw,%u,%u,%u",\
              p->snode,p->sendpoint, pCmd->ZW_SwitchBinaryReportV2Frame.currentValue);

              Sapi2Zwave_send(SendBuff);

              cmdclassinfo_ctx.update.nodeid = p->snode;
              cmdclassinfo_ctx.update.endpoint = p->sendpoint;
              cmdclassinfo_ctx.update.state = pCmd->ZW_SwitchBinaryReportV2Frame.currentValue;
              SAPI_DbUpdateCmdClassInfo();
            }
            else{
              LOG_PRINTF("Binary switch node id = %d, endpoint =  %d, state = %d\n",\
              p->snode,GlobalRecieved.desEpid,pCmd->ZW_SwitchBinaryReportV2Frame.currentValue);

              snprintf(SendBuff,sizeof(SendBuff),"cb:bin_sw,%u,%u,%u",\
              p->snode,GlobalRecieved.desEpid, pCmd->ZW_SwitchBinaryReportV2Frame.currentValue);

              Sapi2Zwave_send(SendBuff);

              cmdclassinfo_ctx.update.nodeid = p->snode;
              cmdclassinfo_ctx.update.endpoint = GlobalRecieved.desEpid;
              cmdclassinfo_ctx.update.state = pCmd->ZW_SwitchBinaryReportV2Frame.currentValue;
              SAPI_DbUpdateCmdClassInfo();
            }
            SAPI_data_mem_free(val);
          }
        break;
  
        case COMMAND_CLASS_SWITCH_MULTILEVEL:

          if(p->sendpoint > 0 && p->sendpoint < 255)
          {
            LOG_PRINTF("Multilevel switch node id = %d, endpoint = %d, state = %d\n",\
            p->snode,p->sendpoint,pCmd->ZW_SwitchMultilevelReportFrame.value);

            snprintf(SendBuff,sizeof(SendBuff),"cb:ml_sw,%u,%u,%u",\
            p->snode,p->sendpoint,pCmd->ZW_SwitchMultilevelReportFrame.value);

            Sapi2Zwave_send(SendBuff);

            cmdclassinfo_ctx.update.nodeid = p->snode;
            cmdclassinfo_ctx.update.endpoint = p->sendpoint;
            cmdclassinfo_ctx.update.state = pCmd->ZW_SwitchMultilevelReportFrame.value;
            SAPI_DbUpdateCmdClassInfo();
          }
          else
          {
            LOG_PRINTF("Multilevel switch node id = %d, endpoint = %d, state = %d\n",\
            p->snode,GlobalRecieved.desEpid,pCmd->ZW_SwitchMultilevelReportFrame.value);

            snprintf(SendBuff,sizeof(SendBuff),"cb:ml_sw,%u,%u,%u",\
            p->snode,GlobalRecieved.desEpid,pCmd->ZW_SwitchMultilevelReportFrame.value);

            Sapi2Zwave_send(SendBuff);

            cmdclassinfo_ctx.update.nodeid = p->snode;
            cmdclassinfo_ctx.update.endpoint = GlobalRecieved.desEpid;
            cmdclassinfo_ctx.update.state = pCmd->ZW_SwitchMultilevelReportFrame.value;
            SAPI_DbUpdateCmdClassInfo();
          }
        break;
  
        case COMMAND_CLASS_MULTI_CHANNEL_V2:

          if(pCmd->ZW_MultiChannelCmdEncapV2Frame.properties1 > 0 && pCmd->ZW_MultiChannelCmdEncapV2Frame.properties1 < 255)
          {
            LOG_PRINTF("Binary switch node id = %d, endpoint =  %d, state = %d\n",\
            p->snode,pCmd->ZW_MultiChannelCmdEncapV2Frame.properties1 ,pCmd->ZW_MultiChannelCmdEncapV2Frame.encapFrame.ZW_SwitchBinaryReportV2Frame.targetValue);
  
            snprintf(SendBuff,sizeof(SendBuff),"cb:bin_sw,%u,%u,%u",\
            p->snode,pCmd->ZW_MultiChannelCmdEncapV2Frame.properties1 ,pCmd->ZW_MultiChannelCmdEncapV2Frame.encapFrame.ZW_SwitchBinaryReportV2Frame.targetValue);

            Sapi2Zwave_send(SendBuff);

            cmdclassinfo_ctx.update.nodeid = p->snode;
            cmdclassinfo_ctx.update.endpoint = pCmd->ZW_MultiChannelCmdEncapV2Frame.properties1;
            cmdclassinfo_ctx.update.state = pCmd->ZW_MultiChannelCmdEncapV2Frame.encapFrame.ZW_SwitchBinaryReportV2Frame.targetValue;
            SAPI_DbUpdateCmdClassInfo();
          }
          else
          {
            LOG_PRINTF("Binary switch node id = %d, endpoint =  %d, state = %d\n",\
            p->snode,GlobalRecieved.desEpid,pCmd->ZW_MultiChannelCmdEncapV2Frame.encapFrame.ZW_SwitchBinaryReportV2Frame.targetValue);
  
            snprintf(SendBuff,sizeof(SendBuff),"cb:bin_sw,%u,%u,%u",\
            p->snode,GlobalRecieved.desEpid,pCmd->ZW_MultiChannelCmdEncapV2Frame.encapFrame.ZW_SwitchBinaryReportV2Frame.targetValue);

            Sapi2Zwave_send(SendBuff);

            cmdclassinfo_ctx.update.nodeid = p->snode;
            cmdclassinfo_ctx.update.endpoint = GlobalRecieved.desEpid;
            cmdclassinfo_ctx.update.state = pCmd->ZW_MultiChannelCmdEncapV2Frame.encapFrame.ZW_SwitchBinaryReportV2Frame.targetValue;
            SAPI_DbUpdateCmdClassInfo();
          }

        break;
      }
    }
    break;

    case ENDPOINT_REPORT:
      endpoint_val.nodeid = p->snode;
      endpoint_val.endpoints = pCmd->ZW_MultiChannelEndPointReportV2Frame.properties2;
      LOG_PRINTF("Node has endpoints = %d\n",pCmd->ZW_MultiChannelEndPointReportV2Frame.properties2);
      RegisterCallback(ENDPOINT_REPORT,endpoint_val);
    break;

    default:
      Flag = 0;

  }
  pthread_mutex_lock(&flag_mtx);
  Flag = 0;
  pthread_mutex_unlock(&flag_mtx);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// helper functions ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
void SAPI_AddNode(SerApi_ctx_t input)
{ 
  ps.Addnode = true;

  if(input.zw_data == 0){
    LOG_PRINTF("Add Node Started\n");

    ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);

    ZW_AddNodeToNetwork(ADD_NODE_ANY | ADD_NODE_OPTION_NETWORK_WIDE, CbAddNodeStatusUpdate);
  }
  else{
    LOG_PRINTF("Stopping add node process\n");

    ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
  }
}

void SAPI_RemoveNode(SerApi_ctx_t input)
{
  if(input.zw_data == 0){
    LOG_PRINTF("Remove Node Started\n");

    ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, NULL);

    ZW_RemoveNodeFromNetwork(REMOVE_NODE_ANY, CbRemoveNodeStatusUpdate);
  }
  else{
    LOG_PRINTF("Stopping remove node process\n");

    ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, NULL);
  }
}

void SAPI_SendDataSingle(SerApi_ctx_t input)
{ 
  if(input.function == SAPI_SEND_LEVEL)
  { 
    LOG_PRINTF("LEVEL/CURTAIN Initiated\n");
    SerApi_hex_data_t sb;
    SerApi_ctx_t Resp;
    Resp.desNodeId=input.desNodeId;
    memset(sb.ZwBinSwitchSingle,0,sizeof(sb.ZwBinSwitchSingle));
    sb.ZwBinSwitchSingle[0] = input.cmd_classes.cmd_class;
    sb.ZwBinSwitchSingle[1] = input.cmd_classes.cmd_function;
    sb.ZwBinSwitchSingle[2] = input.state;
    sb.ZwBinSwitchSingle[3] = 0x20;

    BYTE datalen = sizeof(sb.ZwBinSwitchSingle);

    if(SAPI_SendTransport(sb,input.desNodeId, datalen,SEND_DATA_SINGLE)){
        Resp.cmd_classes.cmd_class = COMMAND_CLASS_SWITCH_MULTILEVEL;
        Resp.cmd_classes.cmd_function = SWITCH_MULTILEVEL_GET;
        SAPI_GetResponse(Resp);
    }
  }

  else if((input.function == SAPI_SEND_SWITCH)){
    SerApi_hex_data_t sb;
    SerApi_ctx_t Resp;

    Resp.desNodeId = input.desNodeId;
    memset(sb.ZwBinSwitchSingle,0,sizeof(sb.ZwBinSwitchSingle));

    sb.ZwBinSwitchSingle[0] = input.cmd_classes.cmd_class;
    sb.ZwBinSwitchSingle[1] = input.cmd_classes.cmd_function;
    sb.ZwBinSwitchSingle[2] = input.state;
    sb.ZwBinSwitchSingle[3] = 0;

    BYTE datalen = sizeof(sb.ZwBinSwitchSingle);

    if(SAPI_SendTransport(sb,input.desNodeId, datalen,SEND_DATA_SINGLE)){
        Resp.cmd_classes.cmd_class = COMMAND_CLASS_SWITCH_BINARY;
        Resp.cmd_classes.cmd_function = SWITCH_BINARY_GET;
        SAPI_GetResponse(Resp);
    }
  }
}

void SAPI_SendDataMChannel(SerApi_ctx_t input)
{ 

  if(input.function == SAPI_SEND_LEVEL)
  { 
    LOG_PRINTF("LEVEL/CURTAIN Initiated\n");
    SerApi_hex_data_t sb;
    SerApi_ctx_t Resp;
    Resp.desNodeId=input.desNodeId;
    memset(sb.ZwBinSwitchMulti,0,sizeof(sb.ZwBinSwitchMulti));
    sb.ZwBinSwitchMulti[0] = input.cmd_classes.encap_cmd_class;
    sb.ZwBinSwitchMulti[1] = input.cmd_classes.encap_cmd_function;
    sb.ZwBinSwitchMulti[2] = 0x00;
    sb.ZwBinSwitchMulti[3] = input.desEpid;
    sb.ZwBinSwitchMulti[4] = input.cmd_classes.cmd_class;
    sb.ZwBinSwitchMulti[5] = input.cmd_classes.cmd_function;
    sb.ZwBinSwitchMulti[6] = input.state;
    sb.ZwBinSwitchMulti[7] = 0x20;
    


    BYTE datalen = sizeof(sb.ZwBinSwitchMulti);

    if(SAPI_SendTransport(sb,input.desNodeId, datalen,SEND_DATA_MULTICHANNEL)){
        Resp.cmd_classes.cmd_class = COMMAND_CLASS_SWITCH_MULTILEVEL;
        Resp.cmd_classes.cmd_function = SWITCH_MULTILEVEL_GET;
        SAPI_GetResponse(Resp);
      }
    
  }
  else if(input.function == SAPI_SEND_SWITCH)
  {
    SerApi_hex_data_t sb;
    SerApi_ctx_t Resp;

    Resp.desNodeId = input.desNodeId;
    memset(sb.ZwBinSwitchMulti,0,sizeof(sb.ZwBinSwitchMulti));

    sb.ZwBinSwitchMulti[0] = input.cmd_classes.encap_cmd_class;
    sb.ZwBinSwitchMulti[1] = input.cmd_classes.encap_cmd_function;
    sb.ZwBinSwitchMulti[2] = 0x00;
    sb.ZwBinSwitchMulti[3] = input.desEpid;
    sb.ZwBinSwitchMulti[4] = input.cmd_classes.cmd_class;
    sb.ZwBinSwitchMulti[5] = input.cmd_classes.cmd_function;
    sb.ZwBinSwitchMulti[6] = input.state;
    sb.ZwBinSwitchMulti[7] = 0x00;

    BYTE datalen = sizeof(sb.ZwBinSwitchMulti);

    if(SAPI_SendTransport(sb,input.desNodeId, datalen,SEND_DATA_MULTICHANNEL)){
      Resp.cmd_classes.cmd_class = COMMAND_CLASS_SWITCH_BINARY;
      Resp.cmd_classes.cmd_function = SWITCH_BINARY_GET;
      SAPI_GetResponse(Resp);
    }
  }  
}

void SAPI_GetResponse(SerApi_ctx_t input)
{

  SerApi_hex_data_t sb;
  memset(sb.ZwBinSwitchSingle,0,sizeof(sb.ZwBinSwitchSingle));

  sb.ZwBinSwitchSingle[0] = input.cmd_classes.cmd_class;
  sb.ZwBinSwitchSingle[1] = input.cmd_classes.cmd_function;
  sb.ZwBinSwitchSingle[2] = 0;
  sb.ZwBinSwitchSingle[3] = 0;

  BYTE datalen = sizeof(sb.ZwBinSwitchSingle);

  if (!ZW_SendData_Bridge(SAPI_SourceId, input.desNodeId, sb.ZwBinSwitchSingle, datalen, SAPI_TxOption, NULL)){
    LOG_PRINTF("failed to send response\n");
  }
}

#ifdef TEST
void SAPI_GetResponse_Test(BYTE ComandClass, BYTE SubCommand, uint16_t nid)
{
  set_values_4bt_t *ssg = NULL;
  SerApi_hex_data_t *sb = NULL;
  ssg = (set_values_4bt_t*)malloc(sizeof(set_values_4bt_t));
  sb = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
  ssg->CmdClass = ComandClass;
  ssg->ClassSubFunc = SubCommand;
  SAPI_SetSendBytesSingle(sb,ssg);
  BYTE datalen = sizeof(SendBytes.ZwBinSwitchSingle);

  if (!ZW_SendData_Bridge(SAPI_SourceId, nid, sb->ZwBinSwitchSingle, datalen, SAPI_TxOption, NULL)){
    LOG_PRINTF("failed to send command\n");
  }
  free(ssg);
  free(sb);
}
#endif

#ifndef P2
void SAPI_SendDataMulti()
{
  nodemask_t nodeMask_new;
  //DataPayload_new will be removed. define a local variable when this block is activated
  BYTE *DataPayload_new;
  //datalen_new will be removed. define a local variable using sizeof() when this block is activated
  BYTE datalen_new = 0x0F;
  int bufferSize = ZW_MAX_NODES / 8 + 1;
  char data_buffer[PAYLOAD_SINGLE];
  for (int i = 0; i < PAYLOAD_SINGLE; i++)
  {
    data_buffer[i] = SendBytes.ZwBinSwitchSingle[i];
  }
  DataPayload_new = data_buffer;
  memset(nodeMask_new, 0, bufferSize * sizeof(nodeMask_new[0]));
  if (nodemask_add_node(RecievedBytes.desNodeId, nodeMask_new))
  {
    printf("nodemask set");
  }
  else
  {
    printf("nodemask not set");
  }
  printf("\n");
  for (int i = 0; i < 30; i++)
  {
    printf("%d ", nodeMask_new[i]);
  }
  printf("\n");
  if (SerialAPI_ZW_SendDataMulti_Bridge(SAPI_SourceId, nodeMask_new, DataPayload_new, datalen_new, SAPI_TxOption, CbSendDataMulti))
  {
    printf("Successfull\n");
  }
  else
  {
    printf("failed\n");
  }
}
#endif

BOOL SAPI_SendTransport(SerApi_hex_data_t sb, uint16_t Node, BYTE Datalen, BYTE type){
  switch(type){
    case SEND_DATA_SINGLE:
    {
      if (!ZW_SendData_Bridge(SAPI_SourceId, Node, sb.ZwBinSwitchSingle, Datalen, SAPI_TxOption, CbSendDataSingle)){
          LOG_PRINTF("failed\n");
          return false;       
      }

      return true;
    }
    break;

    case SEND_DATA_MULTICHANNEL:
    { 
      if (!ZW_SendData_Bridge(SAPI_SourceId, Node, sb.ZwBinSwitchMulti, Datalen, SAPI_TxOption, CbSendDataSingle)){
          LOG_PRINTF("failed\n");
          return false;       
      }

      return true;
    }
    break;

    default:
    {
      printf("Feature not enabled\n");
      return false;
    }
  }
}

void SAPI_GetNodesInfo(uint16_t rcc)
{
  ZW_RequestNodeInfo(rcc, CbNodeInfo);
}

void SAPI_NodeInfoBridge(BYTE NodeId, BYTE *Bridge, BYTE len)
{
    if(ps.Addnode == true){
       SAPI_ProcessNode(NodeId, Bridge, len);
    }
    ps.Addnode = false;
}

void SAPI_ProcessNode(int NodeId, BYTE * nodeinfo, int Len){

  printf("\n****** Node Info ******\n");
  printf("Node Id = %d\t Node Length = %d\n",NodeId,Len);
  printf("Supported Command Class\n");
  bool ismultichannel = false;

  char buff[MAX_SOCK_MSG_LEN];
  char buff2[MAX_SOCK_MSG_LEN / 2];
  char temp[10];

  DbData.arr_count = Len;
  DbData.Nodeid = NodeId;
  DbData.status = "success";
  DbData.cc_arr = (uint16_t*)calloc(Len,sizeof(uint16_t));

  memset(buff2,0,sizeof(buff));
  printf("[");
  for(int i = 0; i < Len; i++)
  {
    DbData.cc_arr[i] = nodeinfo[i];

    if(nodeinfo[i] == 0x60){
      ismultichannel = true;
    }

    memset(temp,0,sizeof(temp));
    i != Len-1 ? sprintf(temp,"%d,",nodeinfo[i]) : sprintf(temp,"%d",nodeinfo[i]);
    strncat(buff2,temp,sizeof(buff2) - strlen(buff2) - 1);

    printf("%x ",nodeinfo[i]);
  }
  printf("]\n\n");

  info = SAPI_GetAllNodes(false);

  if(ismultichannel == true){
    get_ep(NodeId);
  }
  else{
    sprintf(buff,"nodeinfo,%d,%d,%d,",DbData.arr_count,DbData.Nodeid,0);
    strncat(buff,buff2,sizeof(buff) - strlen(buff) - 1);
    printf("%s\n",buff);
    Sapi2Zwave_send(buff);
    Db_Write(SINGLE_ENDPOINT);

    free(DbData.cc_arr);
  }
}

network_context_t SAPI_GetAllNodes(bool display)
{
  int N_index = 0, nodeCount = 0;
  zwave_ctx_t GetZwData;
  network_context_t info;

  SerialAPI_GetInitData(&GetZwData.version, &GetZwData.capabilities,
                        &GetZwData.length, GetZwData.NodeList, &GetZwData.ChipType, &GetZwData.ChipVersion);


  for (int i = 0; i < 232; i++)
  {
    if (nodemask_test_node(i, GetZwData.NodeList))
    {
      nodeCount++;
    }
  }

  info.NodeCount = nodeCount;

  info.NetworkInfo_Nodes = (int*)calloc(nodeCount,sizeof(int));

  for (int i = 0; i < 232; i++)
  {
    if (nodemask_test_node(i, GetZwData.NodeList))
    {
      info.NetworkInfo_Nodes[N_index] = i;
      N_index++;
    }
  }
  
  GetZwData.RFRegion = ZW_RFRegionGet();

  if (display == true){
    printf("Version : %d Chip Type : %d  Chip Version : %d  Data Length : %d  Capabilities : %d\n\n",\
    GetZwData.version, GetZwData.ChipType, GetZwData.ChipVersion, GetZwData.length, GetZwData.capabilities);

    printf("Node Ids :");
    for (int i = 0; i < nodeCount; i++)
    {
      printf(" %d ",info.NetworkInfo_Nodes[i]);
    }

    printf("\nRF Region = %d",GetZwData.RFRegion);
  }
  
  return info;
}

char * SAPI_JsonCreate(json_package_t input){

  char *string = NULL;
  cJSON *cc = NULL;
  cJSON *monitor = cJSON_CreateObject();
  cJSON *endpoint_list = NULL;
  cJSON *endpoints = NULL;

  if (cJSON_AddStringToObject(monitor, "status", input.status) == NULL)
  {
    goto end;
  }

  if (cJSON_AddNumberToObject(monitor, "node_id", input.Nodeid) == NULL)
  {
    goto end;
  }
    
  endpoints = cJSON_AddObjectToObject(monitor,"endpoints");
    
  
  for(int i = 0; i < input.endpoints; i++){

    cJSON *abb; // = cJSON_CreateArray();

    char * endpoint_name = NULL;
    if(i == 0)
      endpoint_name = "0";
    else
      endpoint_name = itoa(i,10);

    endpoint_list = cJSON_AddObjectToObject(endpoints,endpoint_name);

    abb = cJSON_AddArrayToObject(endpoint_list, "command_classes");

    for (int index = 0; index < input.arr_count; ++index)
    {
      cJSON *num = cJSON_CreateNumber(input.cc_arr[index]);

      if(cJSON_AddItemToArray(abb,num) == false){
        goto end;
      }
    }
    
    if (abb == NULL)
    {
        goto end;
    }
  }

  string = cJSON_Print(monitor);
  if (string == NULL)
  {
    fprintf(stderr, "Failed to print monitor.\n");
  }

  end:
    cJSON_Delete(monitor);
    return string;
}

void SAPI_ConfigureParameter(BYTE Desnode, long int config, BYTE param, BYTE len){

  BYTE config_buff[10];
  BYTE buff_len = 0;

  config_buff[0] = COMMAND_CLASS_CONFIGURATION_V3;
  config_buff[1] = CONFIGURATION_SET_V3;
  config_buff[2] = param;
  config_buff[3] = len;

  if(len == 1){
    config_buff[4] = config;
    buff_len = PRE_CONFIG + len;
  }
  else{   
    int size = len-1;

    for(int i = 0; i <= size; i++){
      config_buff[size + PRE_CONFIG - i] = ((config >> i*8) & 0xFF);
    }
    buff_len = PRE_CONFIG + len;
  }

  if(ZW_SendData_Bridge(SAPI_SourceId,Desnode,config_buff,buff_len,SAPI_TxOption,NULL)){
    LOG_PRINTF("Configuration %lx set for node  %d\n",config, Desnode);
  }

}
// WARNING! This function will delete all data stored in serialgateway local database.
bool SAPI_reset_db(void){
  if(SAPI_datareset_networkinfo() && SAPI_datareset_cmdclassinfo() && SAPI_datareset_supportedcmdclass())
  {
    return 1;
  }
  return 0;
}

  //////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// DB-APIs /////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t SAPI_DbWriteNetworkInfo(){

  return SAPI_data_store_nvm_write_networkinfo(&networkinfo_ctx.write);

}

uint8_t SAPI_DbWriteCmdClassInfo(){

  return SAPI_data_store_nvm_write_cmdclassinfo(&cmdclassinfo_ctx.write);

}

uint8_t SAPI_DbWriteSupportedCmdClass(){
  return SAPI_data_store_nvm_write_supportedcmdclass(&supported_commandclass_ctx.write);
}

uint8_t SAPI_DbUpdateCmdClassInfo(){

  return SAPI_data_store_nvm_update_cci(&cmdclassinfo_ctx.update);

}

uint8_t SAPI_DbDeleteCmdClassInfo(){

  return SAPI_data_store_nvm_delete_cci(&cmdclassinfo_ctx.delete);

}

uint8_t SAPI_DbDeleteSupportedCmdClass(){

  return SAPI_data_store_nvm_delete_supportedcmdclass(&supported_commandclass_ctx.delete);
}

uint8_t SAPI_DbDeleteNetworkInfo(){

  return SAPI_data_store_nvm_delete_networkinfo(&networkinfo_ctx.delete);
}

int SAPI_DbGetIntAttributes(BYTE attribute, BYTE value){

TX_POWER_LEVEL ret;

  switch(attribute){
    case ni_RfRegion:
      return ZW_RFRegionGet();
      break;

    case ni_Txpower:
      ret = ZW_TXPowerLevelGet();
      return ret.measured0dBm;
      break;

    default:
      return -1;
  }
}

  //////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// Utilities ///////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////


void SAPI_delay(int number_of_seconds)
{
  // Converting time into milli_seconds
  int milli_seconds = 1000 * number_of_seconds;

  // Storing start time
  clock_t start_time = clock();

  // looping till required time is not achieved
  while (clock() < start_time + milli_seconds)
    ;
}

#ifdef TEST
void SAPI_SetSendBytesSingle(SerApi_hex_data_t *sb, const set_values_4bt_t *rec)
{
    sb->ZwBinSwitchSingle[0] = rec->CmdClass;
    sb->ZwBinSwitchSingle[1] = rec->ClassSubFunc;
    sb->ZwBinSwitchSingle[2] = rec->state;
    sb->ZwBinSwitchSingle[3] = rec->duration;
}

void SAPI_SetSendBytesMulti(SerApi_hex_data_t *sb, const set_values_8bt_t *rec)
{
    sb->ZwBinSwitchMulti[0] = rec->PCmdClass;
    sb->ZwBinSwitchMulti[1] = rec->PClassSubFunc;
    sb->ZwBinSwitchMulti[2] = rec->undecided;
    sb->ZwBinSwitchMulti[3] = rec->Epid;
    sb->ZwBinSwitchMulti[4] = rec->SCmdClass;
    sb->ZwBinSwitchMulti[5] = rec->SClassSubFunc;
    sb->ZwBinSwitchMulti[6] = rec->state;
    sb->ZwBinSwitchMulti[7] = rec->duration;
}
#endif

#ifdef SAPI_SECURITY
void SAPI_SetSendBytesMulti2(SerApi_hex_data_t *sb, const set_val_18b_t *rec, const uint8_t *t2enc)
{
    sb->Zwm[0] = rec->CmdClass;
    sb->Zwm[1] = rec->ClassSubFunc;
    sb->Zwm[2] = 0x75;
    for(int i = 0; i < 16; i ++){
      SendBytes.Zwm[i+3] = t2enc[i];
    }
}
#endif

void get_ep(int nodeid){

  SerApi_ctx_t ep_ctx;
  ep_ctx.cmd_classes.cmd_class = COMMAND_CLASS_MULTI_CHANNEL_V2;
  ep_ctx.cmd_classes.cmd_function = MULTI_CHANNEL_END_POINT_GET_V2;
  ep_ctx.desNodeId = nodeid;
  SAPI_GetResponse(ep_ctx);
  Flag = 3;
  LOG_PRINTF("fetching endpoints..\n");
}

char* itoa(int val, int base){
	
	static char buf[32] = {0};
	
	int i = 30;
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}

void Db_Write(BYTE endpoints){

    networkinfo_ctx.write.nodeid = DbData.Nodeid;
    networkinfo_ctx.write.EndpointCount = endpoints;
    networkinfo_ctx.write.RfRegion = SAPI_DbGetIntAttributes(ni_RfRegion,0);
    networkinfo_ctx.write.Txpower = SAPI_DbGetIntAttributes(ni_Txpower,0);

    for(int i = 1; i <= endpoints; i++){
      cmdclassinfo_ctx.write.nodeid = DbData.Nodeid;
      cmdclassinfo_ctx.write.endpoint = i;
      cmdclassinfo_ctx.write.cmd_class = "placeholder1";
      cmdclassinfo_ctx.write.cmd_class_len = 13;
      cmdclassinfo_ctx.write.role_type = "placeholder2";
      cmdclassinfo_ctx.write.role_type_len = 13;
      cmdclassinfo_ctx.write.node_type = "placeholder3";
      cmdclassinfo_ctx.write.node_type_len = 13;

      SAPI_DbWriteCmdClassInfo();

    }

    for(int i = 0; i < DbData.arr_count; i++){

      supported_commandclass_ctx.write.nodeid = DbData.Nodeid;
      supported_commandclass_ctx.write.cmd_class = DbData.cc_arr[i];

      SAPI_DbWriteSupportedCmdClass();
        
    }

    if(SAPI_DbWriteNetworkInfo()){
      LOG_PRINTF("data written to database\n");
    }
}
  //////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// Routines ////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////

int nodeadded = 0;

void SAPI_PostAddNodeRoutine(BYTE state, int RecNode){
  char buff[MAX_SOCK_MSG_LEN];

  if(state == SUCCESS && RecNode != -1){

    SAPI_GetNodesInfo(RecNode);

  }
  else if(state == FAILURE && RecNode == -1){
      Sapi2Zwave_send("failure");
    }
}

int noderemoved = 0;

void SAPI_PostRemoveNodeRoutine(BYTE state, int RecNode){
  char buff[MAX_SOCK_MSG_LEN];

  if(state == SUCCESS && RecNode != -1){
    snprintf(buff,sizeof(buff),"success\n%d",RecNode);
    Sapi2Zwave_send(buff);

    networkinfo_ctx.delete.nodeid = RecNode;
    SAPI_DbDeleteNetworkInfo();

    cmdclassinfo_ctx.delete.nodeid = RecNode;
    SAPI_DbDeleteCmdClassInfo();

    supported_commandclass_ctx.delete.nodeid = RecNode;
    SAPI_DbDeleteSupportedCmdClass();
  }
  else if(state == FAILURE && RecNode == -1){
      Sapi2Zwave_send("failure");
    }
}
  //////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////// Callback Functions ///////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////

void RegisterCallback(int type, ep_report_t ep){

  char buff[MAX_SOCK_MSG_LEN];

  if(type == ENDPOINT_REPORT && ep.endpoints > 0 && ep.nodeid == DbData.Nodeid){

    sprintf(buff,"nodeinfo,%d,%d,%d,",DbData.arr_count,DbData.Nodeid,ep.endpoints);

    char temp[10];

    for(int i = 0; i < DbData.arr_count; i++){

      i != DbData.arr_count-1 ? sprintf(temp,"%d,",DbData.cc_arr[i]) : sprintf(temp,"%d",DbData.cc_arr[i]);
      strncat(buff,temp,sizeof(buff) - strlen(buff) - 1);

    }
    Sapi2Zwave_send(buff);

    Db_Write(ep.endpoints);

    free(DbData.cc_arr);
  }

  else if(type == ENDPOINT_REPORT && ep.endpoints == 0){

    sprintf(buff,"nodeinfo,%d,%d,%d,",DbData.arr_count,DbData.Nodeid,0);

    char temp[10];

    for(int i = 0; i < DbData.arr_count; i++){

      i != DbData.arr_count-1 ? sprintf(temp,"%d,",DbData.cc_arr[i]) : sprintf(temp,"%d",DbData.cc_arr[i]);
      strncat(buff,temp,sizeof(buff) - strlen(buff) - 1);
    }
    Sapi2Zwave_send(buff);

    Db_Write(SINGLE_ENDPOINT);

    free(DbData.cc_arr);
  }

}

int CbAddedNode;
int CbRemovedNode;

static void CbAddNodeStatusUpdate(LEARN_INFO *inf)
{
  printf("AddNodeStatusUpdate status=%d info len %d\n", inf->bStatus, inf->bLen);

  switch (inf->bStatus)
  {
    case ADD_NODE_STATUS_LEARN_READY:
      break;

    case ADD_NODE_STATUS_NODE_FOUND:
      break;

    case ADD_NODE_STATUS_ADDING_SLAVE:
    case ADD_NODE_STATUS_ADDING_CONTROLLER:
      if (inf->bLen)
      {
        CbAddedNode = inf->bSource;
        LOG_PRINTF("Node added with nodeid %d\n", inf->bSource);
      }
      break;

    case ADD_NODE_STATUS_PROTOCOL_DONE:
      ZW_AddNodeToNetwork(ADD_NODE_STOP, CbAddNodeStatusUpdate);
      printf("add node service stopped\n");
      break;

    case ADD_NODE_STATUS_DONE:
      LOG_PRINTF("Add Node Successful\n");
      SAPI_PostAddNodeRoutine(SUCCESS,CbAddedNode);
      break;

    case ADD_NODE_STATUS_FAILED:
      ERR_PRINTF("Add Node Failed\n");
      SAPI_PostAddNodeRoutine(FAILURE,-1);
      break;

    case ADD_NODE_STATUS_NOT_PRIMARY:
      break;
  }
}

static void CbRemoveNodeStatusUpdate(LEARN_INFO *inf)
{
  printf("RemoveNodeStatusUpdate status=%d\n", inf->bStatus);
  switch (inf->bStatus)
  {
    case REMOVE_NODE_STATUS_LEARN_READY:
      break;

    case REMOVE_NODE_STATUS_NODE_FOUND:
      break;

    case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:  
    case REMOVE_NODE_STATUS_REMOVING_SLAVE:
      if(inf->bLen){
        CbRemovedNode = inf->bSource;
      }
      break;

    case REMOVE_NODE_STATUS_DONE:
      LOG_PRINTF("Removed Node %d\n",CbRemovedNode);
      SAPI_PostRemoveNodeRoutine(SUCCESS,CbRemovedNode);
      break;

    case REMOVE_NODE_STATUS_FAILED:
      ERR_PRINTF("Remove Node Failed\n");
      SAPI_PostRemoveNodeRoutine(FAILURE,-1);
      break;
  }
}

void CbSetDefault(){
    LOG_PRINTF("Controller reset complete!\n");
}

static void CbSendDataSingle(BYTE val, TX_STATUS_TYPE *val2)
{
}

static void CbSendDataMulti(BYTE val)
{
}

static void CbLearnModeUpdate(LEARN_INFO *inf)
{
}

static void CbNodeInfo(BYTE val)
{
}

#ifdef TEST
void SAPI_UnitTestZW(){
  uint16_t LocalMultichannelNode = 0x69;
  uint16_t LocalMultiLevelNode = 0x4F;
  uint16_t LocalBinaryNode = 0x64;
  for(int val = 0; val < 3; val++){

    ///////////////////////////////// multichannel test //////////////////////////////

    for(uint16_t i = LocalMultichannelNode; i<= LocalMultichannelNode+1; i++){
      for(uint16_t j = 1; j < 5; j++){
      set_values_8bt_t *ssg = NULL;
      SerApi_hex_data_t *sb = NULL;
      ssg = (set_values_8bt_t*)malloc(sizeof(set_values_8bt_t));
      sb = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
      ssg->PCmdClass = COMMAND_CLASS_MULTI_CHANNEL_V2;
      ssg->PClassSubFunc = MULTI_CHANNEL_CMD_ENCAP_V2;
      ssg->undecided = 0x00;
      ssg->SCmdClass = COMMAND_CLASS_SWITCH_BINARY;
      ssg->SClassSubFunc = SWITCH_BINARY_SET;
      ssg->state = 0;
      ssg->duration = 0;
      ssg->Epid = j;
      SAPI_SetSendBytesMulti(sb,ssg);
      BYTE datalen = sizeof(SendBytes.ZwBinSwitchMulti);

      if (!ZW_SendData_Bridge(SAPI_SourceId, i, sb->ZwBinSwitchMulti, datalen, SAPI_TxOption, CbSendDataSingle)){
        LOG_PRINTF("failed\n");
      }
      //SAPI_GetResponse_Test(0x25,0x02,LocalMultichannelNode);
      free(ssg);
      free(sb);

      SAPI_delay(10);
      }

    }

    for(uint16_t i = LocalMultichannelNode; i<= LocalMultichannelNode+1; i++){
      for(uint16_t j = 1; j < 5; j++){
      set_values_8bt_t *ssg = NULL;
      SerApi_hex_data_t *sb = NULL;
      ssg = (set_values_8bt_t*)malloc(sizeof(set_values_8bt_t));
      sb = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
      ssg->PCmdClass = COMMAND_CLASS_MULTI_CHANNEL_V2;
      ssg->PClassSubFunc = MULTI_CHANNEL_CMD_ENCAP_V2;
      ssg->undecided = 0x00;
      ssg->SCmdClass = COMMAND_CLASS_SWITCH_BINARY;
      ssg->SClassSubFunc = SWITCH_BINARY_SET;
      ssg->state = 1;
      ssg->duration = 0;
      ssg->Epid = j;
      SAPI_SetSendBytesMulti(sb,ssg);
      BYTE datalen = sizeof(SendBytes.ZwBinSwitchMulti);

      if (!ZW_SendData_Bridge(SAPI_SourceId, i, sb->ZwBinSwitchMulti, datalen, SAPI_TxOption, CbSendDataSingle)){
        LOG_PRINTF("failed\n");
      }
     // SAPI_GetResponse_Test(0x25,0x02,LocalMultichannelNode);
      free(ssg);
      free(sb);

      SAPI_delay(10);
      }
    }
    //////////////////////////////// Binary Switch Test ////////////////////////////////////
    set_values_4bt_t *ssg1 = NULL;
    SerApi_hex_data_t *sb1 = NULL;
    ssg1 = (set_values_4bt_t*)malloc(sizeof(set_values_4bt_t));
    sb1 = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
    ssg1->CmdClass = COMMAND_CLASS_SWITCH_BINARY;
    ssg1->ClassSubFunc = SWITCH_BINARY_SET;
    ssg1->state = 1;
    ssg1->duration = 0;
    SAPI_SetSendBytesSingle(sb1,ssg1);
    BYTE datalen1 = sizeof(SendBytes.ZwBinSwitchSingle);
    if (!ZW_SendData_Bridge(SAPI_SourceId, LocalBinaryNode, sb1->ZwBinSwitchSingle, datalen1, SAPI_TxOption, CbSendDataSingle)){
      LOG_PRINTF("failed\n");
    }
    //SAPI_GetResponse_Test(0x25,0x02,LocalBinaryNode);
    free(ssg1);
    free(sb1);
    SAPI_delay(10);

    set_values_4bt_t *ssg = NULL;
    SerApi_hex_data_t *sb = NULL;
    ssg = (set_values_4bt_t*)malloc(sizeof(set_values_4bt_t));
    sb = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
    ssg->CmdClass = COMMAND_CLASS_SWITCH_BINARY;
    ssg->ClassSubFunc = SWITCH_BINARY_SET;
    ssg->state = 0;
    ssg->duration = 0;
    SAPI_SetSendBytesSingle(sb,ssg);
    BYTE datalen = sizeof(SendBytes.ZwBinSwitchSingle);
    if (!ZW_SendData_Bridge(SAPI_SourceId, LocalBinaryNode, sb->ZwBinSwitchSingle, datalen, SAPI_TxOption, CbSendDataSingle)){
      LOG_PRINTF("failed\n");
    }
    //SAPI_GetResponse_Test(0x25,0x02,LocalBinaryNode);
    free(ssg);
    free(sb);
    SAPI_delay(10);

    
  }
//////////////////////////////// Multi Level Test //////////////////////////////////

    for(int i = 0; i < 99; i++)
    {
      set_values_4bt_t *ssg2 = NULL;
      SerApi_hex_data_t *sb2 = NULL;
      ssg2 = (set_values_4bt_t*)malloc(sizeof(set_values_4bt_t));
      sb2 = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
      ssg2->CmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
      ssg2->ClassSubFunc = SWITCH_MULTILEVEL_SET;
      ssg2->state = i;
      ssg2->duration = 0;
      SAPI_SetSendBytesSingle(sb2,ssg2);
      BYTE datalen = sizeof(SendBytes.ZwBinSwitchSingle);
      if (!ZW_SendData_Bridge(SAPI_SourceId, LocalMultiLevelNode, sb2->ZwBinSwitchSingle, datalen, SAPI_TxOption, CbSendDataSingle)){
        LOG_PRINTF("failed\n");
      }
      //SAPI_GetResponse_Test(0x26,0x02,LocalMultiLevelNode);
      free(ssg2);
      free(sb2);
      SAPI_delay(10);
    }

    for(int i = 99; i >= 20; i--)
    {
      set_values_4bt_t *ssg2 = NULL;
      SerApi_hex_data_t *sb2 = NULL;
      ssg2 = (set_values_4bt_t*)malloc(sizeof(set_values_4bt_t));
      sb2 = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
      ssg2->CmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL;
      ssg2->ClassSubFunc = SWITCH_MULTILEVEL_SET;
      ssg2->state = i;
      ssg2->duration = 0;
      SAPI_SetSendBytesSingle(sb2,ssg2);
      BYTE datalen = sizeof(SendBytes.ZwBinSwitchSingle);
      if (!ZW_SendData_Bridge(SAPI_SourceId, LocalMultiLevelNode, sb2->ZwBinSwitchSingle, datalen, SAPI_TxOption, CbSendDataSingle)){
        LOG_PRINTF("failed\n");
      }
      //SAPI_GetResponse_Test(0x26,0x02,LocalMultiLevelNode);
      free(ssg2);
      free(sb2);
      SAPI_delay(10);
    }
}
#endif