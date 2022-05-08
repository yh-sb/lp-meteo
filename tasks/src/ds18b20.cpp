#include "tasks/ds18b20.hpp"
#include "tasks/ui.hpp"

void tasks::ds18b20(void *pvParameters)
{
    tasks::ds18b20_ctx_t *ctx = (tasks::ds18b20_ctx_t *)pvParameters;
    drv::ds18b20 *_ds18b20 = ctx->_ds18b20;
    ui_queue_t queue;
    
    _ds18b20->set_resol(0, drv::ds18b20::RESOL_12_BIT);
    
    while(1)
    {
        int res = _ds18b20->get_temp(0, &queue.t);
        
        /* TODO: fix wrong value of temperature sensor when its power is lost
           (value is always 85) */
        if(res || queue.t > 70 || queue.t < -50)
        {
            //log().err("ds18b20: res=%d", res);
            continue;
        }
        
        queue.cmd = CMD_DS18B20;
        xQueueSend(ctx->to_ui, &queue, portMAX_DELAY);
    }
}
