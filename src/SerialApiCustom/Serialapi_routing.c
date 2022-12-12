#include "Serialapi_routing.h"
#include "SerialapiEventHandler.h"
#include "SerialapiProcess.h"


#define MAX_PRIORITY_ROUTE 3


BYTE route_arr[MAX_PRIORITY_ROUTE+2];

void SAPI_LockRoute(){
    uint16_t nodeid = 32;
    ZW_LockRoute(nodeid);
}

void SAPI_DecodeRoute(long int *routeinfo){ 

    BYTE count = MAX_PRIORITY_ROUTE; 

    for(int i = 0 ; i <= MAX_PRIORITY_ROUTE; i++){
        route_arr[MAX_PRIORITY_ROUTE-i] = ((*routeinfo >> i*8) & 0xFF);
    }
  //this can be applied through API also
  //9600 = 1
  //40k =  2
  //100k = 3
    route_arr[4] = ZW_LAST_WORKING_ROUTE_SPEED_9600;
}

void SAPI_PriorityRoute(long int *routeinfo, uint8_t DestNode){

    SAPI_DecodeRoute(routeinfo);

    if(ZW_SetPriorityRoute(DestNode, route_arr)){
      printf("Priority route set for node %x\n",DestNode);
    }
}

void SAPI_GetPriorityRoute(uint8_t node){

    BYTE * lwr = malloc(sizeof(BYTE));

    if(ZW_GetPriorityRoute(node, lwr)){

        printf("route found\n%d<-",node);
        
        for(int i = 0; i < 4; i++){
            if(i != 3){
                printf("%x<-",lwr[i]);
            }
            else if(i != 0){
                printf("%x",lwr[i]);
            }
        }
        printf("\n");
    } 
    else{
        LOG_PRINTF("No route found for node id %d\n",node);
    }
    free(lwr);
}