#include "dht11.hpp"
#include "ui.hpp"

void task::dht11(void *pvParameters)
{
	task::dht11_ctx_t *ctx = (task::dht11_ctx_t *)pvParameters;
	drv::dht *dht11 = ctx->dht11;
	ui_queue_t queue;
	
	while(1)
	{
		int res = dht11->get(&queue.dht);
		if(res)
		{
			//log().err("DHT11: res=%d", res);
			continue;
		}
		
		queue.cmd = CMD_DHT11;
		xQueueSend(ctx->to_ui, &queue, portMAX_DELAY);
	}
}
