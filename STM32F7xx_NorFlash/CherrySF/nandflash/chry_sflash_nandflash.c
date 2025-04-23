/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "chry_sflash_nandflash.h"

static inline int chry_sflash_nandflash_read_deviceid(struct chry_sflash_nandflash *flash, uint8_t *buffer, uint32_t buflen)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = false;
    command_seq.cmd_phase.cmd = NANDFLASH_COMMAND_JEDEC_ID;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.dummy_phase.dummy_bytes = 1;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_READ;
    command_seq.data_phase.data_mode = CHRY_SFLASH_DATAMODE_1LINES;
    command_seq.data_phase.buf = buffer;
    command_seq.data_phase.len = buflen;

    return chry_sflash_transfer(host, &command_seq);
}

static inline int chry_sflash_nandflash_read_status_register(struct chry_sflash_nandflash *flash, uint8_t sr_addr, uint8_t *reg_data)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = false;
    command_seq.cmd_phase.cmd = NANDFLASH_COMMAND_READ_STATUS_REG;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.addr_phase.addr = sr_addr;
    command_seq.addr_phase.addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
    command_seq.addr_phase.addr_size = CHRY_SFLASH_ADDRSIZE_8BITS;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_READ;
    command_seq.data_phase.data_mode = CHRY_SFLASH_DATAMODE_1LINES;
    command_seq.data_phase.buf = reg_data;
    command_seq.data_phase.len = 1;

    return chry_sflash_transfer(host, &command_seq);
}

static inline int chry_sflash_nandflash_write_status_register(struct chry_sflash_nandflash *flash, uint8_t sr_addr, uint8_t reg_data)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = false;
    command_seq.cmd_phase.cmd = NANDFLASH_COMMAND_WRITE_STATUS_REG;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.addr_phase.addr = sr_addr;
    command_seq.addr_phase.addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
    command_seq.addr_phase.addr_size = CHRY_SFLASH_ADDRSIZE_8BITS;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_WRITE;
    command_seq.data_phase.data_mode = CHRY_SFLASH_DATAMODE_1LINES;
    command_seq.data_phase.buf = &reg_data;
    command_seq.data_phase.len = 1;

    return chry_sflash_transfer(host, &command_seq);
}

static inline int chry_sflash_nandflash_ecc_enable(struct chry_sflash_nandflash *flash, bool enable)
{
    uint8_t reg_data = 0;
    int ret;

    ret = chry_sflash_nandflash_read_status_register(flash, NANDFLASH_SR2_ADDR, &reg_data);
    ret += chry_sflash_nandflash_write_status_register(flash, NANDFLASH_SR2_ADDR, enable ? (reg_data | NANDFLASH_SR2_ECC_ENABLE) : (reg_data & ~NANDFLASH_SR2_ECC_ENABLE));
    return ret;
}

static inline int chry_sflash_nandflash_select_buf_mode(struct chry_sflash_nandflash *flash, uint8_t mode)
{
    uint8_t reg_data = 0;
    int ret;

    ret = chry_sflash_nandflash_read_status_register(flash, NANDFLASH_SR2_ADDR, &reg_data);
    ret += chry_sflash_nandflash_write_status_register(flash, NANDFLASH_SR2_ADDR, mode ? (reg_data | NANDFLASH_SR2_BUF_MODE) : (reg_data & ~NANDFLASH_SR2_BUF_MODE));
    return ret;
}

static inline int chry_sflash_nandflash_send_command_data(struct chry_sflash_nandflash *flash, uint8_t command, uint8_t dummy_bytes, uint8_t *data, uint32_t len)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = false;
    command_seq.cmd_phase.cmd = command;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.dummy_phase.dummy_bytes = dummy_bytes;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_WRITE;
    command_seq.data_phase.data_mode = CHRY_SFLASH_DATAMODE_1LINES;
    command_seq.data_phase.buf = data;
    command_seq.data_phase.len = len;

    return chry_sflash_transfer(host, &command_seq);
}

static inline int chry_sflash_nandflash_check_status(struct chry_sflash_nandflash *flash, uint8_t *status)
{
    return chry_sflash_nandflash_read_status_register(flash, NANDFLASH_SR3_ADDR, status);
}

