
#include<unity.h>

#include "../../src/transport/Security_Scheme0.c"

/*********************************************** Stubs ***********************************************/
int cb_count=0;
int sd_count=0;
int sd_size=0;
uint8_t sd_data[128];
ts_param_t sd_p;
void* sd_user;
static int sec0_cb_count = 0;
TX_STATUS_TYPE ts;

ZW_SendDataAppl_Callback_t sd_callback;

void ctimer_set(struct ctimer *c, clock_time_t t,
		void (*f)(void *), void *ptr)
{

}

void ctimer_stop(struct ctimer *c) {

}

/* Reset test counters. To be called between test cases. */
static void test_reset() {
  cb_count=0;
  sd_count=0;
  sd_size=0;
  memset(&sd_p, 0xAB, sizeof(sd_p));
  sec0_cb_count = 0;
  memset(&ts, 0xAB, sizeof(ts));
}

clock_time_t clock_time() {
    return 0x42;
}

u8_t send_data(ts_param_t* p, const u8_t* data, u16_t len,
    ZW_SendDataAppl_Callback_t cb, void* user)
{
    sd_callback = cb;
    sd_size = len;
    memcpy(sd_data,data,len);
    sd_p = *p;
    sd_user = user;
    sd_count++;

    printf("SD : ");
    for(int i=0; i < len; i++) {
        printf("%02x",sd_data[i]);
    }
    printf("\n");
    return TRUE;
}

void ts_param_make_reply(ts_param_t* dst, const ts_param_t* src) {

}


void PRNGOutput(uint8_t* out) {
    memcpy(out,"CCCCCCCC",8 );
}


void zw_appl_nvm_write(u16_t start,const void* dst,u8_t size) {

}

BOOL SerialAPI_AES128_Encrypt(const BYTE *ext_input, BYTE *ext_output, const BYTE *ext_key) {
    for(int i=0; i < 16; i++) {
        ext_output[i] = ext_input[i]^ext_key[i];
    }
    return TRUE;
}

BOOL secure_learn_active() {
    return FALSE;
}

void _xassert(const char* msg, int expr) {
    puts(msg);
}

void sec0_sd_callback(BYTE txStatus,void* user, TX_STATUS_TYPE *txStatEx) {
    sec0_cb_count++;
    //printf("sec0_sd_callback from line %u\n", (unsigned int)user);
}


/****************************************Test cases*******************************************************/



/*
 * From ZGW-2666, send_data callback is found to throw segfault if no ack on S0
 * frame. This test is to verify that msg1_callback works correctly when no ack
 * of S0 message is received while nonce get/report are still well-transmited.
 */
void test_null_ts() {
    sd_count = 0;
    ts_param_t p= {.dnode = 2, .snode=1 };
    uint8_t dec_msg[20];
    const char msg[] = "HelloWorld";
    sec0_init();

    TEST_ASSERT_TRUE(sec0_send_data(&p,msg,sizeof(msg),sec0_sd_callback,(void*)__LINE__));

    /*Verify that we are sending a nonce get*/
    TEST_ASSERT_EQUAL(1,sd_count);
    TEST_ASSERT_EQUAL(COMMAND_CLASS_SECURITY, sd_data[0]);
    TEST_ASSERT_EQUAL(SECURITY_NONCE_GET, sd_data[1]);

    sd_callback(TRANSMIT_COMPLETE_OK,sd_user,&ts);

    sec0_register_nonce(2,1,(const uint8_t*)"AAAAAAAA");
    /* Did we send a SECURITY_MESSAGE_ENCAPSULATION*/
    TEST_ASSERT_EQUAL(2,sd_count);
    TEST_ASSERT_EQUAL(COMMAND_CLASS_SECURITY, sd_data[0]);
    TEST_ASSERT_EQUAL(SECURITY_MESSAGE_ENCAPSULATION, sd_data[1]);

    /* Trigger callback with transmit fail as if no ack is received */
    sd_callback(TRANSMIT_COMPLETE_FAIL,sd_user,0);
}

