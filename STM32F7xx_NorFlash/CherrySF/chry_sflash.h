/*
 * Copyright (c) 2024, sakumisu
 * Copyright (c) 2024, RCSN
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHRY_SFLASH_H
#define CHRY_SFLASH_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define CHRY_SFLASH_ERR_NOMEM       1
#define CHRY_SFLASH_ERR_INVAL       2
#define CHRY_SFLASH_ERR_RANGE       3
#define CHRY_SFLASH_ERR_IO          4
#define CHRY_SFLASH_ERR_TIMEOUT     5

#define CHRY_SFLASH_CMDMODE_NONE    0
#define CHRY_SFLASH_CMDMODE_1LINES  1
#define CHRY_SFLASH_CMDMODE_2LINES  2
#define CHRY_SFLASH_CMDMODE_4LINES  4
#define CHRY_SFLASH_CMDMODE_8LINES  8

#define CHRY_SFLASH_ADDRMODE_NONE   0
#define CHRY_SFLASH_ADDRMODE_1LINES 1
#define CHRY_SFLASH_ADDRMODE_2LINES 2
#define CHRY_SFLASH_ADDRMODE_4LINES 4
#define CHRY_SFLASH_ADDRMODE_8LINES 8

#define CHRY_SFLASH_ADDRSIZE_8BITS  1
#define CHRY_SFLASH_ADDRSIZE_16BITS 2
#define CHRY_SFLASH_ADDRSIZE_24BITS 3
#define CHRY_SFLASH_ADDRSIZE_32BITS 4

#define CHRY_SFLASH_DATAMODE_NONE   0
#define CHRY_SFLASH_DATAMODE_1LINES 1
#define CHRY_SFLASH_DATAMODE_2LINES 2
#define CHRY_SFLASH_DATAMODE_4LINES 4
#define CHRY_SFLASH_DATAMODE_8LINES 8

#define CHRY_SFLASH_DATA_WRITE      0
#define CHRY_SFLASH_DATA_READ       1

#define CHRY_SFLASH_FORMAT_MODE0    0
#define CHRY_SFLASH_FORMAT_MODE1    1
#define CHRY_SFLASH_FORMAT_MODE2    2
#define CHRY_SFLASH_FORMAT_MODE3    3

#define CHRY_SFLASH_IOMODE_SINGLE   0
#define CHRY_SFLASH_IOMODE_DUAL     1
#define CHRY_SFLASH_IOMODE_QUAD     2
#define CHRY_SFLASH_IOMODE_OCTAL    3

struct chry_sflash_request {
    uint16_t cs_pin;
    uint16_t freq;

    bool dma_enable;

    struct {
        uint8_t cmd;
        uint8_t cmd_mode;
    } cmd_phase;

    struct {
        uint32_t addr;
        uint8_t addr_mode;
        uint8_t addr_size;
    } addr_phase;

    struct {
        uint8_t dummy_bytes;
    } dummy_phase;

    struct {
        uint8_t direction;
        uint8_t data_mode;
        uint8_t *buf;
        uint32_t len;
    } data_phase;
};

struct chry_sflash_host {
    uint32_t spi_idx;
    uint8_t iomode;
    uint8_t format;
    void *user_data;
};

#ifdef __cplusplus
extern "C" {
#endif

int chry_sflash_init(struct chry_sflash_host *host);
int chry_sflash_deinit(struct chry_sflash_host *host);
int chry_sflash_set_frequency(struct chry_sflash_host *host, uint32_t freq);
int chry_sflash_transfer(struct chry_sflash_host *host, struct chry_sflash_request *req);

#ifdef __cplusplus
}
#endif

#endif