#include <at89c5131.h>
#include <stdio.h>
#include <lcd.h> //Header file with LCD interfacing functions
#include "serial.c" //C file with UART interfacing functions

unsigned char LFSR = 0x0F; //initializing lower nibble of LFSR to (1 ,1 ,1 ,1)
unsigned char current_loc = 0x8E; //variable maintains tile position and every tile starts from this position on the LCD
unsigned char up_filled = 0x7F ;//variable maintains the location of last filled location in top line
unsigned char down_filled = 0xBF ;//variable maintains the location of last filled location in bottom line
unsigned char tilenum = 0;// can take 0,1 or 2 according to whether it is 1 starred , 2 starred or 3 starred respectively
unsigned char tilestate=1;//for orientation of tiles (1 for default)
unsigned char survived_time = 0;// to keep track of score
unsigned char max_time = 0;// highest score
unsigned char timer_count = 0;
unsigned char query = 0;
bit one_sec_flag=0; // set when 1 sec delay is over
bit blocked_flag=0; //set when the current tile reaches its end path

void printer(unsigned char loc ,unsigned char *s){
	lcd_cmd(loc);
	msdelay(5);
	lcd_write_string(s);
	
}

/* lcd_movement() carries out the tile movement after every second*/
void lcd_movement(void){
	switch(tilenum)
	{
		case 0:
			if(tilestate == 1) printer(current_loc,"* ");
			if(tilestate == 2) printer(current_loc + 0x40,"* ");
			break;
		case 1:
			if(tilestate == 1) printer(current_loc,"** ");
			if(tilestate == 2) printer(current_loc + 0x40,"** ");
			if(tilestate == 3) {printer(current_loc+1,"* ");
													printer(current_loc + 0x41,"* ");}
			break;
		case 2:
			if(tilestate == 1) {printer(current_loc,"* ");
													printer(current_loc+0x40,"** ");}
			if(tilestate == 2) {printer(current_loc,"** ");
													printer(current_loc+0x40,"* ");}
			if(tilestate == 3) {printer(current_loc,"** ");
													printer(current_loc+0x40," * ");}
			if(tilestate == 4) {printer(current_loc," * ");
													printer(current_loc+0x40,"** ");}
			break;
	}
}
/* blocked() checks if the moving tile has reached the end and sets the blocked_flag accordingly */
bit blocked(void){
		if(tilenum == 0){
			if(tilestate == 1 && current_loc - up_filled <= 1){
				up_filled = up_filled + 1;
				return 1;
			}
			if(tilestate == 2 && current_loc + 0x40 - down_filled <=1){
				down_filled = down_filled+1;
				return 1;
			}
		}
		
		if(tilenum == 1){
			if(tilestate == 1 && current_loc - up_filled <= 1){
				up_filled = up_filled + 2;
				return 1;
			}
			if(tilestate == 2 && current_loc +0x40 - down_filled <=1){
				down_filled = down_filled+2;
				return 1;
			}
			if(tilestate == 3 && current_loc - up_filled <=1){
				up_filled += 1;
				down_filled = up_filled + 0x40;
				return 1;
			}
			if(tilestate == 3 && current_loc +0x40 - down_filled <=1){
				down_filled = down_filled + 1;
				up_filled = down_filled - 0x40;
				return 1;
			}
		}
		if(tilenum == 2){
			if(tilestate == 1 && current_loc - up_filled <=1){
				up_filled += 1;
				down_filled = up_filled + 0x40 + 1;
				return 1;
			}
			if(tilestate == 1 && current_loc +0x40 - down_filled <=1){
				down_filled = down_filled + 2;
				up_filled = down_filled - 0x40 - 1;
				return 1;
			}
			if(tilestate == 2 && current_loc - up_filled <=1){
				up_filled += 2;
				down_filled = up_filled + 0x40 -1;
				return 1;
			}
			if(tilestate == 2 && current_loc +0x40 - down_filled <=1){
				down_filled = down_filled + 1;
				up_filled = down_filled - 0x40 + 1;
				return 1;
			}
			if(tilestate == 3 && current_loc - up_filled <= 1){
				up_filled += 2;
				down_filled = up_filled + 0x40;
				return 1;			
			}
			if(tilestate == 3 && current_loc + 0x40 - down_filled <= 1){
				down_filled += 1;
				down_filled = up_filled + 0x40;
				return 1;
			}
			if(tilestate == 4 && current_loc - up_filled <= 1){
				up_filled += 1;
				down_filled = up_filled + 0x40;
				return 1;			
			}
			if(tilestate == 4 && current_loc + 0x40 - down_filled <= 1){
				down_filled += 2;
				down_filled = up_filled + 0x40;
				return 1;
			}
			
			
			
		}
		return 0;
}


