#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>

#define IOC_MAGIC   0x94

#define MYCALC_IOC_SET_NUM1 _IOW(IOC_MAGIC, 0x1, long)         // defines our ioctl call.
#define MYCALC_IOC_SET_NUM2 _IOW(IOC_MAGIC, 0x2, long)         // defines our ioctl call.

#define MYCALC_IOC_SET_OPERATION    _IOW(IOC_MAGIC, 0x3, char) // defines our ioctl call.
#define MYCALC_IOC_GET_RESULT   _IO(IOC_MAGIC, 0x4 )           // defines our ioctl call.
#define MYCALC_IOC_DO_OPERATION _IO(IOC_MAGIC, 0x5 )           // defines our ioctl call.

int main ()
{
    char wbuffer[512];
    char rbuffer[512];

    int f, i, ret;
    long result;
    ssize_t bytes;

    /* Open a file */
    f = open ("/dev/mycalc0", O_RDWR, 0666);

    if (f == -1) {
        printf ("\nfailed to open file.");
        return 1;
    }

    /* Write to buffer */
    sprintf (wbuffer, "5+6");

    /* Write to file */
    write (f, wbuffer, 5);

    /* Read from file */
    bytes = read (f, rbuffer, sizeof (rbuffer));
    rbuffer[bytes] = 0;
    ret = ioctl (f, MYCALC_IOC_SET_NUM1, 9);
    ret = ioctl (f, MYCALC_IOC_SET_NUM2, 2);
    ret = ioctl (f, MYCALC_IOC_SET_OPERATION, '+');
    ret = ioctl (f, MYCALC_IOC_DO_OPERATION);
    result = ioctl (f, MYCALC_IOC_GET_RESULT);

    printf ("\n----------------- \n");
    printf ("\nSuccessfully read %d bytes.\n", bytes);
    printf ("\nDATA: %s\n", rbuffer);
    printf ("----------------- \n", rbuffer);

    printf ("\nOperation 2 result: %ld \n", result);

    /* Close file */
    close (f);
}