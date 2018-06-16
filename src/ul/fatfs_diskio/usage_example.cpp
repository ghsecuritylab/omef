
#include <string.h>


#include "hal/gpio.hpp"
#include "drv/sd/sd_spi.hpp"
#include "drv/di/di.hpp"
#include "third_party/FreeRTOS/include/FreeRTOS.h"
#include "third_party/FreeRTOS/include/task.h"
#include "third_party/FatFs/ff.h"
#include "ul/fatfs_diskio/fatfs_diskio.hpp"

using namespace hal;
using namespace drv;
using namespace ul;

static void cd_cb(di *di, bool state, void *ctx);

static void di_task(void *pvParameters)
{
	di *cd_di = (di *)pvParameters;
	while(1)
	{
		cd_di->poll();
		vTaskDelay(1);
	}
}

int main(void)
{
	static gpio b1(0, 0, GPIO_MODE_DI, 0);
	
	static gpio spi1_mosi(0, 7, GPIO_MODE_AF, 0);
	static gpio spi1_miso(0, 6, GPIO_MODE_AF, 0);
	static gpio spi1_clk(0, 5, GPIO_MODE_AF, 0);
	static gpio sd_cs(0, 4, GPIO_MODE_DO, 1);
	static gpio sd_cd(0, 3, GPIO_MODE_DI, 1);
	
	static dma spi1_rx_dma(DMA_2, DMA_STREAM_0, DMA_CH_3,
		DMA_DIR_PERIPH_TO_MEM, DMA_INC_SIZE_8);
	static dma spi1_tx_dma(DMA_2, DMA_STREAM_3, DMA_CH_3,
		DMA_DIR_MEM_TO_PERIPH, DMA_INC_SIZE_8);
	
	static spi spi1(SPI_1, 1000000, SPI_CPOL_0, SPI_CPHA_0, SPI_BIT_ORDER_MSB,
		spi1_tx_dma, spi1_rx_dma, spi1_mosi, spi1_miso, spi1_clk);
	
	static sd_spi sd1(spi1, sd_cs, &sd_cd);
	
	static di cd_di(sd_cd, 100, 1);
	cd_di.cb(cd_cb, &sd1);
	
	fatfs_diskio_add(0, sd1);
	
	xTaskCreate(di_task, "di", configMINIMAL_STACK_SIZE * 60, &cd_di,
		tskIDLE_PRIORITY + 2, NULL);
	
	vTaskStartScheduler();
}

static void cd_cb(di *di, bool state, void *ctx)
{
	if(state)
		return;
	
	sd *sd1 = (sd *)ctx;
	
	FATFS fs;
	FRESULT ff_res = f_mount(&fs, "SD", 1);
	if(ff_res)
	{
		ff_res = f_mkfs("SD", FM_FAT32, 0, NULL, 0);
		if(ff_res)
			return;
		ff_res = f_mount(&fs, "SD", 1);
		if(ff_res)
			return;
		ff_res = f_unmount("SD");
		if(ff_res)
			return;
	}
	
	FIL file;
	ff_res = f_open(&file, "SD:test.txt", FA_WRITE | FA_CREATE_ALWAYS);
	if(ff_res)
		return;
	UINT size = strlen("abcdefgh-1234567890");
	ff_res = f_write(&file, "abcdefgh-1234567890", size, &size);
	if(ff_res)
		return;
	ff_res = f_close(&file);
	if(ff_res)
		return;
	ff_res = f_open(&file, "SD:test.txt", FA_READ);
	if(ff_res)
		return;
	
	size = 0;
	uint8_t buff[64];
	memset(buff, 0, sizeof(buff));
	ff_res = f_read(&file, buff, sizeof(buff), &size);
	
	f_close(&file);
}
