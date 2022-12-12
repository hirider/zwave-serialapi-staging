/* © 2014 Silicon Laboratories Inc.
 */

#include "contiki.h"
#include "net/uip.h"
#include "Serialapi.h"
#include "ZW_controller_api.h"
#include "ZW_controller_api_ex.h"
#include "ZW_ZIPApplication.h"

#include "ZIP_Router.h" /* ApplicationCommandHandlerSerial */

#include "Bridge.h"
#include "security_layer.h"
#include "ZIP_Router_logging.h"
#include "txmodem.h"
#include "ZW_basis_api.h"

PROCESS(serial_api_process, "ZW Serial api");
extern void SerialFlush();
extern uint8_t SupportsSerialAPISetup_func(uint8_t);
extern int ZWGeckoFirmwareUpdate(void * fw_desc,void *chip_desc,
                          char *filename, size_t len);

BYTE serial_ok = FALSE;

/*TODO For some reason this is not in WIN32 environments by ZW_controller_api.h */
#ifdef WIN32
extern void
ApplicationControllerUpdate(
    BYTE bStatus, /*IN  Status of learn mode */
    BYTE bNodeID, /*IN  Node id of the node that send node info */
    BYTE *pCmd,   /*IN  Pointer to Application Node information */
    BYTE bLen);   /*IN  Node info length                        */
#endif

const struct SerialAPI_Callbacks serial_api_callbacks =
  { ApplicationCommandHandlerSerial, 0,
      ApplicationControllerUpdate, 0, 0, 0, 0,
      ApplicationCommandHandlerSerial, SerialAPIStarted};

struct chip_descriptor chip_desc = {CHIP_DESCRIPTOR_UNINITIALIZED, CHIP_DESCRIPTOR_UNINITIALIZED};

/* Serial port polling loop */
static void
pollhandler(void)
{
  if (SerialAPI_Poll())
  {
    //Schedule more polling if needed
    process_poll(&serial_api_process);
  }

  secure_poll();
}

