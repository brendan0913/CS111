// NAME: Brendan Rossmango
// EMAIL: brendan0913@ucla.edu
// ID: 505370692

#ifdef DUMMY
#define MRAA_GPIO_IN 0
#define MRAA_SUCCESS 0
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
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>
#include <poll.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/ssl.h>

int scale_flag = 1; // default Fahrenheit, F is 1, C is 2
int period = 1; // default 1 second
char* log_filename = NULL;
FILE *fp = NULL;
int log_flag = 0;
int is_stopped = 0;
int sockfd = -1;
char* id = "";
char* hostname = "";
int port_num = 0;
SSL *ssl_client = NULL;

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
		exit(2);
	}
	button = mraa_gpio_init(60);
	if (button == NULL){
		fprintf(stderr, "Error initializing GPIO pin 60 for button\n");
		mraa_deinit();
		exit(2);
	}
}
#endif

// From discussion slides
float convert_temperature(int reading){ // reading is mraa_aio_read(temperature)
	float R = 1023.0/((float) reading) - 1.0;
	R *= 100000.0;
	float C = 1.0/(log(R/100000.0)/4275 + 1/298.15) - 273.15;
	float F = (C * 9)/5 + 32;
	if (scale_flag == 1) return F;
	else if (scale_flag == 2) return C;
	else{
		fprintf(stderr, "scale option error\n");
		exit(2);
	}
}

void shutdown_and_exit(){
	struct timespec start_time;
	struct tm* formatted_time;
	clock_gettime(CLOCK_REALTIME, &start_time);
	formatted_time = localtime(&(start_time.tv_sec));
	char time_buf[9];
	strftime(time_buf, 9, "%H:%M:%S", formatted_time);
	char buf[19];
	sprintf(buf, "%s SHUTDOWN\n", time_buf);
	SSL_write(ssl_client, buf, strlen(buf));
	if (log_flag){
		fprintf(fp, "%s", buf);
		fclose(fp);
	}
	if (mraa_aio_close(temperature) != MRAA_SUCCESS){
		fprintf(stderr, "Error closing temperature sensor\n");
		exit(2);
	}
    if (mraa_gpio_close(button) != MRAA_SUCCESS){
		fprintf(stderr, "Error closing button\n");
		exit(2);
	}
	SSL_shutdown(ssl_client);
	SSL_free(ssl_client);
	exit(0);
}

void client_connect(char* hostname, int port_num){
    struct hostent* server;
    struct sockaddr_in serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "Error opening socket to server\n");
        exit(2);
    }
    if ((server = gethostbyname(hostname)) == NULL){
        fprintf(stderr, "Error connecting to server at hostname %s\n", hostname);
        exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port_num);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        fprintf(stderr, "Error connecting to client socket\n");
        exit(2);
    }
}

