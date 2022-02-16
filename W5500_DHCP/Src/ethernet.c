/**
 * @file ethernet.c
 * @author Flynn Harrison
 * @brief 
 * @version 0.1
 * @date 2022-02-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "ethernet.h"
#include "wizchip_conf.h"
#include "dhcp.h"
#include "stdio.h"
#include "string.h"

// Private defines
#define SOCK_DHCP 		0
#define DATA_BUF_SIZE 2048			// See if this can be reduced 

#define SPI_TIMEOUT 0xFFFF

// Private variables
static GPIO_TypeDef* 			g_GPIOx_CS;
static uint16_t 					g_GPIO_CS;
static SPI_HandleTypeDef* g_spi;

static uint8_t g_dhcpMsg[DATA_BUF_SIZE];

// Prototypes 
void cs_sel(void);
void cs_desel(void);
void spi_wb(uint8_t buf);
uint8_t spi_rb(void);
void spi_wb_burst(uint8_t* buf, uint16_t len);
void spi_rb_burst(uint8_t* buf, uint16_t len);

void ip_assign(void);
void ip_conflict(void);

void network_init(wiz_NetInfo* netInfo);

int ethernet_init(SPI_HandleTypeDef* hspi,GPIO_TypeDef* GPIOx_CS, uint16_t GPIO_CS, void* socketBufSize)
{
	uint8_t tmp;
	uint8_t mac[6] = MAC_ADDRESS;

	// Attach the handles to the CB functions (Yes this is cursed)
	g_GPIOx_CS 	= GPIOx_CS;
	g_GPIO_CS 	= GPIO_CS;
	g_spi 			= hspi;

	// Register the call back functions with the ioLibary
	reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
	reg_wizchip_spi_cbfunc(spi_rb, spi_wb);
  //reg_wizchip_spiburst_cbfunc(spi_rb_burst, spi_wb_burst);

#if SPI_BURST
  //reg_wizchip_spiburst_cbfunc(spi_rb_burst, spi_wb_burst);
#endif

	// wizchip init
	if(ctlwizchip(CW_INIT_WIZCHIP, socketBufSize) == -1)
	{
#ifdef _ETHERNET_DEBUG_
		printf("ETHERNET: W5500 init failed!\r\n");
#endif
		while(1); // Find better fail state
	}

	do
	{
		if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)
		{
#ifdef _ETHERNET_DEBUG_
			printf("ETHERNET: Unknown PHY link status!\r\n");
#endif
		}
	} while (tmp == PHY_LINK_OFF);

	// Set source MAC address
	setSHAR(mac);

	return 1;
}

void ethernet_dhcpLease()
{
	uint8_t DHCPstate;

	// Register DHCP function callbacks 	
	reg_dhcp_cbfunc(ip_assign, ip_assign, ip_conflict);

	// Open socket to obtain ip, mask, gateway, dns ect
	DHCP_init(SOCK_DHCP, g_dhcpMsg);

	// Try and obtain info from DHCP
	do
	{
		DHCPstate = DHCP_run();
		switch(DHCPstate)
		{
		case DHCP_IP_ASSIGN:
#ifdef _ETHERNET_DEBUG_
			printf("ETHERNET: DHCP_IP_ASSIGN\r\n");
#endif
			break;
		
		case DHCP_IP_LEASED:
#ifdef _ETHERNET_DEBUG_
			printf("ETHERNET: DHCP_IP_LEASED\r\n");
#endif
			break;
		
		case DHCP_FAILED:
#ifdef _ETHERNET_DEBUG_
			printf("ETHERNET: DHCP_FAILED\r\n");
#endif
			break;

		default:
			break;
		}

	} while (DHCPstate != DHCP_IP_LEASED);
}

/********************
 * Private functions*
 ********************/
void cs_sel(void)
{
	HAL_GPIO_WritePin(g_GPIOx_CS, g_GPIO_CS, GPIO_PIN_RESET);
}

void cs_desel(void)
{
	HAL_GPIO_WritePin(g_GPIOx_CS, g_GPIO_CS, GPIO_PIN_SET);
}

uint8_t spi_rb(void)
{
	uint8_t rbuf;
	HAL_SPI_Receive(g_spi, &rbuf, sizeof(rbuf), SPI_TIMEOUT);
	return rbuf;
}

void spi_wb(uint8_t buf)
{
	HAL_SPI_Transmit(g_spi, &buf, sizeof(buf), SPI_TIMEOUT);
}

//#if SPI_BURST
void spi_wb_burst(uint8_t* buf, uint16_t len)
{
  // Check if last SPI tranfer has finished
  //printf("write: %d %d %d\r\n", buf[0], buf[1], buf[2]);
	while(HAL_SPI_GetState(g_spi) != HAL_SPI_STATE_READY){};
	HAL_SPI_Transmit_DMA(g_spi, buf, len);
  
}

void spi_rb_burst(uint8_t* buf, uint16_t len)
{
  memset((void*)buf, 0, (size_t)len);
  HAL_SPI_Receive_DMA(g_spi, buf, len);
  while(HAL_SPI_GetState(g_spi) != HAL_SPI_STATE_READY){};
  
}
//#endif

void network_init(wiz_NetInfo* netInfo)
{
	// Set network info
	ctlnetwork(CN_SET_NETINFO, (void*)netInfo);

#ifdef _ETHERNET_DEBUG_
	uint8_t tmpstr[6] = {0,};
	// Get Network info
	ctlnetwork(CN_GET_NETINFO, (void*)netInfo);

	// Display info
	ctlwizchip(CW_GET_ID,(void*)tmpstr);
	if (netInfo->dhcp == NETINFO_DHCP)
	{
		printf("\r\n= %s NET CONF : DHCP =\r\n", (char*)tmpstr);
	} 
	else
	{
		printf("\r\n= %s NET CONF : Static =\r\n", (char*)tmpstr);
	}

	printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",netInfo->mac[0],netInfo->mac[1],netInfo->mac[2],netInfo->mac[3],netInfo->mac[4],netInfo->mac[5]);
	printf("SIP: %d.%d.%d.%d\r\n", netInfo->ip[0],netInfo->ip[1],netInfo->ip[2],netInfo->ip[3]);
	printf("GAR: %d.%d.%d.%d\r\n", netInfo->gw[0],netInfo->gw[1],netInfo->gw[2],netInfo->gw[3]);
	printf("SUB: %d.%d.%d.%d\r\n", netInfo->sn[0],netInfo->sn[1],netInfo->sn[2],netInfo->sn[3]);
	printf("DNS: %d.%d.%d.%d\r\n", netInfo->dns[0],netInfo->dns[1],netInfo->dns[2],netInfo->dns[3]);
	printf("===========================\r\n");
#endif
}

void ip_assign(void)
{
	wiz_NetInfo netInfo = { .mac = MAC_ADDRESS,			// TODO define each of the parameters of the stuct
													.ip = {192, 168, 1, 123},
													.sn = {255,255,255,0},
													.gw = {192, 168, 1, 1},
													.dns = {0,0,0,0},
													.dhcp = NETINFO_DHCP };

	getIPfromDHCP(netInfo.ip);
	getGWfromDHCP(netInfo.gw);
	getSNfromDHCP(netInfo.sn);
	getDNSfromDHCP(netInfo.dns);
	netInfo.dhcp = NETINFO_DHCP;

	// Apply DHCP info
	network_init(&netInfo);

}

void ip_conflict(void)
{
	printf("NETWORK: CONFLICT IP from DHCP\r\n");

	while(1);
}