/*timer_1s() is used to keep track of 1 second. Done with timer 0 and its ISR */
void timer_1s(void){
	TMOD |= 0x01; //to set timer 0 in mode 1 and not to disturb timer 1(used by UART)
	TH0 = 0x3C;
	TL0 = 0xB0;
	EA =1;
	ET0=1;
	TR0=1;
}

void t0_interrupt(void) interrupt 1
{
	TH0 = 0x3C;
	TL0 = 0xB0;
	timer_count += 1;
	if(timer_count == 40){
		one_sec_flag = 1;
		timer_count = 0;
	}
}

/* tile_generator() is used to generate a new tile once the old tile ends */
void tile_generator(void){
	
	tilenum = LFSR%3;
	tilestate=1;
	
	
	switch(tilenum)
	{	
		case 0:
			printer(0x8E,"*");
			break;
		
		case 1:
			printer(0x8E,"**");
			break;
		
		case 2:
			printer(0x8E,"*");
			printer(0xCE,"**");
			break;
		}
	
}
	


/* LFSR_next_state() updates LFSR */
void LFSR_next_state(void){
	
	unsigned char nextstate = LFSR; //initialization
	
	nextstate = (LFSR>>1) + (LFSR&8) ; // makes nextstate =  b3,b3,b2,b1
	nextstate = nextstate ^ ((LFSR&1)<<3) ;// makes nextstate = b3^b0,b3,b2,b1 as required
	LFSR = nextstate;
	
}

/* up() moves possible tiles up */
void up(void){
	if(tilenum == 0 && tilestate == 2 && current_loc >up_filled){
		printer(current_loc+0x40," ");
		printer(current_loc,"*");
		tilestate = 1;
	}
	if(tilenum == 1 && tilestate == 2 && current_loc > up_filled){
		printer(current_loc+0x40,"  ");
		printer(current_loc,"**");
		tilestate = 1;
	}
}	

/*down() moves possible tiles down */
void down(void){
	if(tilenum == 0 && tilestate == 1 && current_loc + 0x40 >down_filled){
		printer(current_loc+0x40,"*");
		printer(current_loc," ");
		tilestate = 2;
	}
	if(tilenum == 1 && tilestate == 1 && current_loc + 0x40 > down_filled){
		printer(current_loc+0x40,"**");
		printer(current_loc,"  ");
		tilestate = 2;
	}
}	
	
void rotate(void){
	
	if(tilenum == 1){
		if(tilestate == 1 || tilestate == 2){
			printer(current_loc,"* ");
			printer(current_loc + 0x40,"* ");
			tilestate = 3;
			return;
		}
		if(tilestate == 3){
			printer(current_loc,"**");
			printer(current_loc+0x40,"  ");
			tilestate = 1;
			return;
		}
	}
	
	if(tilenum == 2){
		if (tilestate ==1){
			printer(current_loc ,"**");
			printer(current_loc +0x40,"* ");
			tilestate = 2;
			return;
		}
		if (tilestate == 2){
			printer(current_loc +0x40," *");
			tilestate = 3;
			return;
		}
		if (tilestate ==3 && current_loc + 0x40 > down_filled){
			printer(current_loc ," *");
			printer(current_loc +0x40,"**");
			tilestate = 4;
			return;
		}
		if (tilestate == 4 && current_loc >up_filled){
			printer(current_loc ,"* ");
			tilestate = 1;
			return;
		}
	}
}	
	
void score(void){
	unsigned char score[16] = {0} ;
	unsigned char highscore[16] = {0};
	ET0=0;
	
	lcd_cmd(0x01);//clears lcd
	lcd_cmd(0x80);
	sprintf(score,"Score:       %d",survived_time);		
		lcd_write_string(score);
	
	if(survived_time > max_time){
		max_time = survived_time;
	}
	lcd_cmd(0xC0);	
		sprintf(highscore,"High Score:  %d",max_time);		
		lcd_write_string(highscore);
	msdelay(3000);
	lcd_cmd(0x01);
	ET0=1;
}	

void main(void){
	lcd_init();
	uart_init();

	while(1)
	{
		survived_time=0;//setting initial score to 0
		LFSR_next_state();//to get next state
		timer_1s(); //starts the 1second timer
		tile_generator();//tile generated according to updated LFSR
		while(1){
			if(rx_complete == 1){
				rx_complete = 0;
				query = SBUF;
				switch(query){
					case 'q' : up();break;
					case 'a' : down();break;
					case 'r' : rotate();break;
					default : break;
				}	
			}
			if(one_sec_flag ==1){
				one_sec_flag = 0;
				survived_time += 1;
				
				if(blocked() == 0){
					current_loc -= 1;
					lcd_movement();
				}
				else {
					
					if(up_filled < 0x8E && down_filled < 0xCE){
						current_loc = 0x8E;
						LFSR_next_state();
						tile_generator();
					}
					else {
						score();
						up_filled = 0x7F;
						down_filled = 0xBF;
						break;
					}
				}
			}
		}
	}
}
