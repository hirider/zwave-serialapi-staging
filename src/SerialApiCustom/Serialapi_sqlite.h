#include "../ZIP_Router.h"

enum{
    ni_nodeid,
    ni_EndpointCount,  
    ni_created_at,     
    ni_modified_at,    
    ni_RSSI,   
    ni_RfRegion,       
    ni_Txpower
};

enum{
    cci_id,
    cci_nodeid,      
    cci_endpoint,    
    cci_cmd_class,
    cci_cmd_class_len,   
    cci_created_at,  
    cci_modified_at, 
    cci_zw_plus_version_info,
    cci_role_type,
    cci_role_type_len,           
    cci_node_type,
    cci_node_type_len,           
    cci_grouping_identifier, 
    cci_max_nodes_supported, 
    cci_reports_to_follow,   
    cci_supported_groupings, 
    cci_group_identifier,    
    cci_zw_library_type,     
    cci_zw_protocol_type,    
    cci_power_level,         
    cci_state              
};

enum{
    cc_command,
    cc_status = 2
};

enum{
    cmd_id,
    cmd_command
};

enum{
    sup_nodeid,
    sup_cmd_class
};


typedef struct SAPI_datastore_networkinfo_t{

    BYTE nodeid;
    BYTE EndpointCount;
    // char * cmd_class;
    // BYTE cmd_class_len;
    BYTE RSSI;
    BYTE RfRegion;
    BYTE Txpower;
    BYTE ret;

}SAPI_datastore_networkinfo_t;

typedef struct SAPI_datastore_cmdclassinfo_t{

    BYTE id;
    BYTE nodeid;
    BYTE endpoint;
    char * cmd_class;
    BYTE cmd_class_len;
    BYTE zw_plus_version_info;
    char * role_type;
    BYTE role_type_len;
    char * node_type;
    BYTE node_type_len;
    BYTE grouping_identifier;
    BYTE max_nodes_supported;
    BYTE reports_to_follow;
    BYTE supported_groupings;
    BYTE group_identifier;
    BYTE zw_library_type;
    BYTE zw_protocol_type;
    BYTE power_level;
    BYTE state;
    BYTE ret;

}SAPI_datastore_cmdclassinfo_t;

typedef struct SAPI_datastore_command_class_t{

    BYTE command;
    BYTE status;
    BYTE ret;

}SAPI_datastore_command_class_t;

typedef struct SAPI_datastore_commands_t{

    BYTE id;
    BYTE command;
    BYTE ret;

}SAPI_datastore_commands_t;

typedef struct SAPI_datastore_supported_cmdclass_t{

    BYTE nodeid;
    BYTE cmd_class;
    BYTE ret;

}SAPI_datastore_supported_cmdclass_t;


/// @brief generates database tables if not created after first execution
/// @return 1 if success 0 therwise
bool SAPI_data_store_init(void);

/// @brief frees memory
/// @param p data pointer
void SAPI_data_mem_free(void *p);

bool SAPI_data_store_nvm_write_networkinfo(SAPI_datastore_networkinfo_t * n);

bool SAPI_data_store_nvm_write_cmdclassinfo(SAPI_datastore_cmdclassinfo_t * n);

bool SAPI_data_store_nvm_write_supportedcmdclass(SAPI_datastore_supported_cmdclass_t * n);

bool SAPI_data_store_nvm_update_cci(SAPI_datastore_cmdclassinfo_t * n);

bool SAPI_data_store_nvm_delete_networkinfo(SAPI_datastore_networkinfo_t *n);

bool SAPI_data_store_nvm_delete_cci(SAPI_datastore_cmdclassinfo_t * n);

bool SAPI_data_store_nvm_delete_supportedcmdclass(SAPI_datastore_supported_cmdclass_t * n);

//TODO: IMPLEMENT A SAFER WAY TO SEND DATA FOR THE BELOW FUNCTIONS INSTEAD OF RAW POINTER

/// @brief queries networkinfo table for information. BEWARE!!! THIS FUNCTION RETURNS A DYNAMICALLY ALLOCATED RAW POINTER, ALWAYS FREE THE POINTER AFTER USE
/// @param nodeID node id
/// @return 1 if success 0 otherwise
SAPI_datastore_networkinfo_t * SAPI_DbQueryNetworkInfo(nodeid_t nodeID);

/// @brief queries cmdclassinfo table for information. BEWARE!!! THIS FUNCTION RETURNS A DYNAMICALLY ALLOCATED RAW POINTER, ALWAYS FREE THE POINTER AFTER USE
/// @param nodeID nodeid
/// @param endpoint endpoint
/// @return 1 if success 0 otherwise
SAPI_datastore_cmdclassinfo_t * SAPI_DbQueryCmdClassInfo(nodeid_t nodeID, uint8_t endpoint);

/// @brief queries cmdclass table for information. BEWARE!!! THIS FUNCTION RETURNS A DYNAMICALLY ALLOCATED RAW POINTER, ALWAYS FREE THE POINTER AFTER USE
/// @param command_str query statement
/// @return pointer to data
SAPI_datastore_command_class_t * SAPI_DbQueryCmdClasses(char * command_str);

/// @brief queries commands table for information. BEWARE!!! THIS FUNCTION RETURNS A DYNAMICALLY ALLOCATED RAW POINTER, ALWAYS FREE THE POINTER AFTER USE
/// @param command_str query statement
/// @param cmd_class cmd class
/// @return pointer to data
SAPI_datastore_commands_t * SAPI_DbQueryCommands(char * command_str, BYTE cmd_class);

/// @brief queries supportedcmdclass table for information. BEWARE!!! THIS FUNCTION RETURNS A DYNAMICALLY ALLOCATED RAW POINTER, ALWAYS FREE THE POINTER AFTER USE
/// @param nodeid node id
/// @param cmd_class cmd class
/// @return pointer to data
SAPI_datastore_supported_cmdclass_t * SAPI_DbQuerySupportedCmdClass(nodeid_t nodeid, uint8_t cmd_class);

/// @brief delets all data from networkinfo table CAUTION!!! DO NOT CALL IT ANYWHERE BUT DATA RESET. WILL PERMENANTLY DELETE ALL DATA
/// @return 1 if sucess 0 otherwise
bool SAPI_datareset_networkinfo(void);

/// @brief delets all data from cmdclassinfo table CAUTION!!! DO NOT CALL IT ANYWHERE BUT DATA RESET. WILL PERMENANTLY DELETE ALL DATA
/// @return 1 if sucess 0 otherwise
bool SAPI_datareset_cmdclassinfo(void);

/// @brief delets all data from supportedcmdclass table CAUTION!!! DO NOT CALL IT ANYWHERE BUT DATA RESET. WILL PERMENANTLY DELETE ALL DATA
/// @return 1 if sucess 0 otherwise
bool SAPI_datareset_supportedcmdclass(void);




