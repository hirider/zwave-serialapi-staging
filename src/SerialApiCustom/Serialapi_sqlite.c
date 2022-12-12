#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Serialapi_sqlite.h"
#include "SerialapiProcess.h"

static sqlite3 *db;
static sqlite3_stmt *ni_insert_stmt = NULL;
static sqlite3_stmt *ni_select_stmt = NULL;
static sqlite3_stmt *ni_update_stmt = NULL;

static sqlite3_stmt *cci_insert_stmt = NULL;
static sqlite3_stmt *cci_select_stmt = NULL;
static sqlite3_stmt *cci_update_stmt = NULL;
static sqlite3_stmt *cci_delete_stmt = NULL;

static sqlite3_stmt *cc_select_stmt = NULL;

static sqlite3_stmt *cmd_select_stmt = NULL;

static sqlite3_stmt *sup_insert_stmt = NULL;
static sqlite3_stmt *sup_select_stmt = NULL;
static sqlite3_stmt *sup_delete_stmt = NULL;

static sqlite3_stmt *cleanup_update_stmt = NULL;

#ifndef PROD
const char *serial_database_file = "/home/aura/dev/db/serialgateway.db";
#else
const char *serial_database_file = "/usr/local/var/lib/zipgateway/serialgateway.db";
#endif

char TimeStamp[20];


//Convenience  macros, it turns out that sqlite_bind_xxx is
// 1 indexed and the sqlite3_column_xxx functions are all 0 indexed
#define sqlite3_bind_ex_int(stmt, idx, val) \
  sqlite3_bind_int(stmt, idx + 1, val)
#define sqlite3_bind_ex_text(stmt, idx, ptr, sz, func) \
  sqlite3_bind_text(stmt, idx + 1, ptr, sz, func)

BYTE SAPI_SanatizeReadInput(BYTE int_val){

  char tohex[50];
  sprintf(tohex,"%x",int_val);
  return atoh(tohex);
}

