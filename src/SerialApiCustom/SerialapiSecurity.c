#ifdef SAPI_SECUIRTY
#include <../serialapi/Serialapi.h>
#include <SerialapiEventHandler.h>
#ifndef _STDIO_H_
#include <stdio.h>
#endif
#include <../../libs2/include/s2_inclusion.h>
#include <../../libs2/include/s2_protocol.h>
#include <../../libs2/include/S2.h>
#include "../../libs2/include/ccm.h"
uint16_t Lsrc = 1;
uint16_t Ldes;
uint16_t LTxOp;

struct S2 SAPIS2Context;
s2_connection_t SAPIPeer;

struct S2 SAPI_s2_joining_context;
s2_connection_t SAPI_s2_joining_conn;

// static uint8_t  m_test_public_key_b[] = {0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4,
//                                          0xd3, 0x5b, 0x61, 0xc2, 0xec, 0xe4, 0x35, 0x37,
//                                          0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78, 0x67, 0x4d,
//                                          0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f};
#ifndef TESTER
static uint8_t  m_test_mem_pool[4][256];  // Statically allocated chunks of memory that can be freely used during the test.

static uint8_t  m_test_public_key_a[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                                         0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
                                         0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99,
                                         0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

static uint8_t  m_test_obfuscated_public_key_a[] = {0x00, 0x00, 0x00, 0x00, 0x55, 0x66, 0x77, 0x88,
                                                    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00,
                                                    0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99,
                                                    0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

static uint8_t  m_temp_key[] = {0x7E, 0xFE, 0x12, 0x32, 0x45, 0x65, 0x78, 0x98,
                                0x7E, 0xFE, 0x12, 0x32, 0x45, 0x65, 0x78, 0x98};

static uint8_t  m_test_network_key_s2_class_0[] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
                                                   0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99};

static uint8_t  m_test_network_key_s2_class_1[] = {0xAA, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
                                                   0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99};

static uint8_t  m_test_network_key_s2_class_2[] = {0xBB, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
                                                   0x00, 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99};

static uint8_t  m_test_network_key_s0[] = {0xCA, 0xFE, 0xBA, 0xBE, 0x44, 0x33, 0x22, 0x11,
                                           0xCA, 0xFE, 0xBA, 0xBE, 0xCC, 0xBB, 0xAA, 0x99};

static s2_connection_t s2_con;
#endif

void SAPISecurityInit(uint16_t * SrcId, uint16_t * DesId, BYTE * TxOption){
    //Lsrc = *SrcId;
    Ldes = *DesId;
    LTxOp = *TxOption;
}

void SAPI_S2InclusionInit(BYTE *AddedDesNode){

    // SAPI_s2_joining_context.inclusion_state = 0;
    // SAPIS2Context.scheme_support = 2;
    // SAPIS2Context.inclusion_state = 0;
    // SAPIPeer.l_node = Lsrc;
    // SAPIPeer.r_node = *AddedDesNode;
    // SAPIPeer.class_id = 0XFF;
    // SAPIPeer.rx_options = 0x00;

    // s2_inclusion_init(SECURITY_2_SCHEME_1_SUPPORT,KEX_REPORT_CURVE_25519,
    // 0x81 /* SECURITY_2_KEY_0 | SECURITY_2_KEY_2_CLASS_0 */);

    // s2_inclusion_including_start(&SAPIS2Context,&SAPIPeer);
    // uint8_t s2_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT,
    //                                0x02, // bit 0 is echo field, bit 1 is CSA.
    //                                0x02, // Supported schemes. Scheme 1.
    //                                0x01, // Supported ECDH Profiles, bit0=1 is curve 25519 value.
    //                                0x82};// Requested keys bits. Security 2 class 1, Security 0 network key.

    // SAPIS2Context.buf = s2_kex_report_frame;
    // SAPIS2Context.length = sizeof(s2_kex_report_frame);
    // SAPIPeer.class_id = 0xff;
    // s2_inclusion_post_event(&SAPIS2Context,&SAPIPeer);

    // s2_inclusion_key_grant(&SAPIS2Context, 1, 0x81,0);
    // uint8_t public_key_frame[3 + sizeof(m_test_public_key_b)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x00}; // Key exchange received from slave - public key for secure exchange of LTK.
    // memcpy(&public_key_frame[3], m_test_public_key_b, sizeof(m_test_public_key_b));
    // SAPIS2Context.buf    = public_key_frame;
    // SAPIS2Context.length = sizeof(public_key_frame);
    // SAPIPeer.class_id = 0xFF;
    // s2_inclusion_post_event(&SAPIS2Context,&SAPIPeer);

    // s2_inclusion_challenge_response(&SAPIS2Context, 1, m_test_public_key_b, 2);
    // uint8_t s2_echo_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x03, 0x02, 0x01, 0x82};
    // SAPIS2Context.buf    = s2_echo_kex_set_frame;
    // SAPIS2Context.length = sizeof(s2_echo_kex_set_frame);
    // SAPIPeer.class_id = 5;//UNIT_TEST_TEMP_KEY_SECURE;
    // s2_inclusion_post_event(&SAPIS2Context,&SAPIPeer);

struct S2 s2_context;

    s2_context.inclusion_state = S2_INC_IDLE;
    //memcpy(s2_context.temp_network_key, m_temp_key, sizeof(s2_context.temp_network_key));

    s2_inclusion_init(0x02, 0x01, 0x82);
    //s2_inclusion_set_event_handler(s2_event_handler);
    //helper_func_init_s2_conn();

    s2_inclusion_joining_start(&s2_context, &s2_con, 0x01);

    // KEX Get frame received.
    uint8_t S2_kex_get_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_GET};
    s2_context.buf           = S2_kex_get_frame;
    s2_context.length        = sizeof(S2_kex_get_frame);
    s2_con.class_id = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // KEX report is expeted to be transmitted. Let's make the s2_send_frame a success.
    s2_inclusion_send_done(&s2_context, 1);

    // KEX Set frame received.
    // bit0: echo field not set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_kex_set_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_SET, 0x02, 0x02, 0x01, 0x82};

    // if (test_flavour == 1)
    // {
    //   /* Test the special case where a reserved bit is set in the KEX_SET */
    //   s2_kex_set_frame[2] |= 0x10;
    // }

    s2_context.buf    = s2_kex_set_frame;
    s2_context.length = sizeof(s2_kex_set_frame);
    s2_con.class_id = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    uint8_t public_key_frame[3 + sizeof(m_test_public_key_a)] = {COMMAND_CLASS_SECURITY_2, PUBLIC_KEY_REPORT, 0x01}; // Key exchange received from slave - public key for secure exchange of LTK.
    memcpy(&public_key_frame[3], m_test_obfuscated_public_key_a, sizeof(m_test_obfuscated_public_key_a));
    s2_context.buf    = public_key_frame;
    s2_context.length = sizeof(public_key_frame);
    s2_con.class_id = 0xFF;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // After the received public key is pushed upwards in the system, then it is expected to receive a challenge response from the upper level.
    // The challenge response should result in correct pub key being set on the context.
    s2_inclusion_challenge_response(&s2_context, 1, m_test_public_key_a, DSK_CSA_CHALLENGE_LENGTH);
    //TEST_ASSERT_EQUAL_UINT8_ARRAY(s2_context.public_key, m_test_public_key_a, sizeof(m_test_public_key_a));

    // After receiving public key from joining node, the upper layer must specify our public key to be send. (This can also be done on initialization, but must happen no later than the received event).

    // Echo(KEX Report) frame received.
    // bit0: echo field set, bit1-7: Reserved.
    // Selected schemes: scheme 0 and scheme 2.
    // Selected curve25519
    // Keys to exchange, Security2, class 2 - Security0, network key.
    uint8_t s2_echo_kex_report_frame[] = {COMMAND_CLASS_SECURITY_2, KEX_REPORT, 0x03, 0x02, 0x01, 0x82};

    s2_context.buf    = s2_echo_kex_report_frame;
    s2_context.length = sizeof(s2_echo_kex_report_frame);
    s2_con.class_id = 0;//UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Network Key Report frame received.
    uint8_t s2_net_key_report_frame[3 + sizeof(m_test_network_key_s2_class_1)] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_REPORT, 0x02, };   // Keys requested, Security2, class 2 - Security0, network key.
    memcpy(&s2_net_key_report_frame[3], m_test_network_key_s2_class_1, sizeof(m_test_network_key_s2_class_1));
    s2_context.buf    = s2_net_key_report_frame;
    s2_context.length = sizeof(s2_net_key_report_frame);
    s2_con.class_id = 0;//UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // S2 Transfer end frame received.
    // bit0: Key request complete not set,
    // bit1: Key verified set,
    // bit2-7: Reserved.
    uint8_t s2_transfer_end_frame[] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_TRANSFER_END, 0x02};
    s2_context.buf    = s2_transfer_end_frame;
    s2_context.length = sizeof(s2_transfer_end_frame);
    s2_con.class_id = 0;//UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // Network Key Report frame received.
    uint8_t s2_net_key_report_0_frame[3 + sizeof(m_test_network_key_s0)] = {COMMAND_CLASS_SECURITY_2, SECURITY_2_NETWORK_KEY_REPORT, 0x80, };   // Keys requested, Security2, class 2 - Security0, network key.
    memcpy(&s2_net_key_report_0_frame[3], m_test_network_key_s0, sizeof(m_test_network_key_s0));
    s2_context.buf    = s2_net_key_report_0_frame;
    s2_context.length = sizeof(s2_net_key_report_0_frame);
    s2_con.class_id = 0;//UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    // S2 Transfer end frame received.
    // bit0: Key request complete not set,
    // bit1: Key verified set,
    // bit2-7: Reserved.
    s2_context.buf    = s2_transfer_end_frame;
    s2_context.length = sizeof(s2_transfer_end_frame);
    s2_con.class_id = 0;//UNIT_TEST_TEMP_KEY_SECURE;
    s2_inclusion_post_event(&s2_context, &s2_con);

    s2_inclusion_send_done(&s2_context, true);
}

