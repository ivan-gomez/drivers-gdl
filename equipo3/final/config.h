#define IOC_MAGIC   0x94

#define BLAST_IOC_SET_VOLUME		_IOW(IOC_MAGIC, 0x1, char)			/* Defines our ioctl call. */
#define BLAST_IOC_SET_SAMPLE_RATE	_IOW(IOC_MAGIC, 0x2, char)			/* Defines our ioctl call. */
#define BLAST_IOC_SET_NUMERATOR		_IOW(IOC_MAGIC, 0x3, char[32])		/* Defines our ioctl call. */
#define BLAST_IOC_SET_DENOMINATOR	_IOW(IOC_MAGIC, 0x4, char[32])		/* Defines our ioctl call. */
#define BLAST_IOC_PLAY				_IO(IOC_MAGIC, 0x5 )				/* Defines our ioctl call. */
#define BLAST_IOC_STOP				_IO(IOC_MAGIC, 0x6 )				/* Defines our ioctl call. */

#define CMD_NEW_SAMPLE	0x00
#define CMD_SET_VOLUME	0x01
#define CMD_NUMERATOR	0x02
#define CMD_DENOMINATOR	0x03
#define CMD_PLAY		0x04
#define CMD_SET_RATE	0x05
#define CMD_STOP		0x06
#define CMD_RESET		0x00
