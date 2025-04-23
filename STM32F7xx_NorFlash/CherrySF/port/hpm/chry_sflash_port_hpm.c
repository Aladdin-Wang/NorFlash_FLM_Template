/*
 * Copyright (c) 2024, sakumisu
 * Copyright (c) 2024, RCSN
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "chry_sflash.h"

#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"
#include "hpm_spi_drv.h"
#include "hpm_l1c_drv.h"
#include "hpm_clock_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_spi.h"
#include "board.h"

typedef struct {
    uint8_t rx_dma_ch;
    uint8_t tx_dma_ch;
    uint8_t rx_dma_req;
    uint8_t tx_dma_req;
    void *dma_base;
    void *dmamux_base;
} hpm_spi_dma_config_t;

typedef struct {
    hpm_spi_dma_config_t dma_control;
    uint32_t transfer_max_size;
    SPI_Type *host_base;
    uint16_t cs_pin;
} hpm_spi_config_t;

#include "hpm_spi_config.h"

static hpm_stat_t spi_nor_tx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num, SPI_Type *spi_ptr,
                                         uint32_t src, uint8_t data_width, uint32_t size, uint8_t burst_size)
{
    hpm_stat_t stat;
    dma_channel_config_t config;
    if (ch_num >= DMA_SOC_CHANNEL_NUM) {
        return status_invalid_argument;
    }
    dma_default_channel_config(dma_ptr, &config);
    config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    config.dst_mode = DMA_HANDSHAKE_MODE_HANDSHAKE;
    config.src_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
    config.src_mode = DMA_HANDSHAKE_MODE_NORMAL;
    config.src_width = data_width;
    config.dst_width = data_width;
    config.src_addr = src;
    config.dst_addr = (uint32_t)&spi_ptr->DATA;
    config.size_in_byte = size;
    config.src_burst_size = burst_size;
    stat = dma_setup_channel(dma_ptr, ch_num, &config, true);
    if (stat != status_success) {
        return stat;
    }
    return stat;
}

static hpm_stat_t spi_nor_rx_trigger_dma(DMA_Type *dma_ptr, uint8_t ch_num, SPI_Type *spi_ptr, uint32_t dst,
                                         uint8_t data_width, uint32_t size, uint8_t burst_size)
{
    hpm_stat_t stat;
    dma_channel_config_t config;
    if (ch_num >= DMA_SOC_CHANNEL_NUM) {
        return status_invalid_argument;
    }
    dma_default_channel_config(dma_ptr, &config);
    config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_INCREMENT;
    config.dst_mode = DMA_HANDSHAKE_MODE_HANDSHAKE;
    config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
    config.src_mode = DMA_HANDSHAKE_MODE_NORMAL;
    config.src_width = data_width;
    config.dst_width = data_width;
    config.src_addr = (uint32_t)&spi_ptr->DATA;
    config.dst_addr = dst;
    config.size_in_byte = size;
    config.src_burst_size = burst_size;
    stat = dma_setup_channel(dma_ptr, ch_num, &config, true);
    if (stat != status_success) {
        return stat;
    }
    return stat;
}

static void hpm_config_cmd_addr_format(hpm_spi_config_t *config, struct chry_sflash_request *cmd_seq, spi_control_config_t *control_config)
{
    spi_trans_mode_t _trans_mode;
    control_config->master_config.cmd_enable = true;

    /* judge the valid of addr */
    if (cmd_seq->addr_phase.addr_mode != CHRY_SFLASH_ADDRMODE_NONE) {
        control_config->master_config.addr_enable = true;
        if (cmd_seq->addr_phase.addr_mode == CHRY_SFLASH_ADDRMODE_1LINES) {
            control_config->master_config.addr_phase_fmt = spi_address_phase_format_single_io_mode;
        } else {
            control_config->master_config.addr_phase_fmt = spi_address_phase_format_dualquad_io_mode;
        }
        spi_set_address_len((SPI_Type *)config->host_base, cmd_seq->addr_phase.addr_size - 1);
    } else {
        control_config->master_config.addr_enable = false;
    }

    /* judge the valid of buf */
    if ((cmd_seq->data_phase.buf != NULL) || (cmd_seq->data_phase.len != 0)) {
        if (cmd_seq->dummy_phase.dummy_bytes == 0) {
            _trans_mode = (cmd_seq->data_phase.direction == CHRY_SFLASH_DATA_READ) ? spi_trans_read_only : spi_trans_write_only;
        } else {
            if (cmd_seq->addr_phase.addr_mode == CHRY_SFLASH_ADDRMODE_NONE) {
                control_config->common_config.dummy_cnt = cmd_seq->dummy_phase.dummy_bytes - 1;
            } else {
                control_config->common_config.dummy_cnt = ((cmd_seq->dummy_phase.dummy_bytes * 8 / cmd_seq->data_phase.data_mode) * cmd_seq->addr_phase.addr_mode / 8) - 1;
            }

            _trans_mode = (cmd_seq->data_phase.direction == CHRY_SFLASH_DATA_READ) ? spi_trans_dummy_read : spi_trans_dummy_write;
        }
        control_config->common_config.trans_mode = _trans_mode;

        if ((cmd_seq->data_phase.data_mode == CHRY_SFLASH_DATAMODE_1LINES)) {
            control_config->common_config.data_phase_fmt = spi_single_io_mode;
        } else if (cmd_seq->data_phase.data_mode == CHRY_SFLASH_DATAMODE_2LINES) {
            control_config->common_config.data_phase_fmt = spi_dual_io_mode;
        } else {
            control_config->common_config.data_phase_fmt = spi_quad_io_mode;
        }
    } else {
        control_config->common_config.trans_mode = spi_trans_no_data;
    }
}

