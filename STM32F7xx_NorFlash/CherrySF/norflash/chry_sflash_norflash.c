/*
 * Copyright (c) 2024, sakumisu
 * Copyright (c) 2024, RCSN
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "chry_sflash_norflash.h"

#define MAX_24BIT_ADDRESSING_SIZE ((1UL << 24))

/**
 * @brief QE bit enable sequence option
 */
typedef enum {
    spi_nor_quad_en_auto_or_ignore = 0U,                      /**< Auto enable or ignore */
    spi_nor_quad_en_set_bit6_in_status_reg1 = 1U,             /**< QE bit is at bit6 in Status register 1 */
    spi_nor_quad_en_set_bit1_in_status_reg2 = 2U,             /**< QE bit is at bit1 in Status register 2 register 2 */
    spi_nor_quad_en_set_bit7_in_status_reg2 = 3U,             /**< QE bit is at bit7 in Status register 2 */
    spi_nor_quad_en_set_bi1_in_status_reg2_via_0x31_cmd = 4U, /**< QE bit is in status register 2 and configured by CMD 0x31 */
} spi_nor_quad_enable_seq_t;

static inline int chry_sflash_norflash_read_sfdp(struct chry_sflash_norflash *flash, uint32_t addr, uint8_t *buffer, uint32_t buflen)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = false;
    command_seq.cmd_phase.cmd = NORFLASH_COMMAND_READ_SFDP;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.addr_phase.addr = addr;
    command_seq.addr_phase.addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
    command_seq.addr_phase.addr_size = CHRY_SFLASH_ADDRSIZE_24BITS;
    command_seq.dummy_phase.dummy_bytes = 1;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_READ;
    command_seq.data_phase.data_mode = CHRY_SFLASH_DATAMODE_1LINES;
    command_seq.data_phase.buf = buffer;
    command_seq.data_phase.len = buflen;

    return chry_sflash_transfer(host, &command_seq);
}

static inline int chry_sflash_norflash_send_command(struct chry_sflash_norflash *flash, uint8_t command)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = false;
    command_seq.cmd_phase.cmd = command;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;

    return chry_sflash_transfer(host, &command_seq);
}

static inline int chry_sflash_norflash_read_status_register(struct chry_sflash_norflash *flash, uint8_t command, uint8_t *reg_data)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = false;
    command_seq.cmd_phase.cmd = command;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.data_phase.data_mode = CHRY_SFLASH_DATAMODE_1LINES;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_READ;
    command_seq.data_phase.buf = reg_data;
    command_seq.data_phase.len = sizeof(uint8_t);

    return chry_sflash_transfer(host, &command_seq);
}

static inline int chry_sflash_norflash_is_busy(struct chry_sflash_norflash *flash, bool *busy)
{
    uint8_t sr = 0;
    int ret;

    ret = chry_sflash_norflash_read_status_register(flash, NORFLASH_COMMAND_READ_STATUS_REG1, &sr);
    if (ret < 0) {
        return ret;
    }
    *busy = (sr & 0b1) ? true : false;
    return 0;
}

static int chry_sflash_norflash_write_status_register(struct chry_sflash_norflash *flash, uint8_t command, uint8_t reg_data)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };
    int ret;
    bool busy;

    while (1) {
        ret = chry_sflash_norflash_is_busy(flash, &busy);
        if (ret < 0) {
            return ret;
        }
        if (!busy) {
            break;
        }
    }

    ret = chry_sflash_norflash_send_command(flash, NORFLASH_COMMAND_WRITE_ENABLE);
    if (ret < 0) {
        return ret;
    }

    command_seq.dma_enable = false;
    command_seq.cmd_phase.cmd = command;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.data_phase.data_mode = CHRY_SFLASH_DATAMODE_1LINES;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_WRITE;
    command_seq.data_phase.buf = &reg_data;
    command_seq.data_phase.len = sizeof(uint8_t);

    ret = chry_sflash_transfer(host, &command_seq);
    if (ret < 0) {
        return ret;
    }

    while (1) {
        ret = chry_sflash_norflash_is_busy(flash, &busy);
        if (ret < 0) {
            return ret;
        }
        if (!busy) {
            break;
        }
    }
    return 0;
}