void get_time(){
  time_t now = time(NULL);
  strftime(TimeStamp, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
}

void *SAPI_data_mem_alloc(uint8_t size)
{
  return malloc(size);
}

void SAPI_data_mem_free(void *p)
{
  free(p);
}

static int
SAPI_datastore_exec_sql(const char *sql)
{
  char *err_msg = NULL;
  int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
  if (rc != SQLITE_OK)
  {
    if (err_msg != NULL)
    {
      ERR_PRINTF("SQL Error: %s\n", err_msg);
    }
    else
    {
      ERR_PRINTF("SQL Error: Unknown\n");
    }
    sqlite3_free(err_msg);
  }
  return rc;
}

bool SAPI_prepare_statements()
{
  int rc;
  const char *sql;

  sql = "SELECT * FROM cmd_class_info WHERE nodeid = ? AND endpoint = ?";
  rc = sqlite3_prepare_v2(db, sql, -1, &cci_select_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  sql = "SELECT * FROM cmd_classes WHERE command_class = ?";
  rc = sqlite3_prepare_v2(db, sql, -1, &cc_select_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  sql = "SELECT id,command FROM commands WHERE end_command = ? AND cmd_class = ?";
  rc = sqlite3_prepare_v2(db, sql, -1, &cmd_select_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  sql = "SELECT * FROM network_info WHERE nodeid = ?";
  rc = sqlite3_prepare_v2(db, sql, -1, &ni_select_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  sql = "SELECT nodeid,cmd_class FROM supported_cmd_class WHERE nodeid = ? and cmd_class = ?";
  rc = sqlite3_prepare_v2(db, sql, -1, &sup_select_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  sql = "INSERT INTO network_info VALUES(?,?,?,?,?,?,?)";
  rc = sqlite3_prepare_v2(db, sql, -1, &ni_insert_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  sql = "INSERT INTO cmd_class_info VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
  rc = sqlite3_prepare_v2(db, sql, -1, &cci_insert_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  sql = "INSERT INTO supported_cmd_class VALUES(?,?,?)";
  rc = sqlite3_prepare_v2(db, sql, -1, &sup_insert_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  sql = "UPDATE cmd_class_info SET state = ?, modified_at = ? WHERE nodeid = ? AND endpoint = ?";
  rc = sqlite3_prepare_v2(db, sql, -1, &cci_update_stmt, NULL);
  if (rc != SQLITE_OK)
  {
    goto fail;
  }

  ///// clean-up statements /////

  // sql = "UPDATE SQLITE_SEQUENCE SET SEQ = 0 WHERE NAME = ?";
  // rc = sqlite3_prepare_v2(db, sql, -1, &cleanup_update_stmt, NULL);
  // if (rc != SQLITE_OK)
  // {
  //   goto fail;
  // }
  
  return true;

fail:
  ERR_PRINTF("prepare failed: %s\n", sqlite3_errmsg(db));
  return false;
}

bool SAPI_data_store_init(void)
{
  sqlite3_stmt *stmt = NULL;
  int rc;

   SAPI_SanatizeReadInput(31);

  if (db == 0)
  {
    LOG_PRINTF("Using db file: %s\n", serial_database_file);
    int rc = sqlite3_open(serial_database_file, &db);
    if (rc != SQLITE_OK)
    {
      ERR_PRINTF("Cannot open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return false;
    }

    rc = sqlite3_prepare_v2(db, "SELECT SQLITE_VERSION()", -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
      ERR_PRINTF("Failed to fetch data: %s\n", sqlite3_errmsg(db));
      if (rc != SQLITE_OK)
      {
        goto fail;
      }
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
      LOG_PRINTF("SQLITE Version: %s\n", sqlite3_column_text(stmt, 0));
    }
    //sqlite3_finalize(stmt);

    // Create tables if not exist
    rc = SAPI_datastore_exec_sql("CREATE TABLE IF NOT EXISTS network_info ("
                            "nodeid   INTEGER    PRIMARY KEY,"
                            "EndpointCount           INTEGER,"
                            "created_at            TIMESTAMP,"
                            "modified_at           TIMESTAMP,"
                            "RSSI                    INTEGER,"
                            "RfRegion                INTEGER,"
                            "Txpower                  INTEGER"
                            ");");

    if (rc != SQLITE_OK)
    {
      goto fail;
    }

    rc = SAPI_datastore_exec_sql("CREATE TABLE IF NOT EXISTS cmd_class_info ("
                            "id   INTEGER   PRIMARY KEY AUTOINCREMENT,"
                            "nodeid                    INTEGER,"
                            "endpoint                  INTEGER,"
                            "cmd_class     VARCHAR    NOT NULL,"
                            "cmd_class_len             INTEGER,"
                            "created_at              TIMESTAMP,"
                            "modified_at             TIMESTAMP,"
                            "zw_plus_version_info      INTEGER,"
                            "role_type      VARCHAR   NOT NULL,"
                            "role_type_len  INTEGER   NOT NULL,"
                            "node_type   VARCHAR      NOT NULL,"
                            "node_type_len  INTEGER   NOT NULL,"
                            "grouping_identifier       INTEGER,"
                            "max_nodes_supported       INTEGER,"
                            "reports_to_follow         INTEGER,"
                            "supported_groupings       INTEGER,"
                            "group_identifier          INTEGER,"
                            "zw_library_type           INTEGER,"
                            "zw_protocol_type          INTEGER,"
                            "power_level               INTEGER,"
                            "state                      INTEGER"
                            ");");

    if (rc != SQLITE_OK)
    {
      goto fail;
    }

    rc = SAPI_datastore_exec_sql("CREATE TABLE IF NOT EXISTS cmd_classes ("
                            "command INTEGER      PRIMARY KEY,"
                            "command_class   VARCHAR NOT NULL,"
                            "status                    INTEGER"
                            ");");
    if (rc != SQLITE_OK)
    {
      goto fail;
    }

    rc = SAPI_datastore_exec_sql("CREATE TABLE IF NOT EXISTS commands ("
                            "id    INTEGER    PRIMARY KEY,"
                            "command              INTEGER,"
                            "end_command VARCHAR NOT NULL,"
                            "cmd_class             INTEGER"
                            ");");
    if (rc != SQLITE_OK)
    {
      goto fail;
    }

    rc = SAPI_datastore_exec_sql("CREATE TABLE IF NOT EXISTS supported_cmd_class ("
                            "id   INTEGER   PRIMARY KEY AUTOINCREMENT,"
                            "nodeid             INTEGER,"
                            "cmd_class           INTEGER"
                            ");");
    if (rc != SQLITE_OK)
    {
      goto fail;
    }

    if (!SAPI_prepare_statements())
    {
      goto fail;
    }
  }

  if (!SAPI_prepare_statements())
  {
    goto fail;
  }

  sqlite3_finalize(stmt);
  return true;

fail:
  sqlite3_close(db);
  return false;
}

bool SAPI_data_store_nvm_delete_networkinfo(SAPI_datastore_networkinfo_t * n)
{
  sqlite3_stmt *stmt = NULL;
  int rc;
  rc = sqlite3_prepare_v2(db, "DELETE FROM network_info WHERE nodeid = ?", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    ERR_PRINTF("SQL Error: Failed to execute statement: %s\n",
               sqlite3_errmsg(db));
    return false;
  }
  sqlite3_bind_int(stmt, 1, n->nodeid);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true;
}

bool SAPI_data_store_nvm_delete_cci(SAPI_datastore_cmdclassinfo_t * n)
{
  sqlite3_stmt *stmt = NULL;
  int rc;
  rc = sqlite3_prepare_v2(db, "DELETE FROM cmd_class_info WHERE nodeid = ?", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    ERR_PRINTF("SQL Error: Failed to execute statement: %s\n",
               sqlite3_errmsg(db));
    return false;
  }
  sqlite3_bind_int(stmt, 1, n->nodeid);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true;

}

bool SAPI_data_store_nvm_delete_supportedcmdclass(SAPI_datastore_supported_cmdclass_t * n)
{
  sqlite3_stmt *stmt = NULL;
  int rc;
  rc = sqlite3_prepare_v2(db, "DELETE FROM supported_cmd_class WHERE nodeid = ?", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    ERR_PRINTF("SQL Error: Failed to execute statement: %s\n",
               sqlite3_errmsg(db));
    return false;
  }
  sqlite3_bind_int(stmt, 1, n->nodeid);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true;
}

bool SAPI_datareset_networkinfo(void)
{
  sqlite3_stmt *stmt = NULL;
  int rc;
  rc = sqlite3_prepare_v2(db, "DELETE FROM network_info", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    ERR_PRINTF("SQL Error: Failed to execute statement: %s\n",
               sqlite3_errmsg(db));
    return false;
  }
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true;
}

bool SAPI_datareset_cmdclassinfo(void)
{
  sqlite3_stmt *stmt = NULL;
  int rc;
  rc = sqlite3_prepare_v2(db, "DELETE FROM cmd_class_info", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    ERR_PRINTF("SQL Error: Failed to execute statement: %s\n",
               sqlite3_errmsg(db));
    return false;
  }
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true;
}

bool SAPI_datareset_supportedcmdclass(void)
{
  sqlite3_stmt *stmt = NULL;
  int rc;
  rc = sqlite3_prepare_v2(db, "DELETE FROM supported_cmd_class", -1, &stmt, NULL);
  if (rc != SQLITE_OK)
  {
    ERR_PRINTF("SQL Error: Failed to execute statement: %s\n",
               sqlite3_errmsg(db));
    return false;
  }
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return true;
}

bool SAPI_data_store_nvm_write_networkinfo(SAPI_datastore_networkinfo_t * n)
{
  int rc;
  get_time();

  sqlite3_reset(ni_insert_stmt);
  sqlite3_bind_ex_int(ni_insert_stmt, ni_nodeid, n->nodeid);
  sqlite3_bind_ex_int(ni_insert_stmt, ni_EndpointCount, n->EndpointCount);
  sqlite3_bind_ex_text(ni_insert_stmt, ni_created_at, TimeStamp, -1, SQLITE_TRANSIENT);
  sqlite3_bind_ex_text(ni_insert_stmt, ni_modified_at, TimeStamp, -1, SQLITE_TRANSIENT);
  sqlite3_bind_ex_int(ni_insert_stmt, ni_RSSI, n->RSSI);
  sqlite3_bind_ex_int(ni_insert_stmt, ni_RfRegion, n->RfRegion);
  sqlite3_bind_ex_int(ni_insert_stmt, ni_Txpower, n->Txpower);

  rc = sqlite3_step(ni_insert_stmt);
  if (rc != SQLITE_DONE && rc != SQLITE_ROW)
  {
    ERR_PRINTF("execution failed: %s\n", sqlite3_errmsg(db));
    return false;
  }
  return true;
}

bool SAPI_data_store_nvm_write_cmdclassinfo(SAPI_datastore_cmdclassinfo_t * n){
  int rc;
  get_time();

  sqlite3_reset(cci_insert_stmt);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_nodeid, n->nodeid);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_endpoint, n->endpoint);
  sqlite3_bind_ex_text(cci_insert_stmt, cci_cmd_class, n->cmd_class,-1, SQLITE_STATIC);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_cmd_class_len, n->cmd_class_len);
  sqlite3_bind_ex_text(cci_insert_stmt, cci_created_at, TimeStamp,-1, SQLITE_TRANSIENT);
  sqlite3_bind_ex_text(cci_insert_stmt, cci_modified_at, TimeStamp,-1, SQLITE_TRANSIENT);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_zw_plus_version_info, n->zw_plus_version_info);
  sqlite3_bind_ex_text(cci_insert_stmt, cci_role_type, n->role_type, -1, SQLITE_STATIC);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_role_type_len, n->role_type_len);
  sqlite3_bind_ex_text(cci_insert_stmt, cci_node_type, n->node_type, -1, SQLITE_STATIC);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_node_type_len, n->node_type_len);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_grouping_identifier, n->grouping_identifier);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_max_nodes_supported, n->max_nodes_supported);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_reports_to_follow, n->reports_to_follow);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_supported_groupings, n->supported_groupings);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_group_identifier, n->group_identifier);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_zw_library_type, n->zw_library_type);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_zw_protocol_type, n->zw_protocol_type);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_power_level, n->power_level);
  sqlite3_bind_ex_int(cci_insert_stmt, cci_state, n->state);

  rc = sqlite3_step(cci_insert_stmt);
  if (rc != SQLITE_DONE && rc != SQLITE_ROW)
  {
    ERR_PRINTF("execution failed: %s\n", sqlite3_errmsg(db));
    return false;
  }
  return true;
}

