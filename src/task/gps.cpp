#include "gps.hpp"
#include "rtc/rtc.hpp"

void task::gps(void *pvParameters)
{
	task::gps_ctx_t *ctx = (task::gps_ctx_t *)pvParameters;
	
	QueueHandle_t from_gps_drv = ctx->nmea->get_queue();
	drv::nmea::queue_t data;
	
	while(1)
	{
		static uint32_t time_sync_cnt = 0;
		if(xQueueReceive(from_gps_drv, &data, 1000) == pdTRUE)
		{
			if(data.res == drv::nmea::RES_OK && data.type == NMEA_GPRMC)
			{
				hal::rtc::set(data.gprmc.time);
				time_sync_cnt++;
			}
		}
		
		struct tm time = hal::rtc::get();
		ctx->lcd->print(20, "%02d:%02d:%02d %02d.%02d.%02d", time.tm_hour,
			time.tm_min, time.tm_sec, time.tm_mday, time.tm_mon + 1,
			time.tm_year % 100);
		
		ctx->lcd->print(84, "sync=%u", time_sync_cnt);
	}
}
