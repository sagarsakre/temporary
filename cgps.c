#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <curses.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#include "gpsd_config.h"
#include "gps.h"
#include "compiler.h"   /* for UNUSED */
#include "gpsdclient.h"
#include "revision.h"
#include "os_compat.h"

static struct gps_data_t gpsdata;
static struct fixsource_t source;
static enum deg_str_type deg_type = deg_dd;


static void push_gps_data(struct gps_data_t *gpsdata);

int main(int argc, char *argv[])
{

        int wait_clicks = 0;  /* cycles to wait before gpsd timeout */
        unsigned int flags = WATCH_ENABLE;
	
	gpsd_source_spec(NULL, &source);
	
	/* Open the stream to gpsd. */
        if (gps_open(source.server, source.port, &gpsdata) != 0) {
                    (void)fprintf(stderr,
                                 "cgps: no gpsd running or network error: %d, %s\n",
                                  errno, gps_errstr(errno));
            exit(EXIT_FAILURE);
        }

        if (source.device != NULL)
        flags |= WATCH_DEVICE;
        (void)gps_stream(&gpsdata, flags, source.device);


	for (;;) {
        	int c;

		/* wait 1/2 second for gpsd */
       		if (!gps_waiting(&gpsdata, 500000)) {
  	        /* 240 tries at .5 Sec a try is a 2 minute timeout */
        	   	if ( 240 < wait_clicks++ ) {
				(void)gps_close(&gpsdata);
				(void)fprintf(stderr,"TIMEOUT: No GPS data\n");
               	    		 exit(0);
				 }
      		} else {
            		wait_clicks = 0; 
    		        errno = 0;
           		if (gps_read(&gpsdata) == -1) {
               			 (void)fprintf(stderr, "cgps: socket error 4\n");
				 printf("Error soket\n");
				 (void)fprintf(stderr,"%s\n",  strerror(errno));
				(void)gps_close(&gpsdata);
				 exit(0);
           		} else {
                		/* Here's where updates go now that things are established. */
                    		push_gps_data(&gpsdata);
            		}
        	}
	}
}

static void push_gps_data(struct gps_data_t *gpsdata)
{
	if (gpsdata->fix.mode >= MODE_2D && isnan(gpsdata->fix.longitude) == 0) {
	        (void)fprintf(stderr,"Longitude %s ",deg_to_str(deg_type, fabs(gpsdata->fix.longitude)));
	}

	if (gpsdata->fix.mode >= MODE_2D && isnan(gpsdata->fix.latitude) == 0) {
       		(void)fprintf(stderr,"Latitude %s ",  deg_to_str(deg_type, fabs(gpsdata->fix.latitude)));
	}
	if (gpsdata->fix.mode >= MODE_3D && isnan(gpsdata->fix.altitude) == 0) {
        	(void)fprintf(stderr, "Altitude %.3f \n",  gpsdata->fix.altitude);
	}
       // (void)printf("Speed %.2f ",  gpsdata->fix.speed);
       // (void)printf("Climb %.2f",  gpsdata->fix.climb);
}

