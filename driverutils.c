//Program created by Reid Edward Erb and Ethan Park for CPSC 359 at the University of Calgary, winter semester, 2021

#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "snesdriver.h"

unsigned int * getGPIOPtr(void)
{
	/* The code for this method is basically identical to the method with the same name that was provided in the project folder.
	It will find the address of the GPIO and allow the program to interact with it through a memory map. */
	
	int fdgpio = open("/dev/gpiomem", O_RDWR); //The variable fdgpio will be given the file descriptor
	
	if (fdgpio < 0)
	{
		printf("Unable to open");
	}
	
	unsigned int *gpioPtr = (unsigned int *)mmap(0,4096,PROT_READ|PROT_WRITE,MAP_SHARED,fdgpio,0);
	
	return gpioPtr;
}

void Init_GPIO(unsigned int *gpio)
{
	/*The purpose of this method is to prepare the core to give output to the SNES controller's latch and clock lines, and for the core to
	receive input from the controller's data line. The numbers representing these GPIO lines were defined as constants.*/
	
	INP_GPIO(LAT); 	//Here, we are clearing the bits on the latch-line pin
	OUT_GPIO(LAT); 	//Then we put a one in the latch-line pin's bits, so it's ready for output
	
	INP_GPIO(CLK);	//Clearing the clock-line pin's bits
	OUT_GPIO(CLK);	//Preparing clock-line for output
	
	INP_GPIO(DAT);	//Preparing data-line for input.
}

void Write_Latch(unsigned int *gpio, int wval)
{
	/* This method will write the value in wval to the latch line. wval should either equal 0 or 1 */
	
	if (wval == 0)
	{
		*(gpio + GPCLR0) = 1 << LAT;
	}
	else if (wval == 1)
	{
		*(gpio + GPSET0) = 1 << LAT;
	}	
}

void Write_Clock(unsigned int *gpio, int wval)
{
	//This function is very similar to Write_Latch, except we'll be writing values to the clock line instead of the latch
		
	if (wval == 0)
	{
		*(gpio + GPCLR0) = 1 << CLK;
	}
	else if (wval == 1)
	{
		*(gpio + GPSET0) = 1 << CLK;
	}
}

int Read_Data(unsigned int *gpio)
{
	//This method will read whatever bit value is received from the data line.
	
	int v = (gpio[GPLEV0] >> DAT) & 1;
	return v;
}	

void Wait(unsigned int val)
{
	//When this function is called, the program will wait for val microseconds
	usleep(val); //The usleep function will sleep for val microseconds.
	/* It seems like we could have just used usleep instead of making the "Wait" method, but since the rubric told me to make
	a "Wait" method, I made one. Whatever. */
}	

unsigned short Read_SNES(unsigned int *gpio)
{
	//This method will read then data line 16 times and return the resulting 16 bits as a short integer
	unsigned short buttbit = 0;
	int i, v;	//i is a counter variable for the loop. v will be the value of the latest bit read from the data line
	Write_Latch(gpio, 1);
	Write_Clock(gpio, 1);
	Wait(LATCH_W);
	Write_Latch(gpio, 0);
	for (i = 1; i <= 16; i++)
	{
		Wait(CLOCK_W);
		Write_Clock(gpio, 0);
		Wait(CLOCK_W);
		v = Read_Data(gpio);
		buttbit |= v;	//We set v as the least significant bit in buttbit
		if (i < 16) buttbit = buttbit << 1;	//We shift buttbit's bits one to the left on every iteration but the last one.
		Write_Clock(gpio, 1);
	}
	Write_Clock(gpio, 0);
	return buttbit;
}
