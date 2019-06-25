#include <string.h>

#include "rtc/rtc.hpp"

#include "ui.hpp"
#include "drv/dht11/dht11.hpp"
#include "drv/ds18b20/ds18b20.hpp"
#include "ul/syslog/syslog.hpp"
#include "third_party/printf/printf.h"

#include "FreeRTOS.h"
#include "task.h"

void task::ui(void *pvParameters)
{
	task::ui_ctx_t *ctx = (task::ui_ctx_t *)pvParameters;
	ui_queue_t queue;
	
	ctx->lcd->init();
	ctx->lcd->clear();
	
	while(1)
	{
		xQueueReceive(ctx->to_ui, &queue, portMAX_DELAY);
		
		switch(queue.cmd)
		{
			case CMD_DHT11:
				ctx->lcd->ddram_addr(0);
				if(queue.dht11.res == drv::dht11::RES_OK)
					ctx->lcd->print("%02d %%", queue.dht11.rh);
				else
				{
					log().err("DHT11: res=%d", queue.dht11.res);
					ctx->lcd->print("DHT11 Error");
				}
				break;
			
			case CMD_DS18B20:
				ctx->lcd->ddram_addr(64);
				if(queue.ds18b20.err == drv::ds18b20::RES_OK)
				{
					char sign = ' ';
					if(queue.ds18b20.t >= 1)
						sign = '+';
					else if(queue.ds18b20.t <= -1)
						sign = '-';
					
					ctx->lcd->print("%c%.2f C", sign, queue.ds18b20.t);
				}
				else
				{
					log().err("DS18B20: res=%d", queue.ds18b20.err);
					ctx->lcd->print("DS18B20 Error");
				}
				break;
			
			default: ASSERT(0);
		}
		
		hal::rtc::time_t time;
		hal::rtc::get(&time);
		
		ctx->lcd->ddram_addr(20);
		ctx->lcd->print("%02d:%02d %02d.%02d.%02d", time.h, time.m,
			time.day, time.month, time.year);
	}
}
