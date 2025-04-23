/*
 * Copyright (c) 2024, sakumisu
 * Copyright (c) 2024, RCSN
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHRY_SFLASH_SFDP_DEF_H
#define CHRY_SFLASH_SFDP_DEF_H

#include <stdint.h>

#define SFDP_READ_FREQUENCY                                                (10000000U)

#define SFDP_SIGNATURE                                                     (0x50444653UL) /* ASCII: SFDP */

#define SFDP_VERSION_MAJOR_1_0                                             (1U)
#define SFDP_VERSION_MINOR_0                                               (0U) /* JESD216 */
#define SFDP_VERSION_MINOR_A                                               (5U) /* JESD216A */
#define SFDP_VERSION_MINOR_B                                               (6U) /* JESD216B */
#define SFDP_VERSION_MINOR_C                                               (7U) /* JESD216C */
#define SFDP_VERSION_MINOR_D                                               (8U) /* JESD216D */

#define SFDP_BASIC_PROTOCOL_TABLE_SIZE_REV0                                (36U)
#define SFDP_BASIC_PROTOCOL_TABLE_SIZE_REVA                                (64U)
#define SFDP_BASIC_PROTOCOL_TABLE_SIZE_REVB                                SFDP_BASIC_PROTOCOL_TABLE_SIZE_REVA
#define SFDP_BASIC_PROTOCOL_TABLE_SIZE_REVC                                (80U)
#define SFDP_BASIC_PROTOCOL_TABLE_SIZE_REVD                                SFDP_BASIC_PROTOCOL_TABLE_SIZE_REVC

#define SFDP_PARAMETER_ID_BASIC_SPI_PROTOCOL                               (0xFF00U)
#define SFDP_PARAMETER_ID_SECTOR_MAP                                       (0xFF81U)
#define SFDP_PARAMETER_ID_RPMC                                             (0xFF03U)
#define SFDP_PARAMETER_ID_4BYTEADDRESS_INSTRUCTION_TABLE                   (0xFF84U)
#define SFDP_PARAMETER_ID_xSPI_PROFILE_V1                                  (0xFF05U)
#define SFDP_PARAMETER_ID_xSPI_PROFILE_V2                                  (0xFF06U)
#define SFDP_PARAMETER_ID_STATUIS_CTRL_CONFIG_REGISTER_MAP                 (0xFF87U)
#define SFDP_PARAMETER_ID_STATUIS_CTRL_CONFIG_REGISTER_MAP_MULTI           (0xFF88U)
#define SFDP_PARAMETER_ID_STATUIS_CTRL_CONFIG_REGISTER_MAP_xSPI_PROFILE_V2 (0xFF09U)
#define SFDP_PARAMETER_ID_COMMAND_SEQUENCE_CHANGE_QCTAL_DDR_8D8D8D_MODE    (0xFF0AU)
#define SFDP_PARAMETER_ID_MSPT                                             (0xFF8BU)
#define SFDP_PARAMETER_ID_X4_QUAD_DS                                       (0xFF0CU)
#define SFDP_PARAMETER_ID_COMMAND_SEQUENCE_CHANGE_QUAD_DDR_4S4D4D_MODE     (0xFF8DU)
#define SFDP_PARAMETER_ID_SECURITY_READ_WRITE                              (0xFF8EU)

/* !@brief SFDP Parameter Header, see JESD216D doc for more details */
typedef union _sfdp_header {
    uint32_t dwords[2];

    struct {
        uint32_t signature;
        uint8_t minor_rev;
        uint8_t major_rev;
        uint8_t nph;
        uint8_t access_protocol;
    };
} sfdp_header_t;

typedef union _sfdp_parameter_header {
    uint32_t dwords[2];

    struct {
        uint8_t parameter_id_lsb;
        uint8_t minor_rev;
        uint8_t major_rev;
        uint8_t parameter_table_length;
        uint8_t parameter_table_pointer[3];
        uint8_t parameter_id_msb;
    };
} sfdp_parameter_header_t;