void test_s0_rx_tx() {
    sd_count = 0;
    sec0_cb_count = 0;
    ts_param_t p= {.dnode = 2, .snode=1 };
    uint8_t dec_msg[20];
    const char msg[] = "HelloWorld";
    test_reset();
    sec0_init();

    TEST_ASSERT_TRUE(sec0_send_data(&p,msg,sizeof(msg),sec0_sd_callback,(void*)__LINE__));

    /*Verify that we are sending a nonce get*/
    TEST_ASSERT_EQUAL(1,sd_count);
    TEST_ASSERT_EQUAL(COMMAND_CLASS_SECURITY, sd_data[0]);
    TEST_ASSERT_EQUAL(SECURITY_NONCE_GET, sd_data[1]);

    sd_callback(TRANSMIT_COMPLETE_OK,sd_user,&ts);

    sec0_register_nonce(2,1,(const uint8_t*)"AAAAAAAA");
    /* Did we send a SECURITY_MESSAGE_ENCAPSULATION*/
    TEST_ASSERT_EQUAL(2,sd_count);
    TEST_ASSERT_EQUAL(COMMAND_CLASS_SECURITY, sd_data[0]);
    TEST_ASSERT_EQUAL(SECURITY_MESSAGE_ENCAPSULATION, sd_data[1]);

    sd_callback(TRANSMIT_COMPLETE_OK,sd_user,&ts);
    /*Verify that the secure tranmission is compleeted*/
    TEST_ASSERT_EQUAL(1,sec0_cb_count);

    /*Now act as a receiver, add three nonces and verify that we will use the right one */
    register_nonce(2,1,0,(const uint8_t*)"AAAAAAAA");
    register_nonce(2,1,0,(const uint8_t*)"DDDDDDDD");
    register_nonce(2,1,0,(const uint8_t*)"EEEEEEEE");

    TEST_ASSERT_NOT_EQUAL(0, sec0_decrypt_message(1,2,sd_data,sd_size,dec_msg));
    TEST_ASSERT_EQUAL_STRING(msg,dec_msg);

    /*Verify that we will not try to decypt the message a second time ie. reuse the nonce from before */
    TEST_ASSERT_EQUAL(0 ,sec0_decrypt_message(1,2,sd_data,sd_size,dec_msg) );

    /* Register the nonce again */
    register_nonce(2,1,0,(const uint8_t*)"AAAAAAAA"); 
    for(int i=0; i < 10; i++) {  //Make the nonce timeout
        nonce_timer_timeout(0);
    }
    /*Check that we will not use the old nonce.*/
    TEST_ASSERT_EQUAL(0 ,sec0_decrypt_message(1,2,sd_data,sd_size,dec_msg) );
}

/**
 * Test that repeated S0 nonce reports are ignored and not reused for encrypting a second message.
 */
