#include <stdio.h>
#include <stdlib.h>
#include "framebuffer.h"
#include "snesdriver.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

//What we want to do with this program is read for input from the SNES controller in one thread, and refresh the screen at a rate of 24 frames per second.
//We'll move a small 32 x 32 pixel box within a larger box. So, if one of the D-Pad buttons is pressed on the controller, the box'll move 32 pixels in that direction.
//Let's say the box moves 2 pixels per frame, so it would take 16 frames to make a move. 

typedef enum dir {UP, DOWN, LEFT, RIGHT, NONE} Dir; //We'll use the "dir" enumeration to specify which direction to move in.
struct fbs framebufferstruct;

typedef struct {
	int color;
	int x, y;
} Pixel;

typedef struct player
{
	int posx, posy; //The x and y position of the top left pixel
} Player;
typedef struct car
{
	int posx, posy;
	int width;
	int speed;
} Car;

void drawPixel(Pixel *pixel){
	long int location = (pixel->x +framebufferstruct.xOff) * (framebufferstruct.bits/8) + (pixel->y+framebufferstruct.yOff) * framebufferstruct.lineLength;
	*((unsigned short int*)(framebufferstruct.fptr + location)) = pixel->color;
}

void drawBack(Pixel *pix, Pixel *screen)
{
	int index;
	//This method will draw a red rectangle that's 1000 pixels wide and 700 pixels tall.
	for (int y = 0; y < 704; y++)
	{
		for (int x = 0; x < 1000; x++)
		{
			pix->color = 0xF800;
			pix->x = x;
			pix->y = y;
			index = (y * 1000) + x;
			screen[index] = *pix;
		}
	}	
}

void drawPlayer(Player *play, Pixel *pix, Pixel *screen)
{
	int x = play->posx;
	int y = play->posy;
	int index;
	int xend = x + 31;
	int yend = y + 31;
	for (y; y < yend; y++)
	{
		for (x = play->posx; x < xend; x++)
		{
			pix->color = 0xF00; //Hopefully this will be green
			pix->x = x;
			pix->y = y;
			index = (y * 1000) + x;
			screen[index] = *pix; //Places the player pixel into the "screen" array of pixels
		}
	}
}

void movecar(Pixel *pix, Pixel *screen, Car *carpoi)
{
	int x = carpoi->posx;
	x += carpoi->speed;
	if (x > 1000 && carpoi->speed > 0) x = -(carpoi->width); //If the car is moving from left to right and moves beyond the right bound, respawn it on the left side
	if (x < -(carpoi->width) && carpoi->speed < 0) x = 1000 + carpoi->width; //If the car is moving from right to left and moves beyond the left bound, respawn it on the right side.
	carpoi->posx = x;
	int y = carpoi->posy;
	int index;
	int xend = x + (carpoi->width - 1);
	int yend = y + 31;
	for (y; y < yend; y++)
	{
		for (x = carpoi->posx; x < xend; x++)
		{
			if (x >= 0 && x <= 1000)	//We only want to place a pixel into the screen array if it's in these bounds
			{
				pix->color = 0xFF; //This should be blue
				pix->x = x;
				pix->y = y;
				index = (y * 1000) + x;
				screen[index] = *pix;
			}
		}
	}	
}

int checkcollision(Player *play, Car *carpoi, int carnum)
{
	for (int i = 0; i < carnum; i++)
	{
		if (play->posy > ((carpoi + i)->posy - 32) && play->posy < ((carpoi + i)->posy + 31))	//If the player is in this range, then it's possible they'll get hit by a car
		{
			if ((carpoi + i)->speed < 0 && (play->posx + 31) > (carpoi + i)->posx && play->posx < (carpoi + i)->posx + ((carpoi + i)->width - 1))
			{
				//That logical statement is a bit complicated, but basically it means that if the car is moving left, and the player is within a certain range of it, they'll get hit
				play->posx = 0;
				play->posy = 0; //For now, if the player gets hit, all we'll do is move them to the corner.
				return 1;
			}
			if ((carpoi + i)->speed > 0 && play->posx > (carpoi + i)->posx && (play->posx) < ((carpoi + i)->posx + ((carpoi + i)->width - 1)))
			{
				play->posx = 0;
				play->posy = 0;
				return 1;
			}
		}
	}
	return 0;
}

