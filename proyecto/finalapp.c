#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "/usr/local/include/directfb.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fs.h>

#define FINAL_IOC_MAGIC		'R'
#define FINAL_IOC_SET_INP	_IOW(FINAL_IOC_MAGIC, 0xE0, int)
#define FINAL_IOC_SET_OUT	_IOW(FINAL_IOC_MAGIC, 0xE1, int)
#define FINAL_IOC_STROBE_HL	_IOW(FINAL_IOC_MAGIC, 0xE2, int)

#define WHITE 0xff, 0xff, 0xff
#define BLACK 0x00, 0x00, 0x00
#define GRAY  0x7f, 0x7f, 0x7f
#define BLUE  0x10, 0x10, 0xff
#define RED   0xff, 0x10, 0x10
#define GREEN 0x10, 0xff, 0x10
#define YELLOW  0xff, 0xff, 0x10
#define PURPLE  0xbf, 0x00, 0xff

#define BACKGROUND WHITE

#define TRUE 1
#define FALSE 0

#define abs(x) ((x) > 0 ? (x) : -(x))

static IDirectFB *dfb;
static IDirectFBSurface *primary;

static int screen_width;
static int screen_height;

enum { black, gray, blue, red, green, yellow, purple } color;

#define DFBCHECK(x...)                                         \
  {                                                            \
    DFBResult err = x;                                         \
                                                               \
    if (err != DFB_OK)                                         \
      {                                                        \
        fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
      }                                                        \
  }

int sqrtr(int n)
{
	int a;
	for (a = 0; a * a < n; a++)
		;
	return a - 1;
}

void set_circle_lim(int xc, int yc, int r, int i, int *init, int *fin)
{
	int d = sqrtr(r * r - (yc - i) * (yc - i));
	*init = xc - d;
	*fin = xc + d;
}

int request_position(int fd, int *xc, int *yc)
{
	char num[3];
	char temp;

	read(fd, num, 3);

	if (num[2] != 2) {
		temp = 0;
		temp += (num[0] & 0b1000) >> 3;
		temp += (num[0] & 0b0100) >> 1;
		temp += (num[0] & 0b0010) << 1;
		temp += (num[0] & 0b0001) << 3;
		num[0] = temp;

		temp = 0;
		temp += (num[1] & 0b1000) >> 3;
		temp += (num[1] & 0b0100) >> 1;
		temp += (num[1] & 0b0010) << 1;
		temp += (num[1] & 0b0001) << 3;
		num[1] = temp;

		num[0] -= (num[0] < 8) ? 7 : 8;
		num[1] -= (num[1] < 8) ? 7 : 8;

		num[0] *= -1;
		num[1] *= -1;
	}
	*xc += num[0];
	*yc += num[1];

	return num[2];
}

void change_color()
{
	switch (color) {
	case black:
		color = gray;
		break;
	case gray:
		color = blue;
		break;
	case blue:
		color = red;
		break;
	case red:
		color = green;
		break;
	case green:
		color = yellow;
		break;
	case yellow:
		color = purple;
		break;
	case purple:
		color = black;
		break;
	}
}

int main(int argc, char **argv)
{
	int i;
	int init, fin;
	int xc, yc, r;
	char clean_past = FALSE;
	int fd;

	dfb = NULL;
	primary = NULL;
	screen_width  = 0;
	screen_height  = 0;

	DFBSurfaceDescription dsc;
	DFBCHECK(DirectFBInit(&argc, &argv));
	DFBCHECK(DirectFBCreate(&dfb));
	DFBCHECK(dfb->SetCooperativeLevel(dfb, DFSCL_FULLSCREEN));
	dsc.flags = DSDESC_CAPS;
	dsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK(dfb->CreateSurface(dfb, &dsc, &primary));
	DFBCHECK(primary->GetSize(primary, &screen_width, &screen_height));
	/*screen_width=1280;
	screen_height=1024;*/

	/*
	Clear the screen by filling a rectangle with the size of the surface.
	Default color is black, default drawing flags are DSDRAW_NOFX.
	*/
	DFBCHECK(primary->SetColor(primary, BACKGROUND, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width,
		screen_height));

	DFBCHECK(primary->SetColor(primary, BLUE, 0xff));

	xc = 500;
	yc = 500;
	r = 15;
	color = gray;

	/*Open device*/
	fd = open("/dev/final0", O_RDWR);
	if (fd < 1) {
		printf("EL archivo no ha podido ser abierto");
		return -1;
	}

	/*Set port as input*/
	ioctl(fd, FINAL_IOC_SET_INP);

	/*Turn power on!*/
	ioctl(fd, FINAL_IOC_STROBE_HL);

	while (TRUE) {

		/*Change circle position*/
		if (request_position(fd, &xc, &yc) == 1) {
			/*Then button is 1, change color and clear screen*/
			change_color();
			DFBCHECK(primary->SetColor(primary, BACKGROUND, 0xff));
			for (i = 0; i < screen_height; i++) {
				DFBCHECK(primary->DrawLine(primary, 0, i,
					screen_width, i));
			}
			DFBCHECK(primary->Flip(primary, NULL, 0));
		}

		/*End condition*/
		if (xc + r >= screen_width || xc - r <= 0 ||
			yc + r >= screen_height || yc - r <= 0)
			break;

		/*Draw circle*/
		switch (color) {
		case black:
			DFBCHECK(primary->SetColor(primary,
				BLACK, 0xff));
			break;
		case gray:
			DFBCHECK(primary->SetColor(primary,
				GRAY, 0xff));
			break;
		case blue:
			DFBCHECK(primary->SetColor(primary,
				BLUE, 0xff));
			break;
		case red:
			DFBCHECK(primary->SetColor(primary,
				RED, 0xff));
			break;
		case green:
			DFBCHECK(primary->SetColor(primary,
				GREEN, 0xff));
			break;
		case yellow:
			DFBCHECK(primary->SetColor(primary,
				YELLOW, 0xff));
			break;
		case purple:
			DFBCHECK(primary->SetColor(primary,
				PURPLE, 0xff));
			break;
		}
		if (yc + r < screen_height && yc - r > 0)
			for (i = yc - r; i < yc + r; i++) {
				set_circle_lim(xc, yc, r, i, &init, &fin);
				DFBCHECK(primary->DrawLine(primary, init, i,
					fin, i));
			}
		DFBCHECK(primary->Flip(primary, NULL, 0));

		/*Delay*/
		usleep(33000);

		if (clean_past) {
			/*Erase existing circle*/
			DFBCHECK(primary->SetColor(primary, BACKGROUND, 0xff));
			if (yc + r < screen_height && yc - r > 0)
				for (i = yc - r; i < yc + r; i++) {
					set_circle_lim(xc, yc, r, i, &init,
						&fin);
					DFBCHECK(primary->DrawLine(primary,
						init, i, fin, i));
				}
			DFBCHECK(primary->Flip(primary, NULL, 0));
		}
	}

	close(fd);

	/*Cleanup in a stack like style.*/
	primary->Release(primary);
	dfb->Release(dfb);

	return 23;
}


