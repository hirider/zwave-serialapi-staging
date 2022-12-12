/* © 2017 Silicon Laboratories Inc.
 */
/*
 * s2_keystore.h
 *
 *  Created on: Sep 29, 2015
 *      Author: trasmussen
 */

#ifndef _S2_KEYSTORE_H_
#define _S2_KEYSTORE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define KEY_CLASS_S0                  0x80
#define KEY_CLASS_S2_UNAUTHENTICATED  0x01
#define KEY_CLASS_S2_AUTHENTICATED    0x02
#define KEY_CLASS_S2_ACCESS           0x04
#define KEY_CLASS_S2_AUTHENTICATED_LR 0x08
#define KEY_CLASS_S2_ACCESS_LR        0x10

#define KEY_CLASS_ALL                 0xFF

/* Flag used in common wrapper functions
 * to differentiate whether the crypto
 * function is PSA based or otherwise.
 */
#define ZWAVE_KEY_ID_NONE                0


/**
 * \defgroup keystore S2 Keystore
 *
 * The S2 keystore interface  handles storage of S2 keys as well as S0 keys.
 *
 * The keystore is responsible for reading and writing network keys and ECDH keys
 * to/from non volatile memory. This functionality must be implemented outside of
 * libs2
 * @{
 */

/**
 * Fetches public Curve25519 key from NVM and copies it to buf
 * \param[out] buf      Public key
 */
void keystore_public_key_read(uint8_t *buf);

/**
 * Fetches private Curve25519 key from NVM and copies it to buf
 * \param[out] buf      Private key
 */
void keystore_private_key_read(uint8_t *buf);

/**
 * Fetches network key from NVM and copies it to buf
 * \param[in]  keyclass Value denoting which keyclass is requested, see \ref s2_keystore.h for values.
 *                      Key class uses the BITMASK format, but exactly one bit must be set.
 * \param[out] buf      Network key
 *
 * \retval true        If network key was successfully read from the keystore and placed in buf.
 * \retval false       If network key could not be retrieved.
 */
bool keystore_network_key_read(uint8_t keyclass, uint8_t *buf);

/**
 * Writes Network key to NVM.
 * \return true if keyclass valid -> key written
 *         false if keyclass not supported and nothing written
 * \param[in] keyclass Value denoting which keyclass is is contained in keybuf, see \ref s2_keystore.h
 *                     for key class values.
 * \param[in] keybuf      Network key
 */
bool keystore_network_key_write(uint8_t keyclass,const uint8_t *keybuf);

/**
* Clears the specified keyclasses in NVM keystore
* \return true if keyclass cleared
*         false if keyclass not supported and therefor not cleared
* \param[in] keyclass The key class to clear. Key class uses the BITMASK format and exactly one
*           bit must be set
*   - KEY_CLASS_S2_UNAUTHENTICATED - S2_UNAUTHENTICATED keyclass cleared
*   - KEY_CLASS_S2_AUTHENTICATED   - S2_AUTHENTICATED keyclass cleared
*   - KEY_CLASS_S2_ACCESS          - S2_ACCESS keyclass cleared
*   - KEY_CLASS_S0                 - S0 keyclass cleared
*   - KEY_CLASS_ALL                - ALL keyclasses are cleared
*/
bool keystore_network_key_clear(uint8_t keyclass);

/**
 * @brief Backs up the non-scrambled network key in the NVM.
 * @details This function MUST be called prior to a firmware upgrade because the
 * key used to scramble is based on the binary in flash. This binary is likely
 * to change upon a firmware upgrade.
 */
void ZW_s2_keystore_backup_keys(void);

/**
 * @brief Restores the scrambled network keys in the NVM.
 * @details This function MUST be called after a firmware upgrade.
 */
void ZW_s2_keystore_restore_keys(void);

/**
 * @}
 */
#endif // _S2_KEYSTORE_H_