int refreshscreen(Player *play, Pixel *pix, Pixel *screen, Car *carpoi, int carnum)
{
		drawBack(pix, screen);
		drawPlayer(play, pix, screen);
		for (int i = 0; i < carnum; i++) movecar(pix, screen, (carpoi + i));
		for (int i = 0; i < 704000; i++) drawPixel((screen + i));
		return checkcollision(play, carpoi, carnum);
}

void moveplayer(Player *play, Pixel *pix, unsigned short bitfield, Pixel *screen, Car *carpoi, int carnum) 
{
	int x = play->posx;
	int y = play->posy;
	int i;
	if ((bitfield & U_DIR) == 0)
	{
		if ((y - 32) >= 0) //If (y - 32) < 0, then moving up would move the player out of bounds (which would be bad)
		{
			for (i = 0; i < 16; i++)
			{
				y -= 2;
				play->posy = y;
				if (refreshscreen(play, pix, screen, carpoi, carnum) == 1) break;
				Wait(20833); //41666 microseconds is approximate to 1/24th of a second
			}
		}
	}
	else if ((bitfield & D_DIR) == 0)
	{
		if ((y + 32) <= 672)
		{
			for (i = 0; i < 16; i++)
			{
				y += 2;
				play->posy = y;
				if (refreshscreen(play, pix, screen, carpoi, carnum) == 1) break;
				Wait(20833);
			}
		}
	}
	else if ((bitfield & L_DIR) == 0)
	{
		if ((x - 32) >= 0)
		{
			for (i = 0; i < 16; i++)
			{
				x -= 2;
				play->posx = x;
				if (refreshscreen(play, pix, screen, carpoi, carnum) == 1) break;
				Wait(20833);
			}	
		}
	}
	else if ((bitfield & R_DIR) == 0)
	{
		if ((x + 32) <= 960)
		{
			for (i = 0; i < 16; i++)
			{
				x += 2;
				play->posx = x;
				if (refreshscreen(play, pix, screen, carpoi, carnum) == 1) break;
				Wait(20833);
			}	
		}
	}
}

int main(int argc, char **argv)
{
	unsigned int *gpio = getGPIOPtr(); //Now, the base memory address for the GPIO should be stored in this pointer
	Init_GPIO(gpio);
	
	framebufferstruct = initFbInfo();
	
	Pixel *pix;
	pix = malloc(sizeof(Pixel));
	Player *play;
	play = malloc(sizeof(Player));
	Pixel *screen;
	screen = (Pixel *)malloc(704000 * sizeof(Pixel)); //This will create an array of pixels
	Car *npo;
	npo = (Car *)malloc(8 * sizeof(Car));
	int pos = 0;
	int i;
	for (i = 0; i < 4; i++)
	{
		npo[i].width = 48;		//128
		npo[i].posy = 128;		// 64
		npo[i].posx = pos;		//192
		pos += 256;
		npo[i].speed = 4;
	}
	pos = 1000;
	for (i = 4; i < 8; i++)
	{
		npo[i].width = 48;
		npo[i].posy = 192;
		npo[i].posx = pos;
		pos -= 256;
		npo[i].speed = -4;
	}
	
	play->posy = 32;
	play->posx = 32;
	
	unsigned short button = NOBUTT;
	Dir direct;
	
	refreshscreen(play, pix, screen, npo, 8);
	
	int looptrue = 1;
	while (looptrue)
	{
		button = Read_SNES(gpio);
		moveplayer(play, pix, button, screen, npo, 8);
		refreshscreen(play, pix, screen, npo, 8);
		Wait(20833);
		if ((button & START_B) == 0) looptrue = 0;
	}
	
	free(pix);
	free(play);
	free(screen);
	free(npo);
	
	return 0;
}
