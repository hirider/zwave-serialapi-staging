#include <stdlib.h>
#include <stdio.h>
#include "ZIP_Router.h"

void SAPI_LockRoute();

/// @brief sets priority route between nodes
/// @param routeinfo route info as a BYTE array
/// @param DestNode destination node id
void SAPI_PriorityRoute(long int *routeinfo, uint8_t DestNode);

/// @brief fetches priority route value between nodes
/// @param node node id
void SAPI_GetPriorityRoute(uint8_t node);