bool SAPI_data_store_nvm_write_supportedcmdclass(SAPI_datastore_supported_cmdclass_t * n)
{
  int rc;

  sqlite3_reset(sup_insert_stmt);
  sqlite3_bind_ex_int(sup_insert_stmt, sup_nodeid+1, n->nodeid);
  sqlite3_bind_ex_int(sup_insert_stmt, sup_cmd_class+1, n->cmd_class);


  rc = sqlite3_step(sup_insert_stmt);
  if (rc != SQLITE_DONE && rc != SQLITE_ROW)
  {
    ERR_PRINTF("execution failed: %s\n", sqlite3_errmsg(db));
    return false;
  }
  return true;
}

bool SAPI_data_store_nvm_update_cci(SAPI_datastore_cmdclassinfo_t * n)
{
  int rc;
  get_time();

  sqlite3_reset(cci_update_stmt);

  sqlite3_bind_ex_int(cci_update_stmt, 0, n->state);
  sqlite3_bind_ex_text(cci_update_stmt, 1,TimeStamp,-1,SQLITE_TRANSIENT);
  sqlite3_bind_ex_int(cci_update_stmt, 2,n->nodeid);
  sqlite3_bind_ex_int(cci_update_stmt, 3,n->endpoint);

  rc = sqlite3_step(cci_update_stmt);
  if (rc != SQLITE_DONE && rc != SQLITE_ROW)
  {
    ERR_PRINTF("execution failed: %s\n", sqlite3_errmsg(db));
    return false;
  }
  return true;
}

