#include <stdio.h>
#include <stdlib.h>
#include "framebuffer.h"
#include "snesdriver.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "numgraphics.h"

//What we want to do with this program is read for input from the SNES controller in one thread, and refresh the screen at a rate of 24 frames per second.
//We'll move a small 32 x 32 pixel box within a larger box. So, if one of the D-Pad buttons is pressed on the controller, the box'll move 32 pixels in that direction.
//Let's say the box moves 2 pixels per frame, so it would take 16 frames to make a move. 

struct fbs framebufferstruct;

typedef struct game
{
	int life;
	int time;
	int score;
	int level;
} Game;

void initgame(Game *gamepoi)
{
	gamepoi->life = 4;
	gamepoi->time = 90;
	gamepoi->score = 0;
	gamepoi->level = 1;
}

typedef struct {
	short color;
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

typedef struct log
{
	int posx, posy;
	int width;
	int speed;
} Log;

void drawPixel(Pixel *pixel){
	long int location = (pixel->x +framebufferstruct.xOff) * (framebufferstruct.bits/8) + (pixel->y+framebufferstruct.yOff) * framebufferstruct.lineLength;
	*((unsigned short int*)(framebufferstruct.fptr + location)) = pixel->color;
}

void drawBack(Pixel *pix, Pixel *screen)
{
	int index;
	for (int y = 0; y < 32; y++) //First loop will draw a red strip at the top of the screen
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
	for (int y = 32; y < 160; y++) //Second loop draws a blue strip (the river)
	{
		for (int x = 0; x < 1000; x++)
		{
			pix->color = 0xFF;
			pix->x = x;
			pix->y = y;
			index = (y * 1000) + x;
			screen[index] = *pix;
		}
	}
	for (int y = 160; y < 704; y++)
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

void drawdigit(int digit, Pixel *pix, Pixel *screen, int xco, int yco)
{
	int indexa, indexb;
	indexa = 0;
	short * digpoi = NULL; //Each image struct contains an array of characters that represents the colors of various pixels. We can cast that array as shorts and point digpoi at its base address
	if (digit >= 0 || digit <= 9) //Check that digit is in the range [0, 9]
	{
		if (digit == 0) digpoi = (short *)digit0.pixel_data;
		else if (digit == 1) digpoi = (short *)digit1.pixel_data;
		else if (digit == 2) digpoi = (short *)digit2.pixel_data;
		else if (digit == 3) digpoi = (short *)digit3.pixel_data;
		else if (digit == 4) digpoi = (short *)digit4.pixel_data;
		else if (digit == 5) digpoi = (short *)digit5.pixel_data;
		else if (digit == 6) digpoi = (short *)digit6.pixel_data;
		else if (digit == 7) digpoi = (short *)digit7.pixel_data;
		else if (digit == 8) digpoi = (short *)digit8.pixel_data;
		else if (digit == 9) digpoi = (short *)digit9.pixel_data;
		
		if (digpoi != NULL)
		{
			for (int y = yco; y < (yco + 16); y++)
			{
				for (int x = xco; x < (xco + 16); x++)
				{
					pix->color = digpoi[indexa];
					indexa++;
					pix->x = x;
					pix->y = y;
					indexb = (y * 1000) + x;
					screen[indexb] = *pix;
				}
			}
		}
	}
}

void drawscore(int score, Pixel *pix, Pixel *screen, int xco, int yco)
{
	int dig;
	int x = xco;
	int tempscore = score;
	dig = tempscore / 10000000;
	drawdigit(dig, pix, screen, x, yco);
	tempscore -= (dig * 10000000);
	x += 16;
	dig = tempscore / 1000000;
	drawdigit(dig, pix, screen, x, yco);
	tempscore -= (dig * 1000000);
	x += 16;
	dig = tempscore / 100000;
	drawdigit(dig, pix, screen, x, yco);
	tempscore -= (dig * 100000);
	x += 16;
	dig = tempscore / 10000;
	drawdigit(dig, pix, screen, x, yco);
	tempscore -= (dig * 10000);
	x += 16;
	dig = tempscore / 1000;
	drawdigit(dig, pix, screen, x, yco);
	tempscore -= (dig * 1000);
	x += 16;
	dig = tempscore /100;
	drawdigit(dig, pix, screen, x, yco);
	tempscore -= (dig * 100);
	x += 16;
	dig = tempscore / 10;
	drawdigit(dig, pix, screen, x, yco);
	tempscore -= (dig * 10);
	x += 16;
	dig = tempscore;
	drawdigit(dig, pix, screen, x, yco);
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

int checkwater(Player *play, int watstart, int watend)
{
	//This method will check if the player falls in the water. watstart is the y coordinate where the water starts, watend is where it ends.
	//This method simply checks if the player is in the range. If they are, then the refreshscreen method will check if they're sitting on a log.
	if (play->posy >= watstart && play->posy <= watend) { return 1; }
	else { return 0; }
	//That's a pretty simple one!
}

int checklog(Player *play, Log *logpoi, int lognum)
{
	//This method will check if the player is sitting on a log. logpoi is the base address for an array of log structures. lognum is the number of logs in the array.
	for (int i = 0; i < lognum; i++)
	{
		if (play->posy == (logpoi + i)->posy) //Check if the player is within a certain vertical range from the log
		{
			if ((play->posx + 31) > (logpoi + i)->posx && play->posx < ((logpoi + i)->posx + ((logpoi + i)->width - 1)))
			{
				//The above if statement checks if the player is within a certain horizontal range of the log
				play->posx += (logpoi + i)->speed; //Moves the player along with the log
				return 1;
			}
		}
	}
	play->posx = 0; //If the player isn't on a log, return them to the top-left corner
	play->posy = 0;
	return 0;
}

void movelog(Pixel *pix, Pixel *screen, Log *logpoi)
{
	logpoi->posx += logpoi->speed;
	if (logpoi->posx > 1000 && logpoi->speed > 0) logpoi->posx = -(logpoi->width);
	if (logpoi->posx < -(logpoi->width) && logpoi->speed < 0) logpoi->posx = 1000 + logpoi->width;
	int x = logpoi->posx;
	int y = logpoi->posy;
	int index;
	int xend = x + (logpoi->width - 1);
	int yend = y + 31;
	for (y; y < yend; y++)
	{
		for (x = logpoi->posx; x < xend; x++)
		{
			if (x >= 0 && x <= 1000)
			{
				pix->color = 0xF800; //This should be brown;
				pix->x = x;
				pix->y = y;
				index = (y * 1000) + x;
				screen[index] = *pix;
			}
		}
	}
}

int refreshscreen(Player *play, Pixel *pix, Pixel *screen, Car *carpoi, int carnum, Log *logpoi, int lognum)
{
	drawBack(pix, screen);
	for (int i = 0; i < lognum; i++) movelog(pix, screen, (logpoi + i));
	drawscore(69420, pix, screen, 0, 0);
	drawPlayer(play, pix, screen);
	for (int i = 0; i < carnum; i++) movecar(pix, screen, (carpoi + i));
	for (int i = 0; i < 704000; i++) drawPixel((screen + i));
	if (checkcollision(play, carpoi, carnum) == 1) return 1; //We need to return some value if the player's hit by a car to interrupt the moveplayer method
}

void moveplayer(Player *play, Pixel *pix, unsigned short bitfield, Pixel *screen, Car *carpoi, int carnum, Log *logpoi, int lognum) 
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
				if (refreshscreen(play, pix, screen, carpoi, carnum, logpoi, lognum) == 1) break;
				Wait(20833);
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
				if (refreshscreen(play, pix, screen, carpoi, carnum, logpoi, lognum) == 1) break;
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
				if (refreshscreen(play, pix, screen, carpoi, carnum, logpoi, lognum) == 1) break;
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
				if (refreshscreen(play, pix, screen, carpoi, carnum, logpoi, lognum) == 1) break;
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
	Log *logarr; 
	logarr = (Log *)malloc(16 * sizeof(Log));
	int pos = 0;
	int i;
	for (i = 0; i < 4; i++)
	{
		npo[i].width = 48;		//128
		npo[i].posy = 192;		// 64
		npo[i].posx = pos;		//192
		pos += 256;
		npo[i].speed = 4;
	}
	pos = 1000;
	for (i = 4; i < 8; i++)
	{
		npo[i].width = 48;
		npo[i].posy = 224;
		npo[i].posx = pos;
		pos -= 256;
		npo[i].speed = -4;
	}
	pos = 0;
	for (i = 0; i < 4; i++)
	{
		logarr[i].width = 48;
		logarr[i].posy = 32;
		logarr[i].posx = pos;
		pos += 256;
		logarr[i].speed = 4;
	}
	pos = 1000;
	for (i = 4; i < 8; i++)
	{
		logarr[i].width = 48;
		logarr[i].posy = 64;
		logarr[i].posx = pos;
		pos -= 256;
		logarr[i].speed = -4;
	}
	pos = 0;
	for (i = 8; i < 12; i++)
	{
		logarr[i].width = 48;
		logarr[i].posy = 96;
		logarr[i].posx = pos;
		pos += 256;
		logarr[i].speed = 4;
	}
	pos = 1000;
	for (i = 12; i < 16; i++)
	{
		logarr[i].width = 48;
		logarr[i].posy = 128;
		logarr[i].posx = pos;
		pos -= 256;
		logarr[i].speed = -4;
	}
	
	play->posy = 32;
	play->posx = 32;
	
	unsigned short button = NOBUTT;
	
	refreshscreen(play, pix, screen, npo, 8, logarr, 16);
	
	int looptrue = 1;
	while (looptrue)
	{
		button = Read_SNES(gpio);
		moveplayer(play, pix, button, screen, npo, 8, logarr, 16);
		if (checkwater(play, 32, 159) == 1) checklog(play, logarr, 16); //We don't want to check if the player's on a log until they have completeted a "leap"
		refreshscreen(play, pix, screen, npo, 8, logarr, 16);
		Wait(20833);
		if ((button & START_B) == 0) looptrue = 0;
	}
	
	free(pix);
	free(play);
	free(screen);
	free(npo);
	free(logarr);
	
	return 0;
}
