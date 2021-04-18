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

void refreshscreen(Player *play, Pixel *pix, Pixel *screen)
{
		drawBack(pix, screen);
		drawPlayer(play, pix, screen);
		for (int i = 0; i < 704000; i++)
		drawPixel((screen + i));
}

void moveplayer(Player *play, Pixel *pix, unsigned short bitfield, Pixel *screen)
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
				refreshscreen(play, pix, screen);
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
				refreshscreen(play, pix, screen);
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
				refreshscreen(play, pix, screen);
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
				refreshscreen(play, pix, screen);
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
	
	play->posy = 32;
	play->posx = 32;
	
	unsigned short button = NOBUTT;
	Dir direct;
	
	refreshscreen(play, pix, screen);
	
	int looptrue = 1;
	while (looptrue)
	{
		while (button == NOBUTT) button = Read_SNES(gpio);
		moveplayer(play, pix, button, screen);
		if ((button & START_B) == 0) looptrue = 0;
		button = NOBUTT;
	}
	
	free(pix);
	free(play);
	free(screen);
	
	return 0;
}
