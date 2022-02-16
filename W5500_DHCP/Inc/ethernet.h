/**
 * @file ethernet.h
 * @author Flynn Harrison
 * @brief Setup and basic controll of the W5500 NIC through the ioLibary_Driver proviced by wiznet
 * @version 0.1
 * @date 2022-02-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef ETHERNET_H
#define ETHERNET_H

#include "gpio.h"
#include "spi.h"

/* Uncomment to enable debug msg */
//#define _ETHERNET_DEBUG_

#define MAC_ADDRESS {0x00, 0x08, 0xdc,0x00, 0xab, 0xcd} 
#define SPI_BURST   TRUE

/**
 * @brief Sets up the interface and callbacks for the W5500
 * 
 * @param hspi Handle for the spi periphery that is connected to the chip 
 * @param GPIOx_CS Chip select port
 * @param GPIO_CS Chip select pin
 * @return int 1 on success and 0 on failure 
 */
int ethernet_init(SPI_HandleTypeDef* hspi,GPIO_TypeDef* GPIOx_CS, uint16_t GPIO_CS, void* socketBufSize);

/**
 * @brief Obtain ip, subnet, ect from DHCP
 * 
 */
void ethernet_dhcpLease();

#endif