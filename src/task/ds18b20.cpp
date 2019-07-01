#include "ds18b20.hpp"
#include "ui.hpp"

void task::ds18b20(void *pvParameters)
{
	task::ds18b20_ctx_t *ctx = (task::ds18b20_ctx_t *)pvParameters;
	drv::ds18b20 *_ds18b20 = ctx->_ds18b20;
	ui_queue_t queue;
	
	_ds18b20->set_resol(0, drv::ds18b20::RESOL_12_BIT);
	
	while(1)
	{
		queue.cmd = CMD_DS18B20;
		queue.ds18b20.err = _ds18b20->get_temp(0, &queue.ds18b20.t);
		
		xQueueSend(ctx->to_ui, &queue, portMAX_DELAY);
	}
}
