/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHRY_SFLASH_NANDFLASH_H
#define CHRY_SFLASH_NANDFLASH_H

#include "chry_sflash.h"

/* NANDFLASH command */
#define NANDFLASH_COMMAND_JEDEC_ID                      (0x9FU)
#define NANDFLASH_COMMAND_READ_STATUS_REG               (0x05U)
#define NANDFLASH_COMMAND_WRITE_STATUS_REG              (0x01U)
#define NANDFLASH_COMMAND_WRITE_ENABLE                  (0x06U)
#define NANDFLASH_COMMAND_WRITE_DISABLE                 (0x04U)
#define NANDFLASH_COMMAND_BLOCK_ERASE                   (0xD8U)
#define NANDFLASH_COMMAND_PROGRAM_DATA_LOAD             (0x02U)
#define NANDFLASH_COMMAND_RANDOM_PROGRAM_DATA_LOAD      (0x84U)
#define NANDFLASH_COMMAND_QUAD_PROGRAM_DATA_LOAD        (0x32U)
#define NANDFLASH_COMMAND_QUAD_RANDOM_PROGRAM_DATA_LOAD (0x34U)
#define NANDFLASH_COMMAND_PROGRAM_EXECUTE               (0x10U)
#define NANDFLASH_COMMAND_PAGE_DATA_READ_INTO_CACHE     (0x13U)
#define NANDFLASH_COMMAND_READ_1_1_1_3B                 (0x03U)
#define NANDFLASH_COMMAND_FAST_READ_1_1_1_3B            (0x0BU)
#define NANDFLASH_COMMAND_FAST_READ_1_1_1_4B            (0x0CU)
#define NANDFLASH_COMMAND_FAST_READ_1_1_2_3B            (0x3BU)
#define NANDFLASH_COMMAND_FAST_READ_1_1_2_4B            (0x3CU)
#define NANDFLASH_COMMAND_FAST_READ_1_1_4_3B            (0x6BU)
#define NANDFLASH_COMMAND_FAST_READ_1_1_4_4B            (0x6CU)
#define NANDFLASH_COMMAND_FAST_READ_1_2_2_3B            (0xBBU)
#define NANDFLASH_COMMAND_FAST_READ_1_2_2_4B            (0xBCU)
#define NANDFLASH_COMMAND_FAST_READ_1_4_4_3B            (0xEBU)
#define NANDFLASH_COMMAND_FAST_READ_1_4_4_4B            (0xECU)

/* Status register addr cmd */
#define NANDFLASH_SR1_ADDR                              0xa0 /* Status Register:Protection Register Addr */
#define NANDFLASH_SR2_ADDR                              0xb0 /* Status Register:Configuration Register Addr */
#define NANDFLASH_SR3_ADDR                              0xc0 /* Status Register:Status */

#define NANDFLASH_SR1_SRP1                              (1 << 0)
#define NANDFLASH_SR1_WP_ENABLE                         (1 << 1)
#define NANDFLASH_SR1_TB                                (1 << 2)
#define NANDFLASH_SR1_BP0                               (1 << 3)
#define NANDFLASH_SR1_BP1                               (1 << 4)
#define NANDFLASH_SR1_BP2                               (1 << 5)
#define NANDFLASH_SR1_BP3                               (1 << 6)
#define NANDFLASH_SR1_SRP0                              (1 << 7)

#define NANDFLASH_SR2_BUF_MODE                          (1 << 3)
#define NANDFLASH_SR2_ECC_ENABLE                        (1 << 4)
#define NANDFLASH_SR2_SR1_LOCK                          (1 << 5)
#define NANDFLASH_SR2_OTP_ENABLE                        (1 << 6)
#define NANDFLASH_SR2_OTP_LOCK                          (1 << 7)

#define NANDFLASH_SR3_BUSY                              (1 << 0)
#define NANDFLASH_SR3_WEL                               (1 << 1)
#define NANDFLASH_SR3_ERASE_FAIL                        (1 << 2)
#define NANDFLASH_SR3_PROGRAM_FAIL                      (1 << 3)
#define NANDFLASH_SR3_ECC_STATUS_SHIFT                  (4)
#define NANDFLASH_SR3_ECC_STATUS_MASK                   (0x3 << NANDFLASH_SR3_ECC_STATUS_SHIFT)
#define NANDFLASH_SR3_BBM_LUT_FULL                      (1 << 6)

struct chry_sflash_nandflash {
    struct chry_sflash_host *host;
    const char *name;
    uint32_t flash_size;
    uint32_t total_blocks;
    uint32_t pages_per_block;
    uint32_t bytes_per_page;
    uint8_t spare_bytes_per_page;
    uint8_t program_cmd;
    uint8_t program_addr_mode;
    uint8_t program_data_mode;
    uint8_t read_cmd;
    uint8_t read_addr_mode;
    uint8_t read_dummy_bytes;
    uint8_t read_data_mode;
};

#ifdef __cplusplus
extern "C" {
#endif

int chry_sflash_nandflash_init(struct chry_sflash_nandflash *flash, struct chry_sflash_host *host);
int chry_sflash_nandflash_erase(struct chry_sflash_nandflash *flash, uint32_t block);
int chry_sflash_nandflash_write(struct chry_sflash_nandflash *flash,
                             uint32_t block,
                             uint32_t page,
                             uint8_t *buf,
                             uint32_t buflen,
                             uint8_t *spare,
                             uint32_t spare_len);
int chry_sflash_nandflash_read(struct chry_sflash_nandflash *flash,
                            uint32_t block,
                            uint32_t page,
                            uint8_t *buf,
                            uint32_t buflen,
                            uint8_t *spare,
                            uint32_t spare_len);

#ifdef __cplusplus
}
#endif

#endif