static int chry_sflash_norflash_read_sfdp_info(struct chry_sflash_norflash *flash, struct chry_sflash_norflash_jedec_info *jedec_info)
{
#define SFDP_PARAMETER_NPH       (10U)
#define SFDP_PARAMETER_TABLE_LEN (23U)
    sfdp_header_t sfdp_header;
    sfdp_parameter_header_t sfdp_param_hdrs[SFDP_PARAMETER_NPH];
    uint8_t param_table[SFDP_PARAMETER_TABLE_LEN * 4];
    uint32_t address;
    uint32_t parameter_header_number;
    uint32_t parameter_id;
    uint32_t table_size;
    int ret;

    address = 0;
    ret = chry_sflash_norflash_read_sfdp(flash, address, (uint8_t *)&sfdp_header, sizeof(sfdp_header));
    if (ret < 0) {
        return ret;
    }

    if (sfdp_header.signature != SFDP_SIGNATURE) {
        return -1;
    }

    parameter_header_number = sfdp_header.nph + 1;
    flash->sfdp_major_version = sfdp_header.major_rev;
    flash->sfdp_minor_version = sfdp_header.minor_rev;

    //printf("SFDP nph %d\r\n", parameter_header_number);

    memset(&sfdp_param_hdrs, 0, sizeof(sfdp_param_hdrs));
    parameter_header_number = parameter_header_number > 10U ? 10U : parameter_header_number;

    address = 0x08U;
    ret = chry_sflash_norflash_read_sfdp(flash, address, (uint8_t *)&sfdp_param_hdrs[0], parameter_header_number * sizeof(sfdp_parameter_header_t));
    if (ret < 0) {
        return ret;
    }

    for (uint16_t i = 0; i < parameter_header_number; i++) {
        parameter_id = sfdp_param_hdrs[i].parameter_id_lsb + ((uint32_t)sfdp_param_hdrs[i].parameter_id_msb << 8);

        address = (uint32_t)(sfdp_param_hdrs[i].parameter_table_pointer[0] << 0) |
                  (uint32_t)(sfdp_param_hdrs[i].parameter_table_pointer[1] << 8) |
                  (uint32_t)(sfdp_param_hdrs[i].parameter_table_pointer[2] << 16);

        table_size = sfdp_param_hdrs[i].parameter_table_length * sizeof(uint32_t);

        //printf("Parameter ID 0x%04X, address 0x%06X, table size %d\r\n", parameter_id, address, table_size);

        table_size = table_size > sizeof(param_table) ? sizeof(param_table) : table_size;
        ret = chry_sflash_norflash_read_sfdp(flash, address, param_table, table_size);
        if (ret < 0) {
            return ret;
        }

//        printf("JEDEC basic flash parameter table info:\r\n");
//        printf("MSB-LSB  3    2    1    0\r\n");
//        for (uint32_t j = 0; j < sfdp_param_hdrs[i].parameter_table_length; j++) {
//            printf("[%04d] 0x%02X 0x%02X 0x%02X 0x%02X\r\n", j + 1,
//                   param_table[j * 4 + 3],
//                   param_table[j * 4 + 2],
//                   param_table[j * 4 + 1],
//                   param_table[j * 4]);
//        }

        switch (parameter_id) {
            case SFDP_PARAMETER_ID_BASIC_SPI_PROTOCOL:
                jedec_info->basic_flash_param_table_size = table_size;
                memcpy(&jedec_info->basic_flash_param_table, param_table, table_size);
                break;
            case SFDP_PARAMETER_ID_4BYTEADDRESS_INSTRUCTION_TABLE:
                jedec_info->jedec_4byte_addressing_inst_table_enable = true;
                memcpy(&jedec_info->jedec_4byte_addressing_inst_table, param_table, table_size);
                break;

            default:
               // printf("Unknown parameter ID 0x%04X\r\n", parameter_id);
                break;
        }
    }
    return 0;
}

