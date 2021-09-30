// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#ifdef DUMMY
#define MRAA_GPIO_IN 0
#define MRAA_SUCCESS 0
#define MRAA_GPIO_EDGE_RISING 1
typedef int mraa_aio_context;
typedef int mraa_gpio_context;

// mraa_aio functions (meaningless operations to avoid warnings)
mraa_aio_context mraa_aio_init(int p){
	p++;
	return 0;
}

int mraa_aio_read(mraa_aio_context c){
	c++;
	return 650;
}

int mraa_aio_close(mraa_aio_context c){
	c++;
	return 0;
}

// mraa_gpio functions (meaningless operations to avoid warnings)
mraa_gpio_context mraa_gpio_init(int p){
	p++;
	return 0;
}

void mraa_gpio_dir(mraa_gpio_context c, int d){
	c++; d++;
}

int mraa_gpio_read(mraa_gpio_context c){
	c++;
	return 0;
}

void mraa_gpio_isr(mraa_gpio_context c, int e, void(*fptr)(void *), void* arg){
	if (fptr != arg){
		c++; e++;
	}
}

int mraa_gpio_close(mraa_gpio_context c){
	c++;
	return 0;
}

void mraa_deinit(){
	return;
}

#else
#include "mraa.h"
#include <mraa/aio.h>
#include <mraa/gpio.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>
#include <poll.h>
#include <ctype.h>

int scale_flag = 1; // default Fahrenheit, F is 1, C is 2
int period = 1; // default 1 second
char* log_filename = NULL;
FILE *fp = NULL;
int log_flag = 0;
int is_stopped = 0;

mraa_aio_context temperature;
mraa_gpio_context button;

#ifdef DUMMY

void initialize_sensors(){
	temperature = mraa_aio_init(1);
	button = mraa_gpio_init(60);
}

#else
void initialize_sensors(){
	temperature = mraa_aio_init(1);
	if (temperature == NULL){
		fprintf(stderr, "Error initializing AIO pin 1 for temperature sensor\n");
		mraa_deinit();
		exit(1);
	}
	button = mraa_gpio_init(60);
	if (button == NULL){
		fprintf(stderr, "Error initializing GPIO pin 60 for button\n");
		mraa_deinit();
		exit(1);
	}
}
#endif

// From discussion slides
float convert_temperature(int reading){ // reading is mraa_aio_read(temperature)
	float R = 1023.0/((float) reading) - 1.0;
	R *= 100000.0;
	float C = 1.0/(log(R/100000.0)/4275 + 1/298.15) - 273.15;
	float F = (C*9)/5 + 32;
	if (scale_flag == 1) return F;
	else if (scale_flag == 2) return C;
	else{
		fprintf(stderr, "scale option error\n");
		exit(1);
	}
}

void shutdown(void *arg){
	if (arg != NULL) return;
	struct timespec start_time;
	struct tm* formatted_time;
	clock_gettime(CLOCK_REALTIME, &start_time);
	formatted_time = localtime(&(start_time.tv_sec));
	char time_buf[9];
	strftime(time_buf, 9, "%H:%M:%S", formatted_time);
	printf("%s SHUTDOWN\n", time_buf);
	if (log_flag){
		fprintf(fp, "%s SHUTDOWN\n", time_buf);
		fclose(fp);
	}
	if (mraa_aio_close(temperature) != MRAA_SUCCESS){
		fprintf(stderr, "Error closing temperature sensor\n");
		exit(1);
	}
	if (mraa_gpio_close(button) != MRAA_SUCCESS){
		fprintf(stderr, "Error closing button\n");
		exit(1);
	}
	exit(0);
}

