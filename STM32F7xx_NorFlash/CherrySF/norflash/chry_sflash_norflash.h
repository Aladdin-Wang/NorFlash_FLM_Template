/*
 * Copyright (c) 2024, sakumisu
 * Copyright (c) 2024, RCSN
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHRY_SFLASH_NORFLASH_H
#define CHRY_SFLASH_NORFLASH_H

#include "chry_sflash_sfdp_def.h"
#include "chry_sflash.h"

/* NORFLASH command */
#define NORFLASH_COMMAND_WRITE_ENABLE          (0x06U)
#define NORFLASH_COMMAND_WRITE_DISABLE         (0x04U)
#define NORFLASH_COMMAND_READ_MANUFACTURER_ID  (0x90U)
#define NORFLASH_COMMAND_READ_JEDECID          (0x9FU)

#define NORFLASH_COMMAND_READ_1_1_1_3B         (0x03U)
#define NORFLASH_COMMAND_READ_1_1_1_4B         (0x13U)
#define NORFLASH_COMMAND_FAST_READ_1_1_1_3B    (0x0BU)
#define NORFLASH_COMMAND_FAST_READ_1_1_1_4B    (0x0CU)

#define NORFLASH_COMMAND_PAGE_PROGRAM_1_1_1_3B (0x02U)
#define NORFLASH_COMMAND_PAGE_PROGRAM_1_1_1_4B (0x12U)

#define NORFLASH_COMMAND_SECTOR_ERASE_4K_3B    (0x20U)
#define NORFLASH_COMMAND_SECTOR_ERASE_4K_4B    (0x21U)
#define NORFLASH_COMMAND_SECTOR_ERASE_64K_3B   (0xD8U)
#define NORFLASH_COMMAND_SECTOR_ERASE_64K_4B   (0xDCU)
#define NORFLASH_COMMAND_CHIPERASE             (0x60U)

#define NORFLASH_COMMAND_READ_STATUS_REG1      (0x05U)
#define NORFLASH_COMMAND_WRITE_STATUS_REG1     (0x01U)
#define NORFLASH_COMMAND_READ_STATUS_REG2      (0x35U)
#define NORFLASH_COMMAND_WRITE_STATUS_REG2     (0x31U)
#define NORFLASH_COMMAND_READ_STATUS_REG3      (0x15U)
#define NORFLASH_COMMAND_WRITE_STATUS_REG3     (0x11U)

#define NORFLASH_COMMAND_READ_SFDP             (0x5AU)

#define NORFLASH_COMMAND_ENTER_4B_ADDRESS_MODE (0xB7U)
#define NORFLASH_COMMAND_EXIT_4B_ADDRESS_MODE  (0xE9U)

#define NORFLASH_COMMAND_FAST_READ_1_1_2_3B    (0x3BU)
#define NORFLASH_COMMAND_FAST_READ_1_1_2_4B    (0x3CU)

#define NORFLASH_COMMAND_FAST_READ_1_2_2_3B    (0xBBU)
#define NORFLASH_COMMAND_FAST_READ_1_2_2_4B    (0xBCU)

#define NORFLASH_COMMAND_PAGE_PROGRAM_1_1_4_3B (0x32U)
#define NORFLASH_COMMAND_PAGE_PROGRAM_1_1_4_4B (0x34U)
#define NORFLASH_COMMAND_FAST_READ_1_1_4_3B    (0x6BU)
#define NORFLASH_COMMAND_FAST_READ_1_1_4_4B    (0x6CU)

#define NORFLASH_COMMAND_FAST_READ_1_4_4_3B    (0xEBU)
#define NORFLASH_COMMAND_FAST_READ_1_4_4_4B    (0xECU)

struct chry_sflash_norflash_jedec_info {
    jedec_basic_flash_param_table_t basic_flash_param_table;
    uint32_t basic_flash_param_table_size;
    jedec_4byte_addressing_inst_table_t jedec_4byte_addressing_inst_table;
    bool jedec_4byte_addressing_inst_table_enable;
};

struct chry_sflash_norflash {
    struct chry_sflash_host *host;
    uint8_t sfdp_major_version;
    uint8_t sfdp_minor_version;
    uint32_t flash_size;
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint8_t addr_size;
    uint8_t block_erase_cmd;
    uint8_t sector_erase_cmd;
    uint8_t page_program_cmd;
    uint8_t page_program_addr_mode;
    uint8_t page_program_data_mode;
    uint8_t read_cmd;
    uint8_t read_addr_mode;
    uint8_t read_dummy_bytes;
    uint8_t read_data_mode;
};

#ifdef __cplusplus
extern "C" {
#endif

int chry_sflash_norflash_init(struct chry_sflash_norflash *flash, struct chry_sflash_host *host);
int chry_sflash_norflash_erase(struct chry_sflash_norflash *flash, uint32_t start_addr, uint32_t len);
int chry_sflash_norflash_write(struct chry_sflash_norflash *flash, uint32_t start_addr, uint8_t *buf, uint32_t buflen);
int chry_sflash_norflash_read(struct chry_sflash_norflash *flash, uint32_t start_addr, uint8_t *buf, uint32_t buflen);

#ifdef __cplusplus
}
#endif

#endif