void ProxySecurity(){

    uint8_t key[16]= {0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f};
    uint8_t nonce[16] = {0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0X1d,0x1e,0x1f};
    uint8_t text_to_encrypt[16] = {0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f};
    uint32_t aad_len = 16;
    uint16_t txtlen = 22;
    uint8_t aad[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f} ;

    // text_to_encrypt = (uint8_t*)malloc(16);
    // for(int i = 0; i < 16; i++){
    //     text_to_encrypt[i] = i;
    // }
    uint8_t ciphertext[22];
    uint32_t ciphertext_len = 22;

    memcpy(ciphertext, text_to_encrypt, 16);

    for(int i = 0; i < 16; i++){
        printf("%x",text_to_encrypt[i]);
    }
    printf("\n\n");
    int ret = CCM_encrypt_and_auth(key,nonce,aad,aad_len,ciphertext,16);
    if(ret != 0){
        // printf("encrypted\n");
        // for(int i = 0; i < sizeof(text_to_encrypt); i++){
        //     printf("%x",text_to_encrypt[i]);
        // }
    }

    set_val_18b_t *ssg = NULL;
    SerApi_hex_data_t *sb = NULL;
    ssg = (set_val_18b_t*)malloc(sizeof(set_val_18b_t));
    sb = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
    ssg->CmdClass = COMMAND_CLASS_SECURITY_2;
    ssg->ClassSubFunc = SECURITY_2_MESSAGE_ENCAPSULATION;
    SetSendBytesMulti2(sb,ssg,ciphertext);
    BYTE datalen = sizeof(sb->Zwm);

    if (!ZW_SendData_Bridge(Lsrc, Ldes, sb->Zwm, datalen, LTxOp, NULL))
    {
      LOG_PRINTF("failed\n");
    }
    free(ssg);
    free(sb);

    // const uint8_t bufval = 123;
    // uint16_t buflen = sizeof(bufval);

    // if(!S2_is_send_data_busy(&SAPIS2Context)){
    //     S2_send_data(&SAPIS2Context, &SAPIPeer, &bufval,buflen);
    // }

}

void GetNonce()
{
    SerApi_hex_data_t *sb = NULL;
    set_values_4bt_t *ssg = NULL;
    ssg = (set_values_4bt_t*)malloc(sizeof(set_values_4bt_t));
    sb = (SerApi_hex_data_t*)malloc(sizeof(SerApi_hex_data_t));
    ssg->CmdClass = COMMAND_CLASS_SECURITY_2;
    ssg->ClassSubFunc = SECURITY_2_NONCE_GET;
    ssg->state = 0;
    ssg->duration = 0;
    SetSendBytesSingle(sb,ssg);
    BYTE datalen = sizeof(sb->ZwBinSwitchSingle);
    ZW_SendData_Bridge(Lsrc, Ldes, sb->ZwBinSwitchSingle, datalen, LTxOp,NULL);
    free(ssg);
    free(sb);
}
#endif