SAPI_datastore_networkinfo_t * SAPI_DbQueryNetworkInfo(nodeid_t nodeID){

  SAPI_datastore_networkinfo_t * n = NULL;
  int rc;

  sqlite3_reset(ni_select_stmt);
  sqlite3_bind_ex_int(ni_select_stmt, 0, nodeID);

  n = SAPI_data_mem_alloc(sizeof(SAPI_datastore_networkinfo_t));

  int step = sqlite3_step(ni_select_stmt);
  if (step != SQLITE_ROW)
  {
    DBG_PRINTF("Node ID %i not found\n", nodeID);
    memset(n, 0, sizeof(n));
    n->ret = false;
    return n;
  }

  if(n != NULL){

    if (n == 0)
    {
      ERR_PRINTF("Out of memory\n");
      n->ret = false;
      return n;
    }

    n->nodeid = SAPI_SanatizeReadInput(sqlite3_column_int(ni_select_stmt, ni_nodeid));
    n->EndpointCount = SAPI_SanatizeReadInput(sqlite3_column_int(ni_select_stmt, ni_EndpointCount));
    n->RSSI = SAPI_SanatizeReadInput(sqlite3_column_int(ni_select_stmt, ni_RSSI));
    n->RfRegion = SAPI_SanatizeReadInput(sqlite3_column_int(ni_select_stmt, ni_RfRegion));
    n->Txpower = SAPI_SanatizeReadInput(sqlite3_column_int(ni_select_stmt, ni_Txpower));
    n->ret = true;
  }

  return n;

}