int serial_process_serial_init(const char *serial_port)
{
  unsigned char buf[14];
  int type;
  int fd = 0;


  LOG_PRINTF("Serial Process init\n");
  if (serial_port)
  {
    LOG_PRINTF("Using serial port %s\n", serial_port);
    serial_ok = SerialAPI_Init(serial_port, &serial_api_callbacks);
    if (!serial_ok)
    {
      ERR_PRINTF("Error opening serial API\n");
      process_exit(&serial_api_process);
      return 1;
    }
    process_post(&zip_process,PROCESS_SERIAL_API,0);
  }
  else
  {
    ERR_PRINTF("Please set serial port name\n");
    process_exit(&serial_api_process);
    return 1;
  }

  while ((type = ZW_Version(buf)) == 0)
  {
    WRN_PRINTF("Unable to communicate with serial API.\n");
  }
  LOG_PRINTF("Version: %s, type %i\n", buf, type);

  PROTOCOL_VERSION zw_protocol_version2 = {0};
  ZW_GetProtocolVersion(&zw_protocol_version2);
  DBG_PRINTF("SDK: %d.%02d.%02d, SDK Build no: %d\n",
      zw_protocol_version2.protocolVersionMajor,
      zw_protocol_version2.protocolVersionMinor,
      zw_protocol_version2.protocolVersionRevision,
      zw_protocol_version2.zaf_build_no);
  DBG_PRINTF(" SDK git hash: ");
  int i;
  for (i = 0; i < 16; i++) {
    printf("%x", zw_protocol_version2.git_hash_id[i]);
  }
  printf("\n");


  /* Abort if we find out that the Z-Wave module is not running a bridge library */
  if (ZW_LIB_CONTROLLER_BRIDGE != type)
  {
    ERR_PRINTF("Z-Wave dongle MUST be a Bridge library\n");
    serial_ok = FALSE;
    process_exit(&serial_api_process);
    return 1;
  }

  /* RFRegion and TXPowerlevel requires Z-Wave module restarts */
  int soft_reset = 0;
  SerialAPI_GetChipTypeAndVersion(&(chip_desc.my_chip_type), &(chip_desc.my_chip_version));
  /*
          * Set the TX powerlevel only if
          * 1. If it's not 500 series
          * 2. If the module supports the command and sub-command
          * 3. Valid powerlevel setting exists in zipgateway.cfg
          * 4. Current settings != settings in zipgateway.cfg
          */
  if ((chip_desc.my_chip_type != ZW_CHIP_TYPE) && 
    SerialAPI_SupportsCommand_func(FUNC_ID_SERIALAPI_SETUP) && 
    SupportsSerialAPISetup_func(SERIAL_API_SETUP_CMD_TX_POWERLEVEL_GET)
    )
  {
    TX_POWER_LEVEL current_txpowerlevel = ZW_TX_POWERLEVEL_GET();
    if (SupportsSerialAPISetup_func(SERIAL_API_SETUP_CMD_TX_POWERLEVEL_SET) && (cfg.is_powerlevel_set == 1) && (current_txpowerlevel.normal != cfg.tx_powerlevel.normal || current_txpowerlevel.measured0dBm != cfg.tx_powerlevel.measured0dBm))
    {
      LOG_PRINTF("Setting TX powerlevel to %d and output powerlevel to %d\n", cfg.tx_powerlevel.normal, cfg.tx_powerlevel.measured0dBm);
      if (ZW_TXPowerLevelSet(cfg.tx_powerlevel))
      {
        soft_reset = 1;
      }
      else
      {
        ERR_PRINTF("Failed to set the TX powerlevel through serial API\n");
      }
    }
  }

  /*
          * Only set the region if
          * 1. it supprts Region Get/Set and not 500 series
          * 2. ZWRFRegion exists in zipgateway.cfg
          * 3. The retrieved current region is valid
          * 4. Current region != region in zipgateway.cfg
          */
  if ((chip_desc.my_chip_type != ZW_CHIP_TYPE) 
    && SerialAPI_SupportsCommand_func(FUNC_ID_SERIALAPI_SETUP) 
    && SupportsSerialAPISetup_func(SERIAL_API_SETUP_CMD_RF_REGION_GET)
  )
  {
    uint8_t current_region = ZW_RFRegionGet();
    if (SupportsSerialAPISetup_func(SERIAL_API_SETUP_CMD_RF_REGION_SET) && (cfg.rfregion != 0xFE) && (current_region != 0xFE) && (current_region != cfg.rfregion))
    {
      LOG_PRINTF("Setting RF region to %02x\n", cfg.rfregion);
      if (ZW_RFRegionSet(cfg.rfregion))
      {
        soft_reset = 1;
      }
      else
      {
        ERR_PRINTF("Failed to set the RF region through serial API\n");
      }
    }
  }

  /* RFRegion and TX_Powerlevel requires Z-Wave module restart to take
          * effect.
          */
  if (soft_reset == 1)
  {
    LOG_PRINTF("Resetting the Z-Wave chip for region and tx powerlevel to take effect\n");
    ZW_SoftReset();
    while ((type = ZW_Version(buf)) == 0)
    {
      LOG_PRINTF("Trying to communicate with serial API.\n");
    }
    LOG_PRINTF("Done resetting the Z-Wave chip\n");
  }
  if (ZW_RFRegionGet() == RF_US_LR) {
    if (SerialAPI_EnableLR() == false) {
      LOG_PRINTF("Fail to enable Z-Wave Long Range capability\n");
    }
  } else {
    if (SerialAPI_DisableLR() == false) {
      LOG_PRINTF("Fail to disable Z-Wave Long Range capability\n");
    }
  }
  ZW_AddNodeToNetwork(ADD_NODE_STOP, 0);
  ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, 0);
  ZW_SetLearnMode(ZW_SET_LEARN_MODE_DISABLE, 0);

  if (chip_desc.my_chip_type == ZW_GECKO_CHIP_TYPE)
  {
    SerialAPI_WatchdogStart();
  }

  process_poll(&serial_api_process);
  return 0;
}


PROCESS_THREAD(serial_api_process, ev, data)
{
  /****************************************************************************/
  /*                              PRIVATE DATA                                */
  /****************************************************************************/
  PROCESS_POLLHANDLER(pollhandler());
  PROCESS_EXITHANDLER(SerialAPI_Destroy());
  PROCESS_BEGIN();
  while (1)
  {
    switch (ev)
    {
    case PROCESS_EVENT_INIT:
      return serial_process_serial_init((const char *)data);
    break;
    }
    PROCESS_WAIT_EVENT();
  }
  SerialFlush();
  PROCESS_END()
}
