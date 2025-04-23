#define PORT_SPI_BASE         HPM_SPI7
#define PORT_SPI_NOR_DMA      HPM_HDMA
#define PORT_SPI_NOR_DMAMUX   HPM_DMAMUX
#define PORT_SPI_RX_DMA_REQ   HPM_DMA_SRC_SPI7_RX
#define PORT_SPI_TX_DMA_REQ   HPM_DMA_SRC_SPI7_TX
#define PORT_SPI_RX_DMA_CH    0
#define PORT_SPI_TX_DMA_CH    1

static void spi_init_pins(hpm_spi_config_t *config)
{
    if (config->host_base == HPM_SPI7) {
        HPM_IOC->PAD[IOC_PAD_PF27].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
        HPM_IOC->PAD[IOC_PAD_PF26].FUNC_CTL = IOC_PF26_FUNC_CTL_SPI7_SCLK | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1);
        HPM_IOC->PAD[IOC_PAD_PF28].FUNC_CTL = IOC_PF28_FUNC_CTL_SPI7_MISO;
        HPM_IOC->PAD[IOC_PAD_PF29].FUNC_CTL = IOC_PF29_FUNC_CTL_SPI7_MOSI;
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
    config->cs_pin = IOC_PAD_PF27;

    board_init_spi_clock(config->host_base);
    spi_init_pins(config);

    return status_success;
}