SAPI_datastore_cmdclassinfo_t * SAPI_DbQueryCmdClassInfo(nodeid_t nodeID, uint8_t endpoint)
{
  SAPI_datastore_cmdclassinfo_t *n = NULL;
  int rc;

  sqlite3_reset(cci_select_stmt);
  sqlite3_bind_ex_int(cci_select_stmt, 0, nodeID);
  sqlite3_bind_ex_int(cci_select_stmt, 1, endpoint);

  n = SAPI_data_mem_alloc(sizeof(SAPI_datastore_cmdclassinfo_t));

  int step = sqlite3_step(cci_select_stmt);
  if (step != SQLITE_ROW)
  {
    DBG_PRINTF("Node ID %i, endpoint %i not found\n", nodeID,endpoint);
    n->ret = false;
    return n;
  }

  if(n != NULL)
  {
    if (n == 0)
    {
      ERR_PRINTF("Out of memory\n");
      memset(n, 0, sizeof(n));
      n->ret = false;
      return n;
    }

    n->id = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_id));
    n->nodeid = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_nodeid));
    n->endpoint = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_endpoint));
    n->cmd_class_len = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_cmd_class_len));

    n->cmd_class = SAPI_data_mem_alloc(n->cmd_class_len); 
    memcpy(n->cmd_class, sqlite3_column_text(cci_select_stmt, cci_cmd_class), n->cmd_class_len);

    n->zw_plus_version_info = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_zw_plus_version_info));

    n->role_type_len = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_role_type_len));

    n->role_type = SAPI_data_mem_alloc(n->role_type_len);
    memcpy(n->role_type, sqlite3_column_text(cci_select_stmt, cci_role_type), n->role_type_len);

    n->node_type_len = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_node_type_len));

    n->node_type = SAPI_data_mem_alloc(n->node_type_len);
    memcpy(n->node_type, sqlite3_column_text(cci_select_stmt, cci_node_type), n->node_type_len);

    n->grouping_identifier = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_grouping_identifier));
    n->max_nodes_supported = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_max_nodes_supported));
    n->reports_to_follow = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_reports_to_follow));
    n->supported_groupings = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_supported_groupings));
    n->group_identifier = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_group_identifier));
    n->zw_library_type = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_zw_library_type));
    n->zw_protocol_type = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_zw_protocol_type));
    n->power_level = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_power_level));
    n->state = SAPI_SanatizeReadInput(sqlite3_column_int(cci_select_stmt, cci_state));

    n->ret = true;
  }
  return n;
}


