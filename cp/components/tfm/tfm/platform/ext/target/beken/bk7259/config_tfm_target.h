/*
 * Copyright (c) 2024 Beken
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __CONFIG_TFM_TARGET_H__
#define __CONFIG_TFM_TARGET_H__

#define ITS_RAM_FS 1

/* Serve psa_cipher_* (single-part and multipart) from the crypto partition. */
#define CRYPTO_CIPHER_MODULE_ENABLED 1

#endif /* __CONFIG_TFM_TARGET_H__ */
