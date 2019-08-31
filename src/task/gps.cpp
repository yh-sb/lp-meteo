#include "gps.hpp"
#include "rtc/rtc.hpp"

constexpr int eet_timezone = +2;
constexpr int sunday = 0;
constexpr int march = 2;
constexpr int october = 9;

static void make_zoned(struct tm &tm, int tz);
static void handle_dst(struct tm &tm);

void task::gps(void *pvParameters)
{
	task::gps_ctx_t *ctx = (task::gps_ctx_t *)pvParameters;
	
	QueueHandle_t from_gps_drv = ctx->nmea->get_queue();
	drv::nmea::queue_t data;
	uint32_t time_sync_cnt = 0;
	
	while(1)
	{
		if(xQueueReceive(from_gps_drv, &data, 1000) != pdTRUE)
			continue;
		
		if(data.res != drv::nmea::RES_OK || data.type != NMEA_GPRMC)
			continue;
		
		make_zoned(data.gprmc.time, eet_timezone);
		handle_dst(data.gprmc.time);
		if(hal::rtc::is_valid(data.gprmc.time))
		{
			hal::rtc::set(data.gprmc.time);
			time_sync_cnt++;
		}
	}
}

static void make_zoned(struct tm &tm, int tz)
{
	tm.tm_hour += tz;
	if(mktime(&tm) == -1)
	{
		//log().err("Can't adjust UTC time with timezone=%d", tz);
	}
}

// https://cboard.cprogramming.com/c-programming/90130-determining-date-[nth]-[day]-[month].html
struct tm *nth_weekday_of_month(int week, int wday, int month, int year)
{
   static const struct tm zero = {0};
   static struct tm worker = {0};
   struct tm *result = &worker;
   time_t t;
   *result = zero;
   /*
    * Build the first day of the month.
    */
   worker.tm_year = year;
   worker.tm_mon  = month;
   worker.tm_mday = 1;
   /*
    * Create the calendar time. Then normalize it back to broken-down form.
    */
   t = mktime(result);
   if ( t == (time_t)-1 )
   {
      return 0;
   }
   /*
    * Go to Nth week, where N is an offset from the first week.
    *   N =  1 is the 2nd week
    *   N =  0 is the 1st week
    *   N = -1 is the last week of the previous month
    */
   if ( wday != result->tm_wday )
   {
      result->tm_mday += wday - result->tm_wday + 7 * (wday < result->tm_wday);
   }
   result->tm_mday += 7 * week;
   /*
    * Re-normalize it.
    */
   t = mktime(result);
   if ( t == (time_t)-1 )
   {
      return 0;
   }
   return result;
}

static time_t last_sunday_of_march_03_00_00_am(int year)
{
	struct tm *tm = nth_weekday_of_month(-1, sunday, march + 1, year);
	if(tm)
	{
		tm->tm_hour = 3;
		return mktime(tm);
	}
	
	return -1;
}

static time_t last_sunday_of_october_03_59_59_am(int year)
{
	struct tm *tm = nth_weekday_of_month(-1, sunday, october + 1, year);
	if(tm)
	{
		tm->tm_hour = 3;
		tm->tm_min = 59;
		tm->tm_sec = 59;
		return mktime(tm);
	}
	
	return -1;
}

static void handle_dst(struct tm &tm)
{
	// Add DST between last sunday of march 03:00 and last sunday of october 04:00
	
	time_t current = mktime(&tm);
	if(current >= last_sunday_of_march_03_00_00_am(tm.tm_year) &&
		current <= last_sunday_of_october_03_59_59_am(tm.tm_year))
	{
		// Add DST
		if(++tm.tm_hour > 23)
			mktime(&tm); // Renormalize
	}
}