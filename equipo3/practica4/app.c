#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include "config.h"

#if ENABLE_INTERRUPT
	//#define 
#else
	#define  WAIT_MANUALLY 1
#endif

#define IOC_MAGIC   0x94

#define PARA_IOC_SET_WRITE   	_IO(IOC_MAGIC, 0x1 )           /* Defines our ioctl call. */
#define PARA_IOC_SET_READ 		_IO(IOC_MAGIC, 0x2 )           /* Defines our ioctl call. */
#define PARA_IOC_SET_STROBE   	_IO(IOC_MAGIC, 0x3 )           /* Defines our ioctl call. */
#define PARA_IOC_CLEAR_STROBE 	_IO(IOC_MAGIC, 0x4 )           /* Defines our ioctl call. */

int main ()
{
    char wbuffer[512];
    char rbuffer[512];
    int f, i, ret;
    long result;
    ssize_t bytes;

    /* Open a file */
    f = open ("/dev/para0", O_RDWR, 0666);

    if (f == -1) {
        printf ("failed to open paralel port.\n");
        return 1;
    }

	/* Configure as output */
	ret = ioctl (f, PARA_IOC_SET_WRITE );

	/* Flash leds */
	sprintf (wbuffer, "%c", 0xFF );
	write (f, wbuffer, 1);
	usleep(100000);
	sprintf (wbuffer, "%c", 0x00 );
	write (f, wbuffer, 1);
		
    /* Write to buffer and file*/
    ret = 0x80;
    for(i=0;i<8;i++){
		sprintf (wbuffer, "%c", ret );
		write (f, wbuffer, 1);
		ret >>= 1;
		usleep(100000);
	}
    
	/* Wait for enter */
	#if WAIT_MANUALLY
	printf ("Enter a string to continue.. \n");
	scanf( "%s", wbuffer );
	#else
	printf ("Execution will pause until data ready.. \n");
	#endif
	
	/* Configure as input */
    ret = ioctl (f, PARA_IOC_SET_READ );
    
    /* Read from file */
    bytes = read (f, rbuffer, sizeof (rbuffer));
    
    rbuffer[bytes] = 0;
    
    printf ("\n----------------- \n");
    printf ("\nSuccessfully read %d bytes.\n", bytes);
    printf ("\nDATA: 0x%X\n", (0xFF) & rbuffer[0]);
    printf ("----------------- \n", rbuffer);

    /* Close file */
    close (f);
}