/* !@brief Basic Flash Parameter Table, see JESD216D doc for more details */
typedef union _jedec_flash_param_table {
    uint32_t dwords[23];

    struct {
        struct {
            uint32_t erase_size               : 2;
            uint32_t write_granularity        : 1;
            uint32_t reserved0                : 2;
            uint32_t unused0                  : 3;
            uint32_t erase4k_inst             : 8;
            uint32_t support_1_1_2_fast_read  : 1;
            uint32_t address_bytes            : 2;
            uint32_t support_ddr_clocking     : 1;
            uint32_t support_1_2_2_fast_read  : 1;
            uint32_t support_1_4_4_fast_read : 1;
            uint32_t support_1_1_4_fast_read  : 1;
            uint32_t unused1                  : 9;
        } dword1;
        struct
        {
            uint32_t flash_memory_density;
        } dword2;
        struct {
            uint32_t dummy_clocks_1_4_4_fast_read : 5;
            uint32_t mode_clocks_1_4_4_fast_read  : 3;
            uint32_t inst_1_4_4_fast_read         : 8;
            uint32_t dummy_clocks_1_1_4_fast_read : 5;
            uint32_t mode_clocks_1_1_4_fast_read  : 3;
            uint32_t inst_1_1_4_fast_read         : 8;
        } dword3;
        struct {
            uint32_t dummy_clocks_1_1_2_fast_read : 5;
            uint32_t mode_clocks_1_1_2_fast_read  : 3;
            uint32_t inst_1_1_2_fast_read         : 8;
            uint32_t dummy_clocks_1_2_2_fast_read : 5;
            uint32_t mode_clocks_1_2_2_fast_read  : 3;
            uint32_t inst_1_2_2_fast_read         : 8;
        } dword4;
        struct {
            uint32_t support_2_2_2_fast_read : 1;
            uint32_t reserved0               : 3;
            uint32_t support_4_4_4_fast_read : 1;
            uint32_t reserved1               : 27;
        } dword5;
        struct {
            uint32_t reserved0                    : 16;
            uint32_t dummy_clocks_2_2_2_fast_read : 5;
            uint32_t mode_clocks_2_2_2_fast_read  : 3;
            uint32_t inst_2_2_2_fast_read         : 8;
        } dword6;
        struct {
            uint32_t reserved0                    : 16;
            uint32_t dummy_clocks_4_4_4_fast_read : 5;
            uint32_t mode_clocks_4_4_4_fast_read  : 3;
            uint32_t inst_4_4_4_fast_read         : 8;
        } dword7;
        struct {
            uint8_t erase_size;
            uint8_t erase_inst;
        } dword8_9[4];
        struct {
            uint32_t erase_time_multiplier : 4;
            uint32_t erase1_time           : 7;
            uint32_t erase2_time           : 7;
            uint32_t erase3_time           : 7;
            uint32_t erase4_time           : 7;
        } dword10;
        struct {
            uint32_t apge_program_time_multiplier : 4;
            uint32_t page_size                    : 4;
            uint32_t page_program_time            : 6;
            uint32_t byte_program_time_first      : 5;
            uint32_t byte_program_time_additional : 5;
            uint32_t chip_erase_time              : 7;
            uint32_t reserved0                    : 1;
        } dword11;
        struct {
            uint32_t reserved0;
        } dword12;
        struct {
            uint32_t inst_program_resume  : 8;
            uint32_t inst_program_suspend : 8;
            uint32_t inst_resume          : 8;
            uint32_t inst_suspend         : 8;
        } dword13;
        struct {
            uint32_t reserved0                      : 2;
            uint32_t status_reg_polling_device_busy : 6;
            uint32_t reserved1                      : 24;
        } dword14;
        struct {
            uint32_t mode_4_4_4_disable_seq     : 4;
            uint32_t mode_4_4_4_enable_seq      : 5;
            uint32_t support_mode_0_4_4         : 1;
            uint32_t exit_method_in_mode_0_4_4  : 6;
            uint32_t entry_method_in_mode_0_4_4 : 4;
            uint32_t quad_enable_requirement    : 3;
            uint32_t hold_reset_disable         : 1;
            uint32_t reserved0                  : 8;
        } dword15;
        struct {
            uint32_t status_reg_write_enable   : 7;
            uint32_t reserved0                 : 1;
            uint32_t soft_reset_rescue_support : 6;
            uint32_t exit_4_byte_addressing    : 10;
            uint32_t enter_4_byte_addrssing    : 8;
        } dword16;
        struct {
            uint32_t dummy_clocks_1_8_8_fast_read : 5;
            uint32_t mode_clocks_1_8_8_fast_read  : 3;
            uint32_t inst_1_8_8_fast_read         : 8;
            uint32_t dummy_clocks_1_1_8_fast_read : 5;
            uint32_t mode_clocks_1_1_8_fast_read  : 3;
            uint32_t inst_1_1_8_fast_read         : 8;
        } dword17;
        struct {
            uint32_t reserved0                          : 18;
            uint32_t output_driver_strength             : 5;
            uint32_t jedec_spi_protocol_reset           : 1;
            uint32_t ds_waveform_in_str                 : 2;
            uint32_t ds_support_in_4_4_4_mode           : 1;
            uint32_t ds_support_in_4s_4d_4d_mode        : 1;
            uint32_t reserved1                          : 1;
            uint32_t cmd_and_extension_in_8d_8d_8d_mode : 2;
            uint32_t byte_order_in_8d_8d_8d_mode        : 1;
        } dword18;
        struct {
            uint32_t mode_8_8s_8_disable_seq    : 4;
            uint32_t mode_8_8_8_enable_seq      : 5;
            uint32_t support_mode_0_8_8         : 1;
            uint32_t exit_method_in_mode_0_8_8  : 6;
            uint32_t entry_method_in_mode_0_8_8 : 4;
            uint32_t octal_enable_requirement   : 3;
            uint32_t reserved0                  : 9;
        } dword19;
        struct {
            uint32_t max_speed_in_4_4_4_mode_without_ds    : 4;
            uint32_t max_speed_in_4_4_4_mode_with_ds       : 4;
            uint32_t max_speed_in_4s_4d_4d_mode_without_ds : 4;
            uint32_t max_speed_in_4s_4d_4d_mode_with_ds    : 4;
            uint32_t max_speed_in_8_8_8_mode_without_ds    : 4;
            uint32_t max_speed_in_8_8_8_mode_with_ds       : 4;
            uint32_t max_speed_in_8d_8d_8d_mode_without_ds : 4;
            uint32_t max_speed_in_8d_8d_8d_mode_with_ds    : 4;
        } dword20;
        struct {
            uint32_t support_1s_1d_1d_fast_read  : 1;
            uint32_t supports_1s_2d_2d_fast_read : 1;
            uint32_t support_1s_4d_4d_fast_read  : 1;
            uint32_t support_4s_4d_4d_fast_read  : 1;
            uint32_t reserved0                   : 28;
        } dword21;
        struct {
            uint32_t dummy_clocks_1s_1d_1d_fast_read : 5;
            uint32_t mode_clocks_1s_1d_1d_fast_read  : 3;
            uint32_t inst_1s_1d_1d_fast_read         : 8;
            uint32_t dummy_clocks_1s_2d_2d_fast_read : 5;
            uint32_t mode_clocks_1s_2d_2d_fast_read  : 3;
            uint32_t inst_1s_2d_2d_fast_read         : 8;
        } dword22;
        struct {
            uint32_t dummy_clocks_1s_4d_4d_fast_read : 5;
            uint32_t mode_clocks_1s_4d_4d_fast_read  : 3;
            uint32_t inst_1s_4d_4d_fast_read         : 8;
            uint32_t dummy_clocks_4s_4d_4d_fast_read : 5;
            uint32_t mode_clocks_4s_4d_4d_fast_read  : 3;
            uint32_t inst_4s_4d_4d_fast_read         : 8;
        } dword23;
    };

} jedec_basic_flash_param_table_t;