int main(int argc, char **argv){
	static struct option long_options[] =
	{
		{"scale", required_argument, NULL, 's'},
		{"period", required_argument, NULL, 'p'},
		{"log", required_argument, NULL, 'l'},
        {"host", required_argument, NULL, 'h'},
        {"id", required_argument, NULL, 'i'},
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
            case 'h':
                hostname = optarg;
                break;
            case 'i':
                id = optarg;
                break;
			default:
				fprintf(stderr, "Incorrect usage: ./lab4c_tls --host=hostname --id=id_num port_num [--scale=[F][C]] [--period=interval] [--log=filename]\n");
				exit(1);
		}
	}
    if (optind < argc){
        port_num = atoi(argv[optind]);
        if (port_num < 0) {
            fprintf(stderr, "Error: invalid port number\n");
            exit(1);
        }
    }
	if (strlen(id) != 9){
		fprintf(stderr, "Error: 9-digit id number required\n");
		exit(1);
	}
    client_connect(hostname, port_num);
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
	SSL_CTX* new_context = SSL_CTX_new(TLSv1_client_method());
	if (new_context == NULL){
		fprintf(stderr, "Error setting new SSL context\n");
		exit(2);
	}
	ssl_client = SSL_new(new_context);
	SSL_set_fd(ssl_client, sockfd);
	if (SSL_connect(ssl_client) != 1){
		fprintf(stderr, "Error starting SSL session\n");
		exit(2);
	}
	char id_buf[16];
	sprintf(id_buf, "ID=%s\n", id);
	SSL_write(ssl_client, id_buf, strlen(id_buf));
    if (log_flag){
        fprintf(fp, "%s", id_buf);
        fflush(fp);
    }
	initialize_sensors();
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	struct pollfd pollfds = { sockfd, POLLIN, 0 };
	struct timespec start_time;
	struct tm* formatted_time;
	time_t next_time;
	float temp = 0.0;
    char commands[256];
	while (1){
		if (poll(&pollfds, 1, 1000) < 0){	// timeout 1 sec
			fprintf(stderr, "Error polling input\n");
			exit(1);
		}
        // Process commands from server
		if ((pollfds.revents & POLLIN)){
            int nbytes = SSL_read(ssl_client, commands, 256);
            if (nbytes < 0){
                fprintf(stderr, "Error with read of ssl client\n");
                exit(2);
            }
			char *c = commands;
			char *end = &commands[nbytes];
            while (c < end){
                char *i = c;
                int length = 0;
                while (*i != '\n' && i < end){
                    length++; i++;
                }
                char* buf = malloc(length + 2);
                memcpy(buf, c, length);
                buf[length] = '\n';
                buf[length + 1] = '\0';
                // Process commands in buf
                if (!strcmp(buf, "OFF\n")){
                    if (log_flag) fprintf(fp, "%s", buf);
                    shutdown_and_exit(); // shutdown and exit
                }
                else if (!strcmp(buf, "START\n")){
                    is_stopped = 0;
                    if (log_flag) fprintf(fp, "%s", buf);
                }
                else if (!strcmp(buf, "STOP\n")){
                    is_stopped = 1;
                    if (log_flag) fprintf(fp, "%s", buf);
                }
                else if (!strcmp(buf, "SCALE=F\n")){
                    scale_flag = 1;	// Fahrenheit
                    if (log_flag) fprintf(fp, "%s", buf);
                }
                else if (!strcmp(buf, "SCALE=C\n")){
                    scale_flag = 2;	// Celsius
                    if (log_flag) fprintf(fp, "%s", buf);
                }
                else if (strlen(buf) > 4 && buf[0] == 'L' && buf[1] == 'O' && buf[2] == 'G'){ // LOG
                    if(log_flag) fprintf(fp, "%s", buf);
                }
                else if (strlen(buf) > 7 && buf[0] == 'P' && buf[1] == 'E' && buf[2] == 'R' && // PERIOD=
                        buf[3] == 'I' && buf[4] == 'O' && buf[5] == 'D' && buf[6] == '='){
                    if (log_flag) fprintf(fp, "%s", buf);
                    int j;
                    int num_length = 0;
                    for (j = 7; j < length + 2; j++){
                        if (isdigit(buf[j])) num_length++;
                        else break;
                    }
                    if (num_length){
                        char* num_buff = malloc((num_length + 1) * 4);
                        memcpy(num_buff, buf + 7, num_length);
                        num_buff[num_length] = '\0';
                        period = atoi(num_buff);
                    }
                }
                else{ // invalid command
                    if (log_flag) fprintf(fp, "%s", c);
                }
                fflush(fp);
                c = i + 1; // place c 1 place past the \n
            }
		}
		clock_gettime(CLOCK_REALTIME, &start_time);
		temp = convert_temperature(mraa_aio_read(temperature));
		// If time to report temperature and is not stopped
		if (start_time.tv_sec >= next_time && !is_stopped){
			formatted_time = localtime(&(start_time.tv_sec));
			char time_buf[9];
			strftime(time_buf, 9, "%H:%M:%S", formatted_time);
			char buf[14];
			sprintf(buf, "%s %.1f\n", time_buf, temp);
			SSL_write(ssl_client, buf, strlen(buf));
            if (log_flag){
                fprintf(fp, "%s", buf);
                fflush(fp);
            }
			next_time = start_time.tv_sec + period;
		}
	}
	SSL_shutdown(ssl_client);
	SSL_free(ssl_client);
	exit(0);
}
