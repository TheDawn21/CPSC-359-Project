#define LAT 9
#define DAT 10
#define CLK 11

#define GPSET0 7
#define GPCLR0 10
#define GPLEV0 13

#define INP_GPIO(g) *(gpio + ((g)/10)) &= ~(7<<(((g) % 10) * 3))
#define OUT_GPIO(g) *(gpio + ((g)/10)) |= (1 <<(((g) % 10) * 3))

#define LATCH_W 12	//We wait 12 microseconds after writing to the latch line
#define CLOCK_W 6	//We wait 6 microseconds after writing to the clock line

#define SPLITSEC 500000	//500000 microseconds is equal to half a second
#define NOBUTT 65535	//65535 == 1111 1111 1111 1111 in binary. If no buttons are pressed on the SNES pad, this is what Read_SNES() will return
#define B_BUTT 32768 	//32768 == 1000 0000 0000 0000 in binary. In the short value returned by Read_SNES(), this bit corresponds to the B button
#define Y_BUTT 16384	//16384 == 0100 0000 0000 0000 in binary (the bit corresponding to the Y button)
#define SELECT_B 8192	// 8192 == 0010 0000 0000 0000 (the select button)
#define START_B 4096	// 4096 == 0001 0000 0000 0000 (the start button)
#define U_DIR 2048		// 2048 == 0000 1000 0000 0000 (up on the D-pad)
#define D_DIR 1024		// 1024 == 0000 0100 0000 0000 (down on the D-pad)
#define L_DIR 512		//  512 == 0000 0010 0000 0000 (left on the D-pad)
#define R_DIR 256		//  256 == 0000 0001 0000 0000 (right on the D-pad)
#define A_BUTT 128		//  128 == 0000 0000 1000 0000 (the A button)
#define X_BUTT 64		//   64 == 0000 0000 0100 0000 (The X button)
#define L_BUTT 32		//   32 == 0000 0000 0010 0000 (The left shoulder button)
#define R_BUTT 16		//   16 == 0000 0000 0001 0000 (The right shoulder button)

unsigned int * getGPIOPtr(void);
void Init_GPIO(unsigned int *gpio);
void Write_Latch(unsigned int *gpio, int wval);
void Write_Clock(unsigned int *gpio, int wval);
int Read_Data(unsigned int *gpio);
void Wait(unsigned int val);
unsigned short Read_SNES(unsigned int *gpio);