/* !@brief 4Byte Addressing Instruction Table, see JESD216D doc for more details */
typedef union _jedec_4byte_addressing_inst_table {
    uint32_t dwords[2];
    struct {
        struct {
            uint32_t support_1_1_1_read                        : 1;
            uint32_t support_1_1_1_fast_read                   : 1;
            uint32_t support_1_1_2_fast_read                   : 1;
            uint32_t support_1_2_2_fast_read                   : 1;
            uint32_t support_1_1_4_fast_read                   : 1;
            uint32_t support_1_4_4_fast_read                   : 1;
            uint32_t support_1_1_1_page_program                : 1;
            uint32_t support_1_1_4_page_program                : 1;
            uint32_t support_1_4_4_page_program                : 1;
            uint32_t support_erase_type1_size                  : 1;
            uint32_t support_erase_type2_size                  : 1;
            uint32_t support_erase_type3_size                  : 1;
            uint32_t support_erase_type4_size                  : 1;
            uint32_t support_1s_1d_1d_dtr_read                 : 1;
            uint32_t support_1s_2d_2d_dtr_read                 : 1;
            uint32_t support_1s_4d_4d_dtr_read                 : 1;
            uint32_t support_volatile_sector_lock_read_cmd     : 1;
            uint32_t support_volatile_sector_lock_write_cmd    : 1;
            uint32_t support_nonvolatile_sector_lock_read_cmd  : 1;
            uint32_t support_nonvolatile_sector_lock_write_cmd : 1;
            uint32_t reserved                                  : 12;
        } dword1;

        struct {
            uint8_t erase_inst[4];
        } dword2;
    };
} jedec_4byte_addressing_inst_table_t;

#endif