static void chry_sflash_norflash_parse_page_program_para(struct chry_sflash_norflash *flash, struct chry_sflash_norflash_jedec_info *jedec_info)
{
    if (flash->addr_size == CHRY_SFLASH_ADDRSIZE_24BITS) {
        flash->page_program_cmd = (flash->host->iomode == CHRY_SFLASH_IOMODE_QUAD) ?
                                      NORFLASH_COMMAND_PAGE_PROGRAM_1_1_4_3B :
                                      NORFLASH_COMMAND_PAGE_PROGRAM_1_1_1_3B;

        flash->page_program_addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
        flash->page_program_data_mode = (flash->host->iomode == CHRY_SFLASH_IOMODE_QUAD) ?
                                            CHRY_SFLASH_DATAMODE_4LINES :
                                            CHRY_SFLASH_DATAMODE_1LINES;

    } else {
        if ((flash->host->iomode == CHRY_SFLASH_IOMODE_QUAD) && jedec_info->jedec_4byte_addressing_inst_table_enable) {
            if (jedec_info->jedec_4byte_addressing_inst_table.dword1.support_1_4_4_page_program) {
            } else if (jedec_info->jedec_4byte_addressing_inst_table.dword1.support_1_1_4_page_program) {
                flash->page_program_cmd = NORFLASH_COMMAND_PAGE_PROGRAM_1_1_4_4B;
                flash->page_program_addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
                flash->page_program_data_mode = CHRY_SFLASH_DATAMODE_4LINES;
            } else {
                flash->page_program_cmd = NORFLASH_COMMAND_PAGE_PROGRAM_1_1_1_4B;
                flash->page_program_addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
                flash->page_program_data_mode = CHRY_SFLASH_DATAMODE_1LINES;
            }
        } else {
            flash->page_program_cmd = NORFLASH_COMMAND_PAGE_PROGRAM_1_1_1_4B;
            flash->page_program_addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
            flash->page_program_data_mode = CHRY_SFLASH_DATAMODE_1LINES;
        }
    }
}