static hpm_stat_t init(hpm_spi_config_t *config)
{
    spi_format_config_t format_config = { 0 };
    if ((config == NULL) || (config->host_base == NULL)) {
        return status_invalid_argument;
    }
    spi_master_get_default_format_config(&format_config);
    format_config.common_config.data_len_in_bits = 8;
    format_config.common_config.mode = spi_master_mode;
    format_config.common_config.cpol = spi_sclk_low_idle;
    format_config.common_config.cpha = spi_sclk_sampling_odd_clk_edges;
    format_config.common_config.data_merge = false;
    spi_format_init(config->host_base, &format_config);

    if ((config->dma_control.dma_base == NULL) || (config->dma_control.dmamux_base == NULL)) {
        return status_invalid_argument;
    }
    dmamux_config(config->dma_control.dmamux_base,
                  DMA_SOC_CHN_TO_DMAMUX_CHN(config->dma_control.dma_base,
                                            config->dma_control.rx_dma_ch),
                  config->dma_control.rx_dma_req, true);

    dmamux_config(config->dma_control.dmamux_base,
                  DMA_SOC_CHN_TO_DMAMUX_CHN(config->dma_control.dma_base,
                                            config->dma_control.tx_dma_ch),
                  config->dma_control.tx_dma_req, true);

    return status_success;
}

