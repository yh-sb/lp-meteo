#include <string.h>
#include <math.h>
#include <assert.h>
#include "tasks/ui.hpp"
#include "periph/rtc.hpp"
#include "syslog.hpp"

constexpr size_t matrix_x_size = 40;
constexpr size_t text_speed = 35; // ms per dot

static void print_temp(drv::dot_matrix &matrix, float t);
static void print_rh(drv::dot_matrix &matrix, uint16_t rh);
static void print_time(drv::dot_matrix &matrix, struct tm &tm);
static void print_date(drv::dot_matrix &matrix, struct tm &tm);

void tasks::ui(void *pvParameters)
{
    tasks::ui_ctx_t *ctx = (tasks::ui_ctx_t *)pvParameters;
    ui_queue_t queue;
    
    struct
    {
        struct
        {
            float val;
            bool is_valid;
        } t;
        struct
        {
            uint16_t val;
            bool is_valid;
        } rh;
    } measurements = {};
    
    while(1)
    {
        while(xQueueReceive(ctx->to_ui, &queue, 0) == pdTRUE)
        {
            switch(queue.cmd)
            {
                case CMD_DHT11:
                    measurements.rh.val = queue.dht.rh_x10;
                    measurements.rh.is_valid = true;
                    break;
                
                case CMD_DS18B20:
                    measurements.t.val = queue.t;
                    measurements.t.is_valid = true;
                    break;
                
                default: assert(0);
            }
        }
        
        if(measurements.t.is_valid)
        {
            measurements.t.is_valid = false;
            print_temp(*ctx->matrix, measurements.t.val);
            vTaskDelay(4000);
        }
        
        if(measurements.rh.is_valid)
        {
            measurements.rh.is_valid = false;
            print_rh(*ctx->matrix, measurements.rh.val);
            vTaskDelay(4000);
        }
        
        struct tm tm = periph::rtc::get();
        print_date(*ctx->matrix, tm);
        vTaskDelay(4000);
        
        tm = periph::rtc::get();
        print_time(*ctx->matrix, tm);
        vTaskDelay(4000);
    }
}

static void print_temp(drv::dot_matrix &matrix, float t)
{
    const char *format = "%d,%01d""\xF8""C";
    
    int integer = (int)t;
    int decimal = (t - integer) * 10;
    
    char sign = ' ';
    if(integer > 0 || decimal > 0)
        sign = '+';
    else if(integer < 0 || decimal < 0)
    {
        sign = '-';
        integer = abs(integer);
        decimal = abs(decimal);
    }
    
    size_t x = matrix.get_size(format, integer, decimal).x_size;
    if(sign != ' ')
        x += matrix.get_size("%c", sign).x_size;
    
    matrix.shift_x(text_speed, (matrix_x_size - x) / 2);
    if(sign != ' ')
        matrix.print(text_speed, "%c", sign);
    matrix.print(text_speed, format, integer, decimal);
    matrix.shift_x(text_speed, ((matrix_x_size - x) / 2));
}

static void print_rh(drv::dot_matrix &matrix, uint16_t rh)
{
    const char *format = "%d%%";
    
    size_t x = matrix.get_size("RH").x_size + 3 + matrix.get_size(format,
        rh / 10, rh % 10).x_size;
    
    matrix.shift_x(text_speed, (matrix_x_size - x) / 2);
    matrix.print(text_speed, "RH");
    matrix.shift_x(text_speed, 3);
    matrix.print(text_speed, format, rh / 10, rh % 10);
    matrix.shift_x(text_speed, (matrix_x_size - x) / 2);
}

static void print_time(drv::dot_matrix &matrix, struct tm &tm)
{
    const char *format = "%02d:%02d";
    
    size_t x = matrix.get_size(format, tm.tm_hour, tm.tm_min).x_size;
    matrix.shift_x(text_speed, (matrix_x_size - x) / 2);
    matrix.shift_x(text_speed, 2);
    matrix.print(text_speed, format, tm.tm_hour, tm.tm_min);
    matrix.shift_x(text_speed, (matrix_x_size - x) / 2);
}

static void print_date(drv::dot_matrix &matrix, struct tm &tm)
{
    const char *format = "%02d.%02d.%02d";
    
    size_t x = matrix.get_size(format, tm.tm_mday, tm.tm_mon + 1,
        tm.tm_year % 100).x_size;
    matrix.shift_x(text_speed, (matrix_x_size - x) / 2);
    matrix.print(text_speed, format, tm.tm_mday, tm.tm_mon + 1, tm.tm_year % 100);
    matrix.shift_x(text_speed, (matrix_x_size - x) / 2);
}