void test_s0_blacklist() {
    ts_param_t p= {.dnode = 2, .snode=1 };
    uint8_t dec_msg[20];
    const char msg[] = "HelloWorld";
    const char msg2[] = "HelloWorld2";
    test_reset();
    sec0_init();

    TEST_ASSERT_TRUE(sec0_send_data(&p,msg,sizeof(msg),sec0_sd_callback,(void*)__LINE__));

    /*Verify that we are sending a nonce get*/
    TEST_ASSERT_EQUAL(1,sd_count);
    TEST_ASSERT_EQUAL(COMMAND_CLASS_SECURITY, sd_data[0]);
    TEST_ASSERT_EQUAL(SECURITY_NONCE_GET, sd_data[1]);

    sd_callback(TRANSMIT_COMPLETE_OK,sd_user,&ts);

    sec0_register_nonce(2,1,(const uint8_t*)"AAAAAAAA");
    /* Did we send a SECURITY_MESSAGE_ENCAPSULATION*/
    TEST_ASSERT_EQUAL(2,sd_count);
    TEST_ASSERT_EQUAL(COMMAND_CLASS_SECURITY, sd_data[0]);
    TEST_ASSERT_EQUAL(SECURITY_MESSAGE_ENCAPSULATION, sd_data[1]);

    sd_callback(TRANSMIT_COMPLETE_OK,sd_user,&ts);
    /*Verify that the secure tranmission is compleeted*/
    TEST_ASSERT_EQUAL(1,sec0_cb_count);

    TEST_ASSERT_TRUE(sec0_send_data(&p,msg2,sizeof(msg2),sec0_sd_callback,(void*)__LINE__));

    /* Send a second msg and verify that we are sending a nonce get (because the nonce report was blacklisted) */
    TEST_ASSERT_EQUAL(3,sd_count);
    TEST_ASSERT_EQUAL(COMMAND_CLASS_SECURITY, sd_data[0]);
    TEST_ASSERT_EQUAL(SECURITY_NONCE_GET, sd_data[1]);

    sd_callback(TRANSMIT_COMPLETE_OK,sd_user,&ts);

    /* Pretend we (the enc msg sender) received the same AAAA...AA nonce again. It should be blacklisted */
    sec0_register_nonce(2,1,(const uint8_t*)"AAAAAAAA");

    /* Verify we did not use the blacklisted nonce */
    TEST_ASSERT_EQUAL(3,sd_count);

    /* Now deliver a fresh nonce and verify that we send the encrypted message */
    sec0_register_nonce(2,1,(const uint8_t*)"BBBBBBBB");

    /* Did we send a SECURITY_MESSAGE_ENCAPSULATION? */
    TEST_ASSERT_EQUAL(4,sd_count);
    TEST_ASSERT_EQUAL(COMMAND_CLASS_SECURITY, sd_data[0]);
    TEST_ASSERT_EQUAL(SECURITY_MESSAGE_ENCAPSULATION, sd_data[1]);

    /* Now act as a receiver. We created the BBBB..BB nonce internally and register it */
    register_nonce(2,1,0,(const uint8_t*)"BBBBBBBB");
    /*Verify that we can decrypt the frame and the BBBB..BB nonce was indeed used */
    TEST_ASSERT_NOT_EQUAL(0, sec0_decrypt_message(1,2,sd_data,sd_size,dec_msg));
    TEST_ASSERT_EQUAL_STRING(msg2,dec_msg);

}

/**
 * Test low-level functions for the nonce blacklist
 */
void test_nonce_blacklist() {
  uint8_t zero_nonce[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  test_reset();
  sec0_init();

  TEST_ASSERT_FALSE(sec0_is_nonce_blacklisted(0,0, zero_nonce));
  sec0_blacklist_add_nonce(2, 1, (const uint8_t*)"11111111");
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(2, 1, (const uint8_t*)"11111111"));
  TEST_ASSERT_FALSE(sec0_is_nonce_blacklisted(3, 1, (const uint8_t*)"11111111"));
  TEST_ASSERT_FALSE(sec0_is_nonce_blacklisted(2, 4, (const uint8_t*)"11111111"));

  /* Now add more nonces than blacklist can hold and ensure that the oldest nonce is no longer blacklisted */
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"22222222");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"33333333");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"44444444");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"55555555");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"66666666");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"77777777");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"88888888");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"99999999");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"AAAAAAAA");
  sec0_blacklist_add_nonce(1, 2, (const uint8_t*)"BBBBBBBB");
  TEST_ASSERT_FALSE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"11111111"));

  /* Test that all ten most recent nonces are indeed blacklisted*/
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"22222222"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"33333333"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"44444444"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"55555555"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"66666666"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"77777777"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"88888888"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"99999999"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"AAAAAAAA"));
  TEST_ASSERT_TRUE(sec0_is_nonce_blacklisted(1, 2, (const uint8_t*)"BBBBBBBB"));
}
