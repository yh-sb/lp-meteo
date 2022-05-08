#pragma once

#include <stdint.h>
#include "periph/uart.hpp"
#include "FreeRTOS.h"
#include "semphr.h"
#include "nmea.h"
#include "gpgga.h"
#include "gpgll.h"
#include "gpgsv.h"
#include "gprmc.h"

namespace drv
{
class nmea_reader
{
public:
    enum res_t
    {
        RES_OK = 0,
        RES_READ_ERR = -1,
        RES_PARSE_ERR = -2,
        RES_UNKNOWN_MSG_ERR = -3,
    };
    
    struct queue_t
    {
        res_t res;
        nmea_t type;
        union
        {
            nmea_gpgga_s gpgga;
            nmea_gpgll_s gpgll;
            nmea_gprmc_s gprmc;
            nmea_gpgsv_s gpgsv;
        };
    };
    
    nmea_reader(periph::uart &uart, UBaseType_t task_priority);
    ~nmea_reader();
    
    QueueHandle_t get_queue() { return queue; };
    
private:
    periph::uart &_uart;
    TaskHandle_t task_hndlr;
    QueueHandle_t queue;
    
    static void task(void *pvParameters);
    void on_recv(char *buff, size_t size);
    queue_t format_queue(nmea_s *data);
    bool get_line(char *buff, size_t size, char **line, size_t *line_size);
};
}
