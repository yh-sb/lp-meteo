#include "dht11.hpp"
#include "ui.hpp"

#include "drv/dht11/dht11.hpp"

#include "ul/syslog/syslog.hpp"

#include "FreeRTOS.h"
#include "task.h"

void task::dht11(void *pvParameters)
{
	task::dht11_ctx_t *ctx = (task::dht11_ctx_t *)pvParameters;
	drv::dht11 *_dht11 = ctx->_dht11;
	ui_queue_t queue;
	
	while(1)
	{
		queue.cmd = CMD_DHT11;
		drv::dht11::val_t val = {};
		queue.dht11.res = _dht11->get(&val);
		queue.dht11.rh = val.rh;
		queue.dht11.t = val.t;
		
		xQueueSend(ctx->to_ui, &queue, portMAX_DELAY);
		
		vTaskDelay(1000);
	}
}
