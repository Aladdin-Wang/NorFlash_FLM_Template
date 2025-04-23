/*
 * Copyright (c) 2024, sakumisu
 * Copyright (c) 2024, RCSN
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "chry_sflash.h"
#include "qspi.h"



void QSPI_SendCmd(uint32_t cmd,uint32_t cmdMode,uint32_t addr,uint32_t addrMode,uint32_t addrSize,uint32_t dataMode, uint32_t dummyCycles)
{      
	u8 DataMode,AddressSize,AddressMode,InstructionMode,dmcycle;
	
	if(cmdMode == CHRY_SFLASH_CMDMODE_NONE){
		InstructionMode = 0;
	}else if(cmdMode == CHRY_SFLASH_CMDMODE_1LINES){
        InstructionMode = 1;	 
	}else if(cmdMode == CHRY_SFLASH_CMDMODE_2LINES){
        InstructionMode = 2;	 
	}else if(cmdMode == CHRY_SFLASH_CMDMODE_4LINES){
        InstructionMode = 3;	  
	}       

	if(addrMode == CHRY_SFLASH_ADDRMODE_NONE){
        AddressMode     = 0;  
	}else if(addrMode == CHRY_SFLASH_ADDRMODE_1LINES){
        AddressMode     = 1;    
	}else if(addrMode == CHRY_SFLASH_ADDRMODE_2LINES){
        AddressMode     = 2;    
	}else if(addrMode == CHRY_SFLASH_ADDRMODE_4LINES){
        AddressMode     = 3;     
	}
	if(addrSize == CHRY_SFLASH_ADDRSIZE_8BITS){
        AddressSize     = 0;    
	}else if(addrSize == CHRY_SFLASH_ADDRSIZE_16BITS){
        AddressSize     = 1;    
	}else if(addrSize == CHRY_SFLASH_ADDRSIZE_24BITS){
        AddressSize     = 2;    
	}else if(addrSize == CHRY_SFLASH_ADDRSIZE_32BITS){
        AddressSize     = 3;   
	}
	
	if(dataMode == CHRY_SFLASH_DATAMODE_NONE){
        DataMode        = 0;    
	}else if(dataMode == CHRY_SFLASH_DATAMODE_1LINES){
        DataMode        = 1;    
	}else if(dataMode == CHRY_SFLASH_DATAMODE_2LINES){
        DataMode        = 2;   
	}else if(dataMode == CHRY_SFLASH_DATAMODE_4LINES){
        DataMode        = 3;   
	}
	
	dmcycle = dummyCycles * 8 / dataMode;
	u8 mode = (DataMode << 6) | ((AddressSize) << 4) | (AddressMode << 2) | (InstructionMode << 0);
    QSPI_Send_CMD(cmd,addr, mode,dmcycle);
 
}

int chry_sflash_init(struct chry_sflash_host *host)
{
    QSPI_SendCmd(0x66, CHRY_SFLASH_CMDMODE_4LINES, 0, 0, 0, 0, 0);  // Enable Reset
    QSPI_SendCmd(0x99, CHRY_SFLASH_CMDMODE_4LINES, 0, 0, 0, 0, 0);  // Execute Reset
    return 0;
}

int chry_sflash_set_frequency(struct chry_sflash_host *host, uint32_t freq)
{

    return 0;
}

int chry_sflash_transfer(struct chry_sflash_host *host, struct chry_sflash_request *req)
{
    int stat = 0;
    if (req->data_phase.direction == CHRY_SFLASH_DATA_READ) {
		QSPI_SendCmd(req->cmd_phase.cmd,req->cmd_phase.cmd_mode,req->addr_phase.addr,req->addr_phase.addr_mode,req->addr_phase.addr_size,req->data_phase.data_mode,req->dummy_phase.dummy_bytes);
		if(req->data_phase.buf != NULL && req->data_phase.len != 0){
			QSPI_Receive(req->data_phase.buf,req->data_phase.len);
		}
	} else {
		QSPI_SendCmd(req->cmd_phase.cmd,req->cmd_phase.cmd_mode,req->addr_phase.addr,req->addr_phase.addr_mode,req->addr_phase.addr_size,req->data_phase.data_mode,req->dummy_phase.dummy_bytes);
		if(req->data_phase.buf != NULL && req->data_phase.len != 0){
		    QSPI_Transmit(req->data_phase.buf,req->data_phase.len);
		}
    }
    return stat;
}

int chry_sflash_deinit(struct chry_sflash_host *host)
{
    return 0;
}






