SAPI_datastore_command_class_t * SAPI_DbQueryCmdClasses(char * command_str)
{
  SAPI_datastore_command_class_t * n = NULL;
  int rc;

  sqlite3_reset(cc_select_stmt);
  sqlite3_bind_ex_text(cc_select_stmt, 0, command_str,-1,SQLITE_STATIC);

  n = SAPI_data_mem_alloc(sizeof(SAPI_datastore_command_class_t));

  int step = sqlite3_step(cc_select_stmt);
  if (step != SQLITE_ROW)
  {
    n->ret = false;
    DBG_PRINTF("string %s not found\n", command_str);
    return n;
  }

  if(n != NULL)
  {
    if (n == 0)
    {
      ERR_PRINTF("Out of memory\n");
      memset(n, 0, sizeof(n));
      n->ret = false;
      return n;
    }

    n->command = SAPI_SanatizeReadInput(sqlite3_column_int(cc_select_stmt, cc_command)); 
    n->status = sqlite3_column_int(cc_select_stmt, cc_status);

    n->ret = true;
  }
  return n;
}

SAPI_datastore_commands_t * SAPI_DbQueryCommands(char * command_str, BYTE cmd_class)
{
  SAPI_datastore_commands_t * n = NULL;
  int rc;

  sqlite3_reset(cmd_select_stmt);
  sqlite3_bind_ex_text(cmd_select_stmt, 0, command_str,-1,SQLITE_STATIC);
  sqlite3_bind_ex_int(cmd_select_stmt, 1, cmd_class);

  n = SAPI_data_mem_alloc(sizeof(SAPI_datastore_commands_t));

  int step = sqlite3_step(cmd_select_stmt);
  if (step != SQLITE_ROW)
  {
    DBG_PRINTF("string %s not found\n", command_str);
    n->ret = false;
    return n;
  }

  if(n != NULL)
  {
    if (n == 0)
    {
      ERR_PRINTF("Out of memory\n");
      memset(n, 0, sizeof(n));
      n->ret = false;
      return n;
    }

    n->id = sqlite3_column_int(cmd_select_stmt, cmd_id);
    n->command = SAPI_SanatizeReadInput(sqlite3_column_int(cmd_select_stmt, cmd_command));

    n->ret = true;
  }
  return n;
}

SAPI_datastore_supported_cmdclass_t * SAPI_DbQuerySupportedCmdClass(nodeid_t nodeID, uint8_t cmd_class){

  SAPI_datastore_supported_cmdclass_t * n = NULL;
  int rc;

  sqlite3_reset(sup_select_stmt);
  sqlite3_bind_ex_int(sup_select_stmt,sup_nodeid, nodeID);
  sqlite3_bind_ex_int(sup_select_stmt,sup_cmd_class,cmd_class);

  n = SAPI_data_mem_alloc(sizeof(SAPI_datastore_supported_cmdclass_t));

  int step = sqlite3_step(sup_select_stmt);
  if (step != SQLITE_ROW)
  {
    DBG_PRINTF("Node ID %i with cmd class %i does not exist\n", nodeID,cmd_class);
    memset(n, 0, sizeof(n));
    n->ret = false;
    return n;
  }

  if(n != NULL)
  {
    if (n == 0)
    {
      ERR_PRINTF("Out of memory\n");
      n->ret = false;
      return n;
    }

    n->nodeid = sqlite3_column_int(sup_select_stmt, sup_nodeid);
    n->cmd_class = SAPI_SanatizeReadInput(sqlite3_column_int(sup_select_stmt, sup_cmd_class));
    n->ret = true;
  }

  return n;
}

// bool SAPI_data_store_nvm_update_cleanup(char * table_name)
// {
//   int rc;

//   sqlite3_reset(cleanup_update_stmt);

//   sqlite3_bind_ex_text(cleanup_update_stmt, 1,table_name,-1,SQLITE_TRANSIENT);

//   rc = sqlite3_step(cleanup_update_stmt);
//   if (rc != SQLITE_DONE && rc != SQLITE_ROW)
//   {
//     ERR_PRINTF("execution failed: %s\n", sqlite3_errmsg(db));
//     return false;
//   }
//   return true;
// }