static hpm_stat_t hpm_spi_transfer_via_dma(hpm_spi_config_t *config, spi_control_config_t *control_config,
                                           uint8_t cmd, uint32_t addr,
                                           uint8_t *buf, uint32_t len, bool is_read)
{
    hpm_stat_t stat;
    uint32_t data_width = 0;
    uint8_t burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
    uint32_t timeout_count = 0;
    uint16_t dma_send_size;
    if (is_read) {
        /*The supplement of the byte less than the integer multiple of four bytes is an integer multiple of four bytes to DMA*/
        data_width = DMA_TRANSFER_WIDTH_WORD;
        if ((len % 4) == 0) {
            dma_send_size = len;
        } else {
            dma_send_size = ((len >> 2) + 1) << 2;
        }
        stat = spi_setup_dma_transfer((SPI_Type *)config->host_base, control_config, &cmd, &addr, 0, len);
        stat = spi_nor_rx_trigger_dma((DMA_Type *)config->dma_control.dma_base,
                                      config->dma_control.rx_dma_ch,
                                      (SPI_Type *)config->host_base,
                                      core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)buf),
                                      data_width,
                                      dma_send_size, burst_size);
        while (spi_is_active((SPI_Type *)config->host_base)) {
            timeout_count++;
            if (timeout_count >= 0xFFFFFF) {
                stat = status_timeout;
                break;
            }
        }
        timeout_count = 0;
        if ((dma_check_transfer_status(
                 (DMA_Type *)config->dma_control.dma_base,
                 config->dma_control.rx_dma_ch) &&
             DMA_CHANNEL_STATUS_TC) == 0) {
            dma_disable_channel((DMA_Type *)config->dma_control.dma_base, config->dma_control.rx_dma_ch);
            dma_reset((DMA_Type *)config->dma_control.dma_base);
        }
    } else {
        if ((len % 4) == 0) {
            spi_enable_data_merge((SPI_Type *)config->host_base);
            data_width = DMA_TRANSFER_WIDTH_WORD;
        } else {
            data_width = DMA_TRANSFER_WIDTH_BYTE;
        }
        spi_set_tx_fifo_threshold((SPI_Type *)config->host_base, 3);
        burst_size = DMA_NUM_TRANSFER_PER_BURST_1T;
        stat = spi_setup_dma_transfer((SPI_Type *)config->host_base, control_config, &cmd, &addr, len, 0);

        stat = spi_nor_tx_trigger_dma((DMA_Type *)config->dma_control.dma_base,
                                      config->dma_control.tx_dma_ch,
                                      (SPI_Type *)config->host_base,
                                      core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)buf),
                                      data_width, len, burst_size);

        while (spi_is_active((SPI_Type *)config->host_base)) {
            timeout_count++;
            if (timeout_count >= 0xFFFFFF) {
                stat = status_timeout;
                break;
            }
        }
        timeout_count = 0;
        spi_disable_data_merge((SPI_Type *)config->host_base);
    }
    return stat;
}

static hpm_stat_t write(hpm_spi_config_t *config, struct chry_sflash_request *cmd_seq)
{
    hpm_stat_t stat = status_success;
    spi_control_config_t control_config = { 0 };
    uint32_t aligned_start;
    uint32_t aligned_end;
    uint32_t aligned_size;
    if ((cmd_seq->data_phase.len > config->transfer_max_size) || (config == NULL) || (config->host_base == NULL)) {
        return status_invalid_argument;
    }

    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(config->cs_pin), GPIO_GET_PIN_INDEX(config->cs_pin), false);

    spi_master_get_default_control_config(&control_config);
    hpm_config_cmd_addr_format(config, cmd_seq, &control_config);

    if (cmd_seq->dma_enable == 1) {
        control_config.common_config.tx_dma_enable = true;
        control_config.common_config.rx_dma_enable = false;
        if (l1c_dc_is_enabled()) {
            /* cache writeback for sent buff */
            aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)cmd_seq->data_phase.buf);
            aligned_end = HPM_L1C_CACHELINE_ALIGN_UP((uint32_t)cmd_seq->data_phase.buf + cmd_seq->data_phase.len);
            aligned_size = aligned_end - aligned_start;
            l1c_dc_writeback(aligned_start, aligned_size);
        }
        stat = hpm_spi_transfer_via_dma(config, &control_config, cmd_seq->cmd_phase.cmd, cmd_seq->addr_phase.addr,
                                        (uint8_t *)cmd_seq->data_phase.buf, cmd_seq->data_phase.len, false);
    } else {
        stat = spi_transfer((SPI_Type *)config->host_base, &control_config,
                            &cmd_seq->cmd_phase.cmd, &cmd_seq->addr_phase.addr,
                            cmd_seq->data_phase.buf, cmd_seq->data_phase.len, NULL, 0);
    }

    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(config->cs_pin), GPIO_GET_PIN_INDEX(config->cs_pin), true);

    return stat;
}