int chry_sflash_nandflash_init(struct chry_sflash_nandflash *flash, struct chry_sflash_host *host)
{
    uint8_t device_id[3];
    uint16_t deviceid;
    uint8_t reg_data;
    int ret;

    flash->host = host;

    chry_sflash_set_frequency(flash->host, 10000000);

    chry_sflash_nandflash_read_deviceid(flash, device_id, 3);

    deviceid = (device_id[1] << 8) | (device_id[2] << 0);
    printf("NAND Flash MF: %02x, Device ID: %04x\r\n", device_id[0], deviceid);

    if (device_id[0] == 0xEF) {
        switch (deviceid) {
            case 0xAA21:
                flash->name = "W25N01GV";
                flash->total_blocks = 1024;
                flash->pages_per_block = 64;
                flash->bytes_per_page = 2048;
                flash->spare_bytes_per_page = 64;
                break;
            case 0xBF22:
                flash->name = "W25N02JW";
                flash->total_blocks = 2048;
                flash->pages_per_block = 64;
                flash->bytes_per_page = 2048;
                flash->spare_bytes_per_page = 64;
                break;
            case 0xBA23:
                flash->name = "W25N04KW";
                flash->total_blocks = 4096;
                flash->pages_per_block = 64;
                flash->bytes_per_page = 2176;
                flash->spare_bytes_per_page = 64;
                break;

            default:
                break;
        }
    } else {
        return -1;
    }

    flash->flash_size = flash->total_blocks * flash->pages_per_block * flash->bytes_per_page;

    if (flash->host->iomode == CHRY_SFLASH_IOMODE_QUAD) {
        flash->program_cmd = NANDFLASH_COMMAND_QUAD_RANDOM_PROGRAM_DATA_LOAD;
        flash->program_addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
        flash->program_data_mode = CHRY_SFLASH_DATAMODE_4LINES;
        flash->read_cmd = NANDFLASH_COMMAND_FAST_READ_1_4_4_3B;
        flash->read_addr_mode = CHRY_SFLASH_ADDRMODE_4LINES;
        flash->read_data_mode = CHRY_SFLASH_DATAMODE_4LINES;
        flash->read_dummy_bytes = 2;
    } else {
        flash->program_cmd = NANDFLASH_COMMAND_RANDOM_PROGRAM_DATA_LOAD;
        flash->program_addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
        flash->program_data_mode = CHRY_SFLASH_DATAMODE_1LINES;
        flash->read_cmd = NANDFLASH_COMMAND_FAST_READ_1_1_1_3B;
        flash->read_addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
        flash->read_data_mode = CHRY_SFLASH_DATAMODE_1LINES;
        flash->read_dummy_bytes = 1;
    }

    ret = chry_sflash_nandflash_read_status_register(flash, NANDFLASH_SR1_ADDR, &reg_data);
    reg_data &= ~NANDFLASH_SR1_WP_ENABLE;
    reg_data &= ~NANDFLASH_SR1_BP0;
    reg_data &= ~NANDFLASH_SR1_BP1;
    reg_data &= ~NANDFLASH_SR1_BP2;
    reg_data &= ~NANDFLASH_SR1_BP3;
    reg_data &= ~NANDFLASH_SR1_SRP1;
    reg_data &= ~NANDFLASH_SR1_SRP0;

    if (flash->host->iomode == CHRY_SFLASH_IOMODE_QUAD) {
        reg_data |= NANDFLASH_SR1_SRP0;
    }
    ret += chry_sflash_nandflash_write_status_register(flash, NANDFLASH_SR1_ADDR, reg_data);

    ret += chry_sflash_nandflash_ecc_enable(flash, true);
    ret += chry_sflash_nandflash_select_buf_mode(flash, 1);

    printf("NAND Flash Name: %s, Size: %dMB\r\n", flash->name, flash->flash_size / 1024 / 1024);
    printf("NAND Flash total_blocks: %d, pages_per_block: %d, bytes_per_page: %d\r\n", flash->total_blocks, flash->pages_per_block, flash->bytes_per_page);
    return ret;
}

int chry_sflash_nandflash_erase(struct chry_sflash_nandflash *flash, uint32_t block)
{
    int ret;
    uint8_t status;
    uint8_t page_addr_buf[2];

    if (block > flash->total_blocks) {
        return -CHRY_SFLASH_ERR_RANGE;
    }

    ret = chry_sflash_nandflash_send_command_data(flash, NANDFLASH_COMMAND_WRITE_ENABLE, 0, NULL, 0);
    if (ret < 0) {
        return ret;
    }

    page_addr_buf[0] = ((block * flash->pages_per_block) >> 8) & 0xff;
    page_addr_buf[1] = (block * flash->pages_per_block) & 0xff;
    ret = chry_sflash_nandflash_send_command_data(flash, NANDFLASH_COMMAND_BLOCK_ERASE, 1, page_addr_buf, 2);
    if (ret < 0) {
        return ret;
    }

    while (1) {
        ret = chry_sflash_nandflash_check_status(flash, &status);
        if (ret < 0) {
            return ret;
        }
        if (!(status & NANDFLASH_SR3_BUSY)) {
            if (status & NANDFLASH_SR3_ERASE_FAIL) {
                return -CHRY_SFLASH_ERR_IO;
            }
            break;
        }
    }

    return 0;
}