int main(int argc, char **argv){
	static struct option long_options[] =
	{
		{"scale", required_argument, NULL, 's'},
		{"period", required_argument, NULL, 'p'},
		{"log", required_argument, NULL, 'l'},
		{0, 0, 0, 0}
	};
	int c;
	while (1){
		c = getopt_long(argc, argv, "", long_options, NULL);
		if (c == -1) break;
		switch (c){
			case 's':  
				if (optarg[0] == 'C') scale_flag = 2;
				else{
					fprintf(stderr, "Incorrect scale option '%s' for --scale: options are F(ahrenheit) or C(elsius)\n", optarg);
					exit(1);
				}
				break;
			case 'p':
				period = atoi(optarg);
				break;
			case 'l':
				log_filename = optarg;
				log_flag = 1;
				fp = fopen(log_filename, "w");	// write only
				if (fp == NULL){
					fprintf(stderr, "Error creating/writing to log file; strerror reports: %s\n", strerror(errno));
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Incorrect usage: ./lab4b [--scale=[F][C]] [--period=interval] [--log=filename]\n");
				exit(1);
		}
	}
	initialize_sensors();
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	struct pollfd pollfds = { 0, POLLIN, 0 };
	struct timespec start_time;
	struct tm* formatted_time;
	time_t next_time;
	float temp = 0.0;
	while (1){
		if (poll(&pollfds, 1, 1000) < 0){	// timeout 1 sec
			fprintf(stderr, "Error polling input\n");
			exit(1);
		}
		// On button press (rising edge), shutdown and exit
		mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, shutdown, NULL);
		clock_gettime(CLOCK_REALTIME, &start_time);
		temp = convert_temperature(mraa_aio_read(temperature));
		// If time to report temperature and is not stopped
		if (start_time.tv_sec >= next_time && !is_stopped){
			formatted_time = localtime(&(start_time.tv_sec));
			char time_buf[9];
			strftime(time_buf, 9, "%H:%M:%S", formatted_time);
			printf("%s %.1f\n", time_buf, temp);
			if (log_flag){
				fprintf(fp, "%s %.1f\n", time_buf, temp);
			}
			next_time = start_time.tv_sec + period;
		}
		// Process input commands
		if ((pollfds.revents & POLLIN)){
			char buf[128];
			fgets(buf, 128, stdin); // stops at \n
			// process input
			char *c = buf;
			while ((*c == ' ') || (*c == '\t')) // ignore beginning spaces and tabs
				++c;
			if (!strcmp(c, "OFF\n")){
				if (log_flag) fprintf(fp, "%s", c);
				shutdown(NULL); // shutdown and exit
			}
			else if (!strcmp(c, "START\n")){
				is_stopped = 0;
				if (log_flag) fprintf(fp, "%s", c);
			}
			else if (!strcmp(c, "STOP\n")){
				is_stopped = 1;
				if (log_flag) fprintf(fp, "%s", c);
			}
			else if (!strcmp(c, "SCALE=F\n")){
				scale_flag = 1;	// Fahrenheit
				if (log_flag) fprintf(fp, "%s", c);
			}
			else if (!strcmp(c, "SCALE=C\n")){
				scale_flag = 2;	// Celsius
				if (log_flag) fprintf(fp, "%s", c);
			}
			else if (strlen(c) > 7 && c[0] == 'P' && c[1] == 'E' && c[2] == 'R' && // PERIOD=num
					c[3] == 'I' && c[4] == 'O' && c[5] == 'D' && c[6] == '='){
				if (log_flag) fprintf(fp, "%s", c);
				int buf_len = strlen(c);
				int i;
				int num_length = 0;
				for (i = 7; i < buf_len; i++){
					if(isdigit(c[i])) num_length++;
					else break;
				}
				if (num_length){
					char* num_buff = malloc((num_length + 1) * 4);
					memcpy(num_buff, &c[7], num_length);
					num_buff[num_length] = '\0';
					period = atoi(num_buff);
				}
			}
			else{ // invalid command or LOG command
				if (log_flag) fprintf(fp, "%s", c);
			}
			fflush(fp);
		}
	}
	exit(0);
}