static void chry_sflash_norflash_parse_read_para(struct chry_sflash_norflash *flash, struct chry_sflash_norflash_jedec_info *jedec_info)
{
    uint32_t read_cmd;
    uint8_t addr_mode;
    uint8_t data_mode;
    uint8_t dummy_clocks;
    uint8_t mode_clocks;

    addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
    data_mode = CHRY_SFLASH_DATAMODE_4LINES;
    mode_clocks = 0;
    dummy_clocks = 0;

    if (flash->host->iomode == CHRY_SFLASH_IOMODE_QUAD) {
        if (flash->addr_size == CHRY_SFLASH_ADDRSIZE_24BITS) {
            if (jedec_info->basic_flash_param_table.dword3.inst_1_4_4_fast_read != 0U) {
                read_cmd = jedec_info->basic_flash_param_table.dword3.inst_1_4_4_fast_read;
                addr_mode = CHRY_SFLASH_ADDRMODE_4LINES;
            } else if (jedec_info->basic_flash_param_table.dword3.inst_1_1_4_fast_read != 0U) {
                read_cmd = jedec_info->basic_flash_param_table.dword3.inst_1_1_4_fast_read;
            } else {
                read_cmd = NORFLASH_COMMAND_READ_1_1_1_3B;
                data_mode = CHRY_SFLASH_DATAMODE_1LINES;

                //printf("Do not find quad mode read command, use 1-1-1 read command\r\n");
            }
        } else {
            if (jedec_info->jedec_4byte_addressing_inst_table_enable) {
                if (jedec_info->jedec_4byte_addressing_inst_table.dword1.support_1_4_4_fast_read != 0U) {
                    read_cmd = NORFLASH_COMMAND_FAST_READ_1_4_4_4B;
                    addr_mode = CHRY_SFLASH_ADDRMODE_4LINES;
                } else if (jedec_info->jedec_4byte_addressing_inst_table.dword1.support_1_1_4_fast_read != 0U) {
                    read_cmd = NORFLASH_COMMAND_FAST_READ_1_4_4_4B;
                } else {
                    read_cmd = NORFLASH_COMMAND_READ_1_1_1_4B;
                    data_mode = CHRY_SFLASH_DATAMODE_1LINES;

                    //printf("Do not find quad mode read command, use 1-1-1 read command\r\n");
                }
            } else if (jedec_info->basic_flash_param_table.dword1.support_1_4_4_fast_read != 0U) {
                read_cmd = NORFLASH_COMMAND_FAST_READ_1_4_4_4B;
                addr_mode = CHRY_SFLASH_ADDRMODE_4LINES;
            } else if (jedec_info->basic_flash_param_table.dword1.support_1_1_4_fast_read != 0U) {
                read_cmd = NORFLASH_COMMAND_FAST_READ_1_4_4_4B;
            } else {
                read_cmd = NORFLASH_COMMAND_READ_1_1_1_3B;
                data_mode = CHRY_SFLASH_DATAMODE_1LINES;

               // printf("Do not find quad mode read command, use 1-1-1 read command\r\n");
            }
        }
    } else if (flash->host->iomode == CHRY_SFLASH_IOMODE_DUAL) {
        if (flash->addr_size == CHRY_SFLASH_ADDRSIZE_24BITS) {
            if (jedec_info->basic_flash_param_table.dword1.support_1_2_2_fast_read != 0U) {
                read_cmd = jedec_info->basic_flash_param_table.dword4.inst_1_2_2_fast_read;
                data_mode = CHRY_SFLASH_DATAMODE_2LINES;
                addr_mode = CHRY_SFLASH_ADDRMODE_2LINES;
            } else if (jedec_info->basic_flash_param_table.dword1.support_1_1_2_fast_read != 0U) {
                read_cmd = jedec_info->basic_flash_param_table.dword4.inst_1_1_2_fast_read;
                data_mode = CHRY_SFLASH_DATAMODE_2LINES;
            } else {
                read_cmd = NORFLASH_COMMAND_READ_1_1_1_3B;
                data_mode = CHRY_SFLASH_DATAMODE_1LINES;

                //printf("Do not find qual mode read command, use 1-1-1 read command\r\n");
            }
        } else {
            if (jedec_info->jedec_4byte_addressing_inst_table_enable) {
                if (jedec_info->jedec_4byte_addressing_inst_table.dword1.support_1_2_2_fast_read != 0U) {
                    read_cmd = NORFLASH_COMMAND_FAST_READ_1_2_2_4B;
                    data_mode = CHRY_SFLASH_DATAMODE_2LINES;
                    addr_mode = CHRY_SFLASH_ADDRMODE_2LINES;
                } else if (jedec_info->jedec_4byte_addressing_inst_table.dword1.support_1_1_2_fast_read != 0U) {
                    read_cmd = NORFLASH_COMMAND_FAST_READ_1_1_2_4B;
                    data_mode = CHRY_SFLASH_DATAMODE_2LINES;
                } else {
                    read_cmd = NORFLASH_COMMAND_READ_1_1_1_4B;
                    data_mode = CHRY_SFLASH_DATAMODE_1LINES;

                    //printf("Do not find dual mode read command, use 1-1-1 read command\r\n");
                }
            } else if (jedec_info->basic_flash_param_table.dword1.support_1_2_2_fast_read != 0U) {
                read_cmd = NORFLASH_COMMAND_FAST_READ_1_2_2_4B;
                data_mode = CHRY_SFLASH_DATAMODE_2LINES;
                addr_mode = CHRY_SFLASH_ADDRMODE_2LINES;
            } else if (jedec_info->basic_flash_param_table.dword1.support_1_1_2_fast_read != 0U) {
                read_cmd = NORFLASH_COMMAND_FAST_READ_1_1_2_4B;
                data_mode = CHRY_SFLASH_DATAMODE_2LINES;
            } else {
                read_cmd = NORFLASH_COMMAND_READ_1_1_1_3B;
                data_mode = CHRY_SFLASH_DATAMODE_1LINES;

                //printf("Do not find dual mode read command, use 1-1-1 read command\r\n");
            }
        }
    } else {
        if (flash->addr_size == CHRY_SFLASH_ADDRSIZE_24BITS) {
            read_cmd = NORFLASH_COMMAND_READ_1_1_1_3B;
            data_mode = CHRY_SFLASH_DATAMODE_1LINES;
        } else {
            if (jedec_info->jedec_4byte_addressing_inst_table_enable && jedec_info->jedec_4byte_addressing_inst_table.dword1.support_1_1_1_fast_read) {
                read_cmd = NORFLASH_COMMAND_FAST_READ_1_1_1_4B;
                data_mode = CHRY_SFLASH_DATAMODE_1LINES;
            } else {
                read_cmd = NORFLASH_COMMAND_READ_1_1_1_4B;
                data_mode = CHRY_SFLASH_DATAMODE_1LINES;
            }
        }
    }

    if (flash->host->iomode == CHRY_SFLASH_IOMODE_QUAD) {
        if (jedec_info->basic_flash_param_table.dword1.support_1_4_4_fast_read != 0U) {
            mode_clocks = jedec_info->basic_flash_param_table.dword3.mode_clocks_1_4_4_fast_read;
            dummy_clocks = jedec_info->basic_flash_param_table.dword3.dummy_clocks_1_4_4_fast_read;
            flash->read_dummy_bytes = (dummy_clocks + mode_clocks) * 4 / 8;
        } else if (jedec_info->basic_flash_param_table.dword1.support_1_1_4_fast_read != 0U) {
            mode_clocks = jedec_info->basic_flash_param_table.dword3.mode_clocks_1_1_4_fast_read;
            dummy_clocks = jedec_info->basic_flash_param_table.dword3.dummy_clocks_1_1_4_fast_read;
            flash->read_dummy_bytes = (dummy_clocks + mode_clocks) * 1 / 8;
        } else {
            flash->read_dummy_bytes = 1;
        }
    } else if (flash->host->iomode == CHRY_SFLASH_IOMODE_DUAL) {
        if (jedec_info->basic_flash_param_table.dword1.support_1_2_2_fast_read != 0U) {
            mode_clocks = jedec_info->basic_flash_param_table.dword4.mode_clocks_1_2_2_fast_read;
            dummy_clocks = jedec_info->basic_flash_param_table.dword4.dummy_clocks_1_2_2_fast_read;
            flash->read_dummy_bytes = (dummy_clocks + mode_clocks) * 2 / 8;
        } else if (jedec_info->basic_flash_param_table.dword1.support_1_1_2_fast_read != 0U) {
            mode_clocks = jedec_info->basic_flash_param_table.dword4.mode_clocks_1_1_2_fast_read;
            dummy_clocks = jedec_info->basic_flash_param_table.dword4.dummy_clocks_1_1_2_fast_read;
            flash->read_dummy_bytes = (dummy_clocks + mode_clocks) * 1 / 8;
        } else {
            flash->read_dummy_bytes = 1;
        }
    } else {
        flash->read_dummy_bytes = 1;
    }

    flash->read_cmd = read_cmd;
    flash->read_addr_mode = addr_mode;
    flash->read_data_mode = data_mode;
}

