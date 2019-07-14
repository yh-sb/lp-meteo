#include <string.h>
#include "ui.hpp"
#include "rtc/rtc.hpp"
#include "ul/syslog/syslog.hpp"

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
				if(queue.dht11.res == drv::dht::RES_OK)
				{
					uint8_t ddram = ctx->lcd->print(0, "%02d,%d %%",
						queue.dht11.val.rh_x10 / 10,
						queue.dht11.val.rh_x10 % 10);
					
					char sign = (queue.dht11.val.t_x10 >= 0) ? '+' : '-';
					ctx->lcd->print(ddram, "  %c%02d,%d C", sign,
						queue.dht11.val.t_x10 / 10, queue.dht11.val.t_x10 % 10);
				}
				else
				{
					log().err("DHT11: res=%d", queue.dht11.res);
					ctx->lcd->print(0, "DHT11 Error");
				}
				break;
			
			case CMD_DS18B20:
				if(queue.ds18b20.err == drv::ds18b20::RES_OK)
				{
					char sign = ' ';
					if(queue.ds18b20.t >= 1)
						sign = '+';
					else if(queue.ds18b20.t <= -1)
						sign = '-';
					
					ctx->lcd->print(64, "        %c%.2f C", sign,
						queue.ds18b20.t);
				}
				else
				{
					log().err("DS18B20: res=%d", queue.ds18b20.err);
					ctx->lcd->print(64, "DS18B20 Error");
				}
				break;
			
			default: ASSERT(0);
		}
	}
}
