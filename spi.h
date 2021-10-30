/*
 * spi.h
 *
 * Created: 15/10/2021 02:05:18 p. m.
 *  Author: Luis Alejandro
 */ 
#ifndef SPI_H_
#define SPI_H_
#include "sam.h"
#define BAUDRATE 9600

void spiInit( void );
uint8_t spiSend( uint8_t data );

#endif /* SPI_H_ */