static int chry_sflash_norflash_enter_quad_mode(struct chry_sflash_norflash *flash, struct chry_sflash_norflash_jedec_info *jedec_info)
{
    uint8_t status_val = 0;
    uint8_t read_status_reg = 0;
    uint8_t write_status_reg = 0;
    int ret;
    spi_nor_quad_enable_seq_t enter_quad_mode_option = spi_nor_quad_en_auto_or_ignore;

    if ((flash->sfdp_minor_version >= SFDP_VERSION_MINOR_A) || (jedec_info->basic_flash_param_table_size >= SFDP_BASIC_PROTOCOL_TABLE_SIZE_REVA)) {
        switch (jedec_info->basic_flash_param_table.dword15.quad_enable_requirement) {
            case 1:
            case 4:
            case 5:
                enter_quad_mode_option = spi_nor_quad_en_set_bit1_in_status_reg2;
                break;
            case 6:
                enter_quad_mode_option = spi_nor_quad_en_set_bi1_in_status_reg2_via_0x31_cmd;
                break;
            case 2:
                enter_quad_mode_option = spi_nor_quad_en_set_bit6_in_status_reg1;
                break;
            case 3:
                enter_quad_mode_option = spi_nor_quad_en_set_bit7_in_status_reg2;
                break;
            default:
                enter_quad_mode_option = spi_nor_quad_en_auto_or_ignore;
                break;
        }

        if (enter_quad_mode_option != spi_nor_quad_en_auto_or_ignore) {
            switch (enter_quad_mode_option) {
                case spi_nor_quad_en_set_bit1_in_status_reg2:
                case spi_nor_quad_en_set_bi1_in_status_reg2_via_0x31_cmd:
                    read_status_reg = NORFLASH_COMMAND_READ_STATUS_REG2;
                    break;
                case spi_nor_quad_en_set_bit6_in_status_reg1:
                    read_status_reg = NORFLASH_COMMAND_READ_STATUS_REG1;
                    break;
                case spi_nor_quad_en_set_bit7_in_status_reg2:
                    read_status_reg = NORFLASH_COMMAND_READ_STATUS_REG2;
                    break;
                default:
                    /* Reserved for future use */
                    break;
            }

            ret = chry_sflash_norflash_read_status_register(flash, read_status_reg, &status_val);
            if (ret < 0) {
                return ret;
            }
            /* Do modify-after-read status and then create Quad mode Enable sequence
             * Enable QE bit only if it is not enabled.
             */
            switch (enter_quad_mode_option) {
                case spi_nor_quad_en_set_bit6_in_status_reg1:
                    if (!(status_val & (1 << 6))) {
                        write_status_reg = NORFLASH_COMMAND_WRITE_STATUS_REG1;
                        status_val &= (uint8_t)~0x3cU; /* Clear Block protection */
                        status_val |= (1 << 6);
                        ret = chry_sflash_norflash_write_status_register(flash, status_val, write_status_reg);
                        if (ret < 0) {
                            return ret;
                        }
                    }
                    break;
                case spi_nor_quad_en_set_bit1_in_status_reg2:
                    if (!(status_val & (1 << 1))) {
                        write_status_reg = NORFLASH_COMMAND_WRITE_STATUS_REG1;
                        status_val |= (1 << 1);
                        /* QE bit will be programmed after status1 register, so need to left shit 8 bit */
                        status_val <<= 8;
                        ret = chry_sflash_norflash_write_status_register(flash, status_val, write_status_reg);
                        if (ret < 0) {
                            return ret;
                        }
                    }
                    break;
                case spi_nor_quad_en_set_bi1_in_status_reg2_via_0x31_cmd:
                    if (!(status_val & (1 << 1))) {
                        write_status_reg = NORFLASH_COMMAND_WRITE_STATUS_REG2;
                        status_val |= (1 << 1);
                        ret = chry_sflash_norflash_write_status_register(flash, status_val, write_status_reg);
                        if (ret < 0) {
                            return ret;
                        }
                    }
                    break;
                case spi_nor_quad_en_set_bit7_in_status_reg2:
                    if (!(status_val & (1 << 7))) {
                        write_status_reg = NORFLASH_COMMAND_WRITE_STATUS_REG2;
                        status_val |= (1 << 7);
                        ret = chry_sflash_norflash_write_status_register(flash, status_val, write_status_reg);
                        if (ret < 0) {
                            return ret;
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return 0;
}

int chry_sflash_norflash_init(struct chry_sflash_norflash *flash, struct chry_sflash_host *host)
{
    struct chry_sflash_norflash_jedec_info jedec_info;
    uint32_t sector_size = 0xffffffff;
    uint32_t block_size = 0U;
    uint32_t block_erase_type = 0U;
    uint32_t sector_erase_type = 0U;
    int ret;

    memset(&jedec_info, 0, sizeof(jedec_info));
    memset(flash, 0, sizeof(struct chry_sflash_norflash));

    flash->host = host;

    chry_sflash_set_frequency(flash->host, SFDP_READ_FREQUENCY);
    ret = chry_sflash_norflash_read_sfdp_info(flash, &jedec_info);
    if (ret < 0) {
        return ret;
    }

    if (jedec_info.basic_flash_param_table.dword2.flash_memory_density & (1 << 31)) {
        /* Flash size >= 4G bits */
        flash->flash_size = 1UL << ((jedec_info.basic_flash_param_table.dword2.flash_memory_density & ~(1UL << 0x1F)) - 3U);
    } else {
        /* Flash size < 4G bits */
        flash->flash_size = (jedec_info.basic_flash_param_table.dword2.flash_memory_density + 1U) >> 3;
    }

    /* Calculate Page size */
    if (jedec_info.basic_flash_param_table_size < SFDP_BASIC_PROTOCOL_TABLE_SIZE_REVA) {
        flash->page_size = 256U;
    } else {
        flash->page_size = 1UL << (jedec_info.basic_flash_param_table.dword11.page_size);
        flash->page_size = (flash->page_size == (1UL << 15)) ? 256U : flash->page_size;
    }

    /* Calculate Sector Size */
    for (uint32_t i = 0; i < 4U; i++) {
        if (jedec_info.basic_flash_param_table.dword8_9[i].erase_size != 0U) {
            uint32_t current_erase_size = 1UL << jedec_info.basic_flash_param_table.dword8_9[i].erase_size;
            if (current_erase_size < 1024U) {
                continue;
            }
            if (current_erase_size < sector_size) {
                sector_size = current_erase_size;
                sector_erase_type = i;
            }
            if ((current_erase_size > block_size) && (current_erase_size < (1024U * 1024U))) {
                block_size = current_erase_size;
                block_erase_type = i;
            }
        }
    }

    if (flash->flash_size > MAX_24BIT_ADDRESSING_SIZE) {
        if (jedec_info.jedec_4byte_addressing_inst_table_enable) {
            flash->sector_erase_cmd = jedec_info.jedec_4byte_addressing_inst_table.dword2.erase_inst[sector_erase_type];
            flash->block_erase_cmd = jedec_info.jedec_4byte_addressing_inst_table.dword2.erase_inst[block_erase_type];
        } else {
            switch (jedec_info.basic_flash_param_table.dword8_9[sector_erase_type].erase_inst) {
                case NORFLASH_COMMAND_SECTOR_ERASE_4K_3B:
                    flash->sector_erase_cmd = NORFLASH_COMMAND_SECTOR_ERASE_4K_4B;
                    break;
                case NORFLASH_COMMAND_SECTOR_ERASE_64K_3B:
                    flash->sector_erase_cmd = NORFLASH_COMMAND_SECTOR_ERASE_64K_4B;
                    break;
                default:
                    /* Reserved for future use */
                    break;
            }
            switch (jedec_info.basic_flash_param_table.dword8_9[block_erase_type].erase_inst) {
                case NORFLASH_COMMAND_SECTOR_ERASE_4K_3B:
                    flash->block_erase_cmd = NORFLASH_COMMAND_SECTOR_ERASE_4K_4B;
                    break;
                case NORFLASH_COMMAND_SECTOR_ERASE_64K_3B:
                    flash->block_erase_cmd = NORFLASH_COMMAND_SECTOR_ERASE_64K_4B;
                    break;
                default:
                    /* Reserved for future use */
                    break;
            }
        }
    } else {
        flash->sector_erase_cmd = jedec_info.basic_flash_param_table.dword8_9[sector_erase_type].erase_inst;
        flash->block_erase_cmd = jedec_info.basic_flash_param_table.dword8_9[block_erase_type].erase_inst;
    }

    flash->addr_size = flash->flash_size > MAX_24BIT_ADDRESSING_SIZE ? CHRY_SFLASH_ADDRSIZE_32BITS : CHRY_SFLASH_ADDRSIZE_24BITS;
    flash->sector_size = sector_size;
    flash->block_size = block_size;

    chry_sflash_norflash_parse_page_program_para(flash, &jedec_info);
    chry_sflash_norflash_parse_read_para(flash, &jedec_info);

    if (flash->host->iomode == CHRY_SFLASH_IOMODE_QUAD) {
        ret = chry_sflash_norflash_enter_quad_mode(flash, &jedec_info);
        if (ret < 0) {
            return ret;
        }
    }

    if (flash->addr_size == CHRY_SFLASH_ADDRSIZE_32BITS) {
        ret = chry_sflash_norflash_send_command(flash, NORFLASH_COMMAND_ENTER_4B_ADDRESS_MODE);
        if (ret < 0) {
            return ret;
        }
    }

//    printf("Nor Flash sfdp version :v%d.%d\r\n", flash->sfdp_major_version, flash->sfdp_minor_version);
//    printf("Nor Flash size :%d MB\r\n", flash->flash_size / 1024 / 1024);
//    printf("Nor Flash block_size :%d KB\r\n", flash->block_size / 1024);
//    printf("Nor Flash sector_size :%d KB\r\n", flash->sector_size / 1024);
//    printf("Nor Flash page_size :%d Byte\r\n", flash->page_size);
//    printf("Nor Flash addr_size :%d Byte\r\n", flash->addr_size);
//    printf("Nor Flash block_erase_cmd :0x%02x\r\n", flash->block_erase_cmd);
//    printf("Nor Flash sector_erase_cmd :0x%02x\r\n", flash->sector_erase_cmd);
//    printf("Nor Flash page_program_cmd: 0x%02X, addr_mode: %d, data_mode: %d\r\n", flash->page_program_cmd, flash->page_program_addr_mode, flash->page_program_data_mode);
//    printf("Nor Flash read_cmd: 0x%02X, addr_mode: %d, data_mode: %d\r\n", flash->read_cmd, flash->read_addr_mode, flash->read_data_mode);
    return 0;
}

int chry_sflash_norflash_erase(struct chry_sflash_norflash *flash, uint32_t start_addr, uint32_t len)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };
    int ret;
    bool busy;
    uint32_t offset;
    uint32_t erase_size;

    if ((start_addr % flash->sector_size) || (len % flash->sector_size)) {
        return -CHRY_SFLASH_ERR_INVAL;
    }

    offset = 0;
    while (len > 0) {
        while (1) {
            ret = chry_sflash_norflash_is_busy(flash, &busy);
            if (ret < 0) {
                return ret;
            }
            if (!busy) {
                break;
            }
        }

        ret = chry_sflash_norflash_send_command(flash, NORFLASH_COMMAND_WRITE_ENABLE);
        if (ret < 0) {
            return ret;
        }

        command_seq.dma_enable = false;
        command_seq.cmd_phase.cmd = flash->sector_erase_cmd;
        command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
        command_seq.addr_phase.addr = start_addr + offset;
        command_seq.addr_phase.addr_mode = CHRY_SFLASH_ADDRMODE_1LINES;
        command_seq.addr_phase.addr_size = flash->addr_size;

        if (len > flash->block_size) {
            erase_size = flash->block_size;
            command_seq.cmd_phase.cmd = flash->block_erase_cmd;
        } else {
            erase_size = flash->sector_size;
            command_seq.cmd_phase.cmd = flash->sector_erase_cmd;
        }

        ret = chry_sflash_transfer(host, &command_seq);
        if (ret < 0) {
            return ret;
        }

        while (1) {
            ret = chry_sflash_norflash_is_busy(flash, &busy);
            if (ret < 0) {
                return ret;
            }
            if (!busy) {
                break;
            }
        }

        offset += erase_size;
        len -= erase_size;
    }

    return 0;
}

int chry_sflash_norflash_write(struct chry_sflash_norflash *flash, uint32_t start_addr, uint8_t *buf, uint32_t buflen)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };
    uint32_t data_len;
    uint8_t *data;
    int ret;
    bool busy;

    if ((start_addr + buflen) > flash->flash_size) {
        return -CHRY_SFLASH_ERR_RANGE;
    }

    command_seq.dma_enable = true;
    command_seq.cmd_phase.cmd = flash->page_program_cmd;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.addr_phase.addr_mode = flash->page_program_addr_mode;
    command_seq.addr_phase.addr_size = flash->addr_size;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_WRITE;
    command_seq.data_phase.data_mode = flash->page_program_data_mode;

    data = buf;
    while (buflen > 0) {
        data_len = flash->page_size - start_addr % flash->page_size;

        command_seq.addr_phase.addr = start_addr;
        command_seq.data_phase.buf = data;
        command_seq.data_phase.len = (buflen > data_len) ? data_len : buflen;

        while (1) {
            ret = chry_sflash_norflash_is_busy(flash, &busy);
            if (ret < 0) {
                return ret;
            }
            if (!busy) {
                break;
            }
        }

        ret = chry_sflash_norflash_send_command(flash, NORFLASH_COMMAND_WRITE_ENABLE);
        if (ret < 0) {
            return ret;
        }

        ret = chry_sflash_transfer(host, &command_seq);
        if (ret < 0) {
            return ret;
        }

        while (1) {
            ret = chry_sflash_norflash_is_busy(flash, &busy);
            if (ret < 0) {
                return ret;
            }
            if (!busy) {
                break;
            }
        }

        buflen -= command_seq.data_phase.len;
        start_addr += command_seq.data_phase.len;
        data += command_seq.data_phase.len;
    }
    return 0;
}

int chry_sflash_norflash_read(struct chry_sflash_norflash *flash, uint32_t start_addr, uint8_t *buf, uint32_t buflen)
{
    struct chry_sflash_host *host = flash->host;
    struct chry_sflash_request command_seq = { 0 };

    command_seq.dma_enable = true;
    command_seq.cmd_phase.cmd = flash->read_cmd;
    command_seq.cmd_phase.cmd_mode = CHRY_SFLASH_CMDMODE_1LINES;
    command_seq.addr_phase.addr = start_addr;
    command_seq.addr_phase.addr_mode = flash->read_addr_mode;
    command_seq.addr_phase.addr_size = flash->addr_size;
    command_seq.dummy_phase.dummy_bytes = flash->read_dummy_bytes;
    command_seq.data_phase.direction = CHRY_SFLASH_DATA_READ;
    command_seq.data_phase.data_mode = flash->read_data_mode;
    command_seq.data_phase.buf = buf;
    command_seq.data_phase.len = buflen;

    return chry_sflash_transfer(host, &command_seq);
}