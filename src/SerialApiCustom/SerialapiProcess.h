//Toggle this macro to switch between dev and prod
#define PROD

/// @brief responsible for writting all data from SerialGateway on socket for upward communication
/// @param buff data buffer
/// @return 1 if success 0 otherwise
int Sapi2Zwave_send(char * buff);
unsigned int atoh(char *ap);