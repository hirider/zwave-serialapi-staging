#ifdef SAPI_SECUIRTY
void SAPISecurityInit(uint16_t * SrcId, uint16_t * DesId, BYTE * TxOption);

void GetNonce();

void ProxySecurity();

void SAPI_S2InclusionInit(BYTE *AddedDesNode);
#endif