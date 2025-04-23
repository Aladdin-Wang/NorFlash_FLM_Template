#define PORT_SPI_BASE         HPM_SPI2
#define PORT_SPI_NOR_DMA      HPM_HDMA
#define PORT_SPI_NOR_DMAMUX   HPM_DMAMUX
#define PORT_SPI_RX_DMA_REQ   HPM_DMA_SRC_SPI2_RX
#define PORT_SPI_TX_DMA_REQ   HPM_DMA_SRC_SPI2_TX
#define PORT_SPI_RX_DMA_CH    0
#define PORT_SPI_TX_DMA_CH    1

static void spi_init_pins(hpm_spi_config_t *config)
{
    if (config->host_base == HPM_SPI2) {
        HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
        HPM_IOC->PAD[IOC_PAD_PB11].FUNC_CTL = IOC_PB11_FUNC_CTL_SPI2_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
        HPM_IOC->PAD[IOC_PAD_PB12].FUNC_CTL = IOC_PB12_FUNC_CTL_SPI2_MISO;
        HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL = IOC_PB13_FUNC_CTL_SPI2_MOSI;
        HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PB14_FUNC_CTL_SPI2_DAT2;
        HPM_IOC->PAD[IOC_PAD_PB15].FUNC_CTL = IOC_PB15_FUNC_CTL_SPI2_DAT3;
    }
}

hpm_stat_t spi_host_init(hpm_spi_config_t *config)
{
    config->host_base = PORT_SPI_BASE;
    config->dma_control.dma_base = PORT_SPI_NOR_DMA;
    config->dma_control.dmamux_base = PORT_SPI_NOR_DMAMUX;
    config->dma_control.rx_dma_ch = PORT_SPI_RX_DMA_CH;
    config->dma_control.tx_dma_ch = PORT_SPI_TX_DMA_CH;
    config->dma_control.rx_dma_req = PORT_SPI_RX_DMA_REQ;
    config->dma_control.tx_dma_req = PORT_SPI_TX_DMA_REQ;
    config->transfer_max_size = SPI_SOC_TRANSFER_COUNT_MAX;
    config->cs_pin = IOC_PAD_PB08;

    board_init_spi_clock(config->host_base);
    spi_init_pins(config);

    return status_success;
}
