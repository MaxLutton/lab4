#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/aio.h>
#include <time.h>
#include <math.h>
sig_atomic_t volatile run_flag = 1;

void do_when_interrupted(int sig)
{
	if (sig == SIGINT)
		run_flag = 0;
}

float convert_temp(int temp)
{
		int B = 4275;
		float R = 1023.0/((float)temp) - 1.0;
		R = 100000.0*R;
		float temperature=1.0/(log(R/100000.0)/B+1/298.15)-273.15;
		
		temperature = (1.8 * temperature) + 32;
		
		return temperature;

}
int main()
{
	uint16_t value;
	mraa_aio_context temp = mraa_aio_init(0);

	FILE* log_file = fopen("log1", "w+");

	signal(SIGINT, do_when_interrupted);
	while (run_flag)
	{
		
		value = mraa_aio_read(temp);
		float ftemp = convert_temp(value);

		struct tm* time_info;
		time_t time_raw;
		time(&time_raw);
		time_info = localtime(&time_raw);

		printf("%d:%d:%d %.1f\n", time_info->tm_hour, 
		    	time_info->tm_min, time_info->tm_sec, ftemp);
		fprintf(log_file, "%d:%d:%d %.1f\n", time_info->tm_hour, 
		    	time_info->tm_min, time_info->tm_sec, ftemp);
                fflush(log_file);

		sleep(1);
	}
	mraa_aio_close(temp);
	fclose(log_file);

	return 0;
}