static hpm_stat_t read(hpm_spi_config_t *config, struct chry_sflash_request *cmd_seq)
{
    hpm_stat_t stat = status_success;
    uint32_t aligned_start;
    uint32_t aligned_end;
    uint32_t aligned_size;
    spi_control_config_t control_config = { 0 };
    uint32_t read_size = 0;
    uint32_t read_start = cmd_seq->addr_phase.addr;
    uint8_t *dst_8 = (uint8_t *)cmd_seq->data_phase.buf;
    uint32_t remaining_len = cmd_seq->data_phase.len;

    if ((config == NULL) || (config->host_base == NULL)) {
        return status_invalid_argument;
    }

    spi_master_get_default_control_config(&control_config);
    hpm_config_cmd_addr_format(config, cmd_seq, &control_config);

    if (cmd_seq->dma_enable == 1) {
        if (config->dma_control.dma_base == NULL) {
            return status_fail;
        }
        // if (((uint32_t)dst_8 % HPM_L1C_CACHELINE_SIZE) != 0) {
        //     return status_invalid_argument;
        // }
        control_config.common_config.tx_dma_enable = false;
        control_config.common_config.rx_dma_enable = true;
    }

    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(config->cs_pin), GPIO_GET_PIN_INDEX(cmd_seq->cs_pin), false);

    while (remaining_len > 0U) {
        read_size = MIN(remaining_len, config->transfer_max_size);
        if (cmd_seq->dma_enable == 1) {
            spi_enable_data_merge((SPI_Type *)config->host_base);
            stat = hpm_spi_transfer_via_dma(config, &control_config, cmd_seq->cmd_phase.cmd, read_start, dst_8, read_size, true);
        } else {
            stat = spi_transfer((SPI_Type *)config->host_base, &control_config, &cmd_seq->cmd_phase.cmd,
                                &read_start, NULL, 0, dst_8, read_size);
        }
        HPM_BREAK_IF(stat != status_success);
        if (l1c_dc_is_enabled()) {
            /* cache invalidate for receive buff */
            aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN((uint32_t)dst_8);
            aligned_end = HPM_L1C_CACHELINE_ALIGN_UP(read_size);
            aligned_size = aligned_end - aligned_start;
            l1c_dc_invalidate(aligned_start, aligned_size);
        }
        read_start += read_size;
        remaining_len -= read_size;
        dst_8 += read_size;
    }

    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(config->cs_pin), GPIO_GET_PIN_INDEX(config->cs_pin), true);
    spi_disable_data_merge((SPI_Type *)config->host_base);
    return stat;
}

static hpm_stat_t transfer(hpm_spi_config_t *config, struct chry_sflash_request *command_seq)
{
    hpm_stat_t stat = status_success;
    if (command_seq->data_phase.direction == CHRY_SFLASH_DATA_READ) {
        stat = read(config, command_seq);
    } else {
        stat = write(config, command_seq);
    }
    return stat;
}

hpm_spi_config_t spi_config;

int chry_sflash_init(struct chry_sflash_host *host)
{
    spi_host_init(&spi_config);

    init(&spi_config);

    return 0;
}

int chry_sflash_set_frequency(struct chry_sflash_host *host, uint32_t freq)
{
    hpm_spi_set_sclk_frequency(spi_config.host_base, freq);
    return 0;
}

int chry_sflash_transfer(struct chry_sflash_host *host, struct chry_sflash_request *req)
{
    hpm_stat_t ret = transfer(&spi_config, req);
    if (ret == status_success)
        return 0;
    else
        return -1;
}

int chry_sflash_deinit(struct chry_sflash_host *host)
{
    return 0;
}