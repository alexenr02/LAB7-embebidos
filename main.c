/*
 * LAB07EMB.c
 *
 * Created: 15/10/2021 01:40:43 p. m.
 * Author : Luis Alejandro
 */ 


#include "sam.h"
#include "spi.h"
#include "stdbool.h"
#include "stdint.h."
#include "my_samd21g18a.h"
#include "myprintf.h"

/*
int main(void)
{
	SystemInit();
	spiInit();
	
	volatile uint8_t rData;
	volatile uint8_t sData = 85;
	while (1) {
		REG_PORT_OUTCLR0 = PORT_PA18; //initiate transaction by SS_low
		rData = spiSend( sData );
		REG_PORT_OUTSET0 = PORT_PA18; //finish transaction by SS_high
	}
}
*/

#define RXBUFSIZE 0x400
#define LENGTH_R1 0x03
#define LENGTH_R7 0x07

void initCycles(void);

#define SIZE_SD_CMD 0x06
#define kCMD00 0x40
#define kCMD08 0x48
#define kCMD55 0x77
#define kCMD41 0x69
#define kCMD58 0x7A

const uint8_t CMD00[SIZE_SD_CMD]  ={0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
const uint8_t CMD08[SIZE_SD_CMD]  ={0x48, 0x00, 0x00, 0x01, 0xAA, 0x87}; // 0100 1000    0000 0000    0000 0000     0000 0001    1010 1010    0000 1111 1000 0111
uint8_t CMD17[SIZE_SD_CMD]  ={0x51, 0x00, 0x00, 0x00, 0x00, 0x01}; //command for a single block read is CMD17 - READ_SINGLE_BLOCK.
uint8_t CMD172[SIZE_SD_CMD]  ={0x51, 0x00, 0x00, 0x08, 0x00, 0x01};
const uint8_t CMD18[SIZE_SD_CMD]  ={0x52, 0x00, 0x00, 0x00, 0x00, 0x01};
const uint8_t CMD55[SIZE_SD_CMD]  ={0x77, 0x00, 0x00, 0x00, 0x00, 0x65};
const uint8_t CMD41[SIZE_SD_CMD] = {0x69, 0x40, 0x00, 0x00, 0x00, 0x77};
	
const uint8_t CMD58[SIZE_SD_CMD] = {0x7A, 0x00, 0x00, 0x00, 0x00, 0x75};

uint8_t RxBuffer[RXBUFSIZE];

//com6 USB serial port
//com5 arduino EDGB virtual

void rcvr_datablock(const uint8_t * send_buff, uint32_t lba, uint8_t * receive_buff, uint32_t bs ) {
	uint8_t temp = 0xFF;
	uint32_t i;
	
	REG_PORT_OUTCLR0 = PORT_PA18;
	myprintf("\n\n");

	temp = send_buff[0]; // CMD17 command for a single block read is CMD17 - READ_SINGLE_BLOCK.
	temp = spiSend(temp); //SEND CMD17
	myprintf(" %x", temp); //PRINT ANSWER
	
	temp = ((uint8_t*)&lba)[3]; 
	temp = spiSend(temp);
	myprintf(" %x", temp);
	
	temp = ((uint8_t*)&lba)[2];
	temp = spiSend(temp);
	myprintf(" %x", temp);

	temp = ((uint8_t*)&lba)[1];
	temp = spiSend(temp);
	myprintf(" %x", temp);
	
	temp = ((uint8_t*)&lba)[0];
	temp = spiSend(temp);
	myprintf(" %x", temp);

	temp = send_buff[5];
	temp = spiSend(temp);
	myprintf(" %x", temp);

	// Reading to find the beginning of the sector

	temp = spiSend(0xFF);
	while(temp != 0xFE){
		temp = spiSend(0xFF);
		myprintf(" %x", temp);
	}
	
	// Receiving the memory sector/block
	
	myprintf("\n\n");
	for(i=0; i< bs; i++) {
		while(SERCOM1->SPI.INTFLAG.bit.DRE == 0);
		SERCOM1->SPI.DATA.reg = 0xFF;
		while(SERCOM1->SPI.INTFLAG.bit.TXC == 0);
		while(SERCOM1->SPI.INTFLAG.bit.RXC == 0);
		temp = SERCOM1->SPI.DATA.reg;
		*(receive_buff++) = temp;
		myprintf(" %x", temp);
	}
	REG_PORT_OUTSET0 = PORT_PA18;
	myprintf("\n\n");
}

uint32_t spiXchg(const uint8_t * send_buff, uint32_t bc, uint8_t * receive_buff ) {
	uint8_t temp = 0xFF;
	uint32_t i;
	uint8_t temp_cmd = send_buff[0];
	
	REG_PORT_OUTCLR0 = PORT_PA18;
	myprintf("\n\nValor recibido cuando enviamos dato deseado:\n\n");
	for(i=0; i< bc; i++) {
		temp = spiSend(*(send_buff++));
	
		//myprintf("%x ",(send_buff[i]));
		myprintf("%x ",temp);
	}
	//myprintf("\n\nValor recibido: %x\n\n",temp);
	switch(temp_cmd) {
		myprintf("\n\nCommand sent:\n");
		case kCMD00 :
		myprintf("\n\nCMD00:\n");
		myprintf("Valor recibido cuando esperamos al menos 3 ciclos enviando 0xff para esperar respuesta:\n\n");
		for(i=0; i<LENGTH_R1; i++) {
			temp = spiSend(0xFF); // en temp está el receive_buff
			receive_buff[i]=temp;
			myprintf("%x ", temp);
		}
		break;
		case kCMD08 :
		myprintf("\n\nCMD08:\n");
		myprintf("Valor recibido cuando esperamos al menos 7 ciclos enviando 0xff para esperar respuesta::\n\n");
		for(i=0; i<LENGTH_R7; i++) {
			temp = spiSend(0xFF);
			receive_buff[i]=temp;
			myprintf("%x ", temp);
		}
		break;
		case kCMD41 :
		myprintf("\n\nCMD41:\n");
		myprintf("Valor recibido cuando esperamos al menos 2 ciclos enviando 0xff para esperar respuesta::\n\n");
		for(i=0; i<LENGTH_R1-1; i++) {
			temp = spiSend(0xFF);
			receive_buff[i]=temp;
			myprintf(" %x", temp);
		}
		spiSend(0xFF);
		break;
		case kCMD55 :
		myprintf("\n\nCMD55:\n");
		myprintf("Valor recibido cuando esperamos al menos 3 ciclos enviando 0xff para esperar respuesta:\n\n");
		for(i=0; i<LENGTH_R1; i++) {
			temp = spiSend(0xFF);
			receive_buff[i]=temp;
			myprintf(" %x", temp);
		}
		break;
		case kCMD58 :
		myprintf("\n\nCMD58:\n");
		myprintf("Valor recibido cuando esperamos al menos 3 ciclos enviando 0xff para esperar respuesta:\n\n");
		for(i=0; i<LENGTH_R7; i++) {
			temp = spiSend(0xFF);
			receive_buff[i]=temp;
			myprintf(" %x", temp);
		}
		break;
		default :
		myprintf("\n Error in CMD");
	}
	REG_PORT_OUTSET0 = PORT_PA18;
	return(temp);
}

int main(void)
{
	bool CMD00_Bandera = 0;
	bool CMD08_Bandera = 0;
	bool CMD55_Bandera = 0;
	bool CMD41_Bandera = 0;
	bool CMD58_Bandera = 0;
	bool readyLeftBranch = 0;
	bool righBranch = 0;
	bool correctResp = 0;
	bool badFunction = 0;
	bool functionOK = 0;
	bool continuar=0;
	int initPos;
	uint32_t temp = 0xFF;
	/* Initialize the SAM system */
	SystemInit();
	UARTInit();
	spiInit();
	initCycles();
	
	myprintf("\n\nEmpiezaPrograma:\n\n");
	while(CMD00_Bandera == 0){
		spiXchg( CMD00, SIZE_SD_CMD, RxBuffer );  /* reset instruction */
		for(int i=0; i<LENGTH_R1; i++){
			if(RxBuffer[i] == 0x01){
				CMD00_Bandera = 1;
			}
		}
	}
	
	while(CMD08_Bandera == 0){
		spiXchg( CMD08, SIZE_SD_CMD, RxBuffer );  //Check voltage range.
		for(int i=0; i<LENGTH_R7; i++){
		if(RxBuffer[i]!= 0xFF){
			CMD08_Bandera = 1;
				if((RxBuffer[i] & 0x04)==0x00) // if bit 34 is clear, right branch of flowchart (version 2.00)
				{
					//righBranch = 1;	
					for(int i=0; i<LENGTH_R7; i++)
					{
						if(RxBuffer[i]==0xAA) // verify check pattern
						{
							badFunction=0;
							if(RxBuffer[i-1]==0x01) //verify voltage field
							{
								badFunction=0;
								functionOK=1;
								goto continueToCMD55;
							}else
							{
								//badFunction=1;
								myprintf("Unusable card");
								goto terminate;
							}
						}else{
							badFunction=1;
						}
					}
				}
				else{
					//leftBranch=1 (Illegal);
					while(CMD58_Bandera == 0){
					spiXchg( CMD58, SIZE_SD_CMD, RxBuffer );  //check voltage operation
					for(int i=0; i<LENGTH_R7; i++){
						if(RxBuffer[i] != 0xff){
							CMD58_Bandera = 1;
							}
						else
							{
								myprintf("Unusable card");
								goto terminate;		
							}
						}
					}
					readyLeftBranch =1;
					goto continueToCMD55;
				}
			}
		}
	}// termina while de cmd08 . Banderas utiles -> functionOK y CMD58_Bandera
	if(badFunction==1)
	{
		myprintf("Unusable card");
		goto terminate;
	}
continueToCMD55:

	while(continuar==0)
	{
		spiXchg( CMD55, SIZE_SD_CMD, RxBuffer );  //Leading command of ACMD<n> command.
		spiXchg( CMD41, SIZE_SD_CMD, RxBuffer );  //For only SDC. Initiate initialization process
		for(int i=0; i<LENGTH_R1; i++){
			if(RxBuffer[i] == 0x00){
				continuar = 1;
			}
		}
	}
	
	if(readyLeftBranch == 1) // si la version fuera mas antigua que 2.0 y se hubiese ido por el left branch
	{
		myprintf("Standard Capacity Card");
	}
	
	while(CMD58_Bandera == 0){
	spiXchg( CMD58, SIZE_SD_CMD, RxBuffer );  //check voltage operation
	for(int i=0; i<LENGTH_R7; i++){
		if(RxBuffer[i] != 0xFF){
				if(RxBuffer[4] & 0x02 == 1) //si el bit 30 (CSC) es igual a 1, es high capacity
				{
					myprintf("\nHigh Capacity Card\n");
					CMD58_Bandera =1;
					goto terminate;
				}else
				{
					myprintf("\nStandard Capacity Card\n");
					CMD58_Bandera =1;
					goto terminate;
				}
			}
		}
	}
	

	myprintf("\n");
	
	terminate:
	rcvr_datablock(CMD17, 0x00000000, RxBuffer, 0x200);
	myprintf("\nDone");
}

void initCycles(void){
	uint32_t i;
	REG_PORT_OUTSET0 = PORT_PA18;
	//myprintf("\n\n");
	uint8_t temp1=0xFF;
	uint8_t temp2=0xFF;
	myprintf("\n\nValor enviado:\n\n");
	for(i=0;i<76;i++)
	{
	myprintf("%x ",temp1);
	spiSend(0xFF);
	}
	myprintf("\n\nTermina Valor enviado\n\n");
	

}

//ADD spiXchg FUNCTION TO THE “spi.c” FILE
