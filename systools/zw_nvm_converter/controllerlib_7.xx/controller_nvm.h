/* © 2019 Silicon Laboratories Inc. */
#ifndef _NVMLIB711_H
#define _NVMLIB711_H

#include <stdbool.h>
#include <json.h>

// #include "zwave_controller_network_info_storage.h"
// #include "zwave_controller_application_info_storage.h"

// #define UNUSED(x) (void)(x)
// #define sizeof_array(ARRAY) ((sizeof ARRAY) / (sizeof ARRAY[0]))

// #define FLASH_PAGE_SIZE      2048
// #define APP_NVM_SIZE         (12 * 1024)
// #define APP_CACHE_SIZE       250
// #define PROTOCOL_NVM_SIZE    (36 * 1024)
// #define PROTOCOL_CACHE_SIZE  250

// #define TOTAL_NVM_SIZE      (APP_NVM_SIZE + PROTOCOL_NVM_SIZE)

bool check_controller_nvm(const uint8_t *nvm_image, size_t nvm_image_size);
bool open_controller_nvm(const uint8_t *nvm_image, size_t nvm_image_size);
void close_controller_nvm(void);

json_object* controller_nvm711_get_json(void);
json_object* controller_nvm715_get_json(void);
bool controller_parse_json(json_object *jo,uint32_t target_version);
size_t get_controller_nvm_image(uint8_t **image_buf);

void dump_controller_nvm_keys(void);

json_object* protocol_nvm_get_json(void);

json_object* app_nvm_get_json(void);

#endif // _NVMLIB711_H
