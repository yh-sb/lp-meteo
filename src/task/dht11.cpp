#include "dht11.hpp"
#include "ui.hpp"

void task::dht11(void *pvParameters)
{
	task::dht11_ctx_t *ctx = (task::dht11_ctx_t *)pvParameters;
	drv::dht *dht11 = ctx->dht11;
	ui_queue_t queue;
	
	while(1)
	{
		queue.cmd = CMD_DHT11;
		drv::dht::val_t val = {};
		queue.dht11.res = dht11->get(&val);
		queue.dht11.val = val;
		
		xQueueSend(ctx->to_ui, &queue, portMAX_DELAY);
	}
}