int chry_sflash_nandflash_write(struct chry_sflash_nandflash *flash,
                                    uint32_t block,
                                    uint32_t page,
                                    uint8_t *buf,
                                    uint32_t buflen,
                                    uint8_t *spare,
                                    uint32_t spare_len)
{
    int ret;
    uint8_t status;
    uint8_t page_addr_buf[2];
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    if (block > flash->total_blocks || page > flash->pages_per_block || (buflen && (buflen > flash->bytes_per_page)) || (spare_len && (spare_len > flash->spare_bytes_per_page))) {
        return -CHRY_SFLASH_ERR_RANGE;
    }

    command_seq.dma_enable = true;
    command_seq.cmd_phase.cmd = flash->program_cmd;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.addr_phase.addr_mode = flash->program_addr_mode;
    command_seq.addr_phase.addr_size = CHRY_SFLASH_ADDRSIZE_16BITS;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_WRITE;
    command_seq.data_phase.data_mode = flash->program_data_mode;

    ret = chry_sflash_nandflash_send_command_data(flash, NANDFLASH_COMMAND_WRITE_ENABLE, 0, NULL, 0);
    if (ret < 0) {
        return ret;
    }

    if (buf && buflen) {
        command_seq.addr_phase.addr = 0;
        command_seq.data_phase.buf = buf;
        command_seq.data_phase.len = buflen;
        ret = chry_sflash_transfer(host, &command_seq);
        if (ret < 0) {
            return ret;
        }
    }

    if (spare && spare_len) {
        command_seq.addr_phase.addr = flash->bytes_per_page;
        command_seq.data_phase.buf = spare;
        command_seq.data_phase.len = spare_len;
        ret = chry_sflash_transfer(host, &command_seq);
        if (ret < 0) {
            return ret;
        }
    }

    page_addr_buf[0] = ((page + block * flash->pages_per_block) >> 8) & 0xff;
    page_addr_buf[1] = (page + block * flash->pages_per_block) & 0xff;
    ret = chry_sflash_nandflash_send_command_data(flash, NANDFLASH_COMMAND_PROGRAM_EXECUTE, 1, page_addr_buf, 2);
    if (ret < 0) {
        return ret;
    }

    while (1) {
        ret = chry_sflash_nandflash_check_status(flash, &status);
        if (ret < 0) {
            return ret;
        }
        if (!(status & NANDFLASH_SR3_BUSY)) {
            if (status & NANDFLASH_SR3_PROGRAM_FAIL) {
                return -CHRY_SFLASH_ERR_IO;
            }
            if (((status & NANDFLASH_SR3_ECC_STATUS_MASK) >> NANDFLASH_SR3_ECC_STATUS_SHIFT) > 1) {
                return -CHRY_SFLASH_ERR_IO;
            }
            break;
        }
    }

    return 0;
}

int chry_sflash_nandflash_read(struct chry_sflash_nandflash *flash,
                                    uint32_t block,
                                    uint32_t page,
                                    uint8_t *buf,
                                    uint32_t buflen,
                                    uint8_t *spare,
                                    uint32_t spare_len)
{
    int ret;
    uint8_t status;
    uint8_t page_addr_buf[2];
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = true;
    command_seq.cmd_phase.cmd = flash->read_cmd;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.addr_phase.addr_mode = flash->read_addr_mode;
    command_seq.addr_phase.addr_size = CHRY_SFLASH_ADDRSIZE_16BITS;
    command_seq.dummy_phase.dummy_bytes = flash->read_dummy_bytes;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_READ;
    command_seq.data_phase.data_mode = flash->read_data_mode;

    page_addr_buf[0] = ((page + block * flash->pages_per_block) >> 8) & 0xff;
    page_addr_buf[1] = (page + block * flash->pages_per_block) & 0xff;
    ret = chry_sflash_nandflash_send_command_data(flash, NANDFLASH_COMMAND_PAGE_DATA_READ_INTO_CACHE, 1, page_addr_buf, 2);
    if (ret < 0) {
        return ret;
    }

    while (1) {
        ret = chry_sflash_nandflash_check_status(flash, &status);
        if (ret < 0) {
            return ret;
        }
        if (!(status & NANDFLASH_SR3_BUSY)) {
            break;
        }
    }

    if (buf && buflen) {
        command_seq.addr_phase.addr = 0;
        command_seq.data_phase.buf = buf;
        command_seq.data_phase.len = buflen;
        ret = chry_sflash_transfer(host, &command_seq);
        if (ret < 0) {
            return ret;
        }
    }

    if (spare && spare_len) {
        command_seq.addr_phase.addr = flash->bytes_per_page;
        command_seq.data_phase.buf = spare;
        command_seq.data_phase.len = spare_len;
        ret = chry_sflash_transfer(host, &command_seq);
        if (ret < 0) {
            return ret;
        }
    }

    while (1) {
        ret = chry_sflash_nandflash_check_status(flash, &status);
        if (ret < 0) {
            return ret;
        }
        if (!(status & NANDFLASH_SR3_BUSY)) {
            if (((status & NANDFLASH_SR3_ECC_STATUS_MASK) >> NANDFLASH_SR3_ECC_STATUS_SHIFT) > 1) {
                return -CHRY_SFLASH_ERR_IO;
            }
            break;
        }
    }

    return 0;
}
