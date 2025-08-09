#ifndef GPRS_LIB_H
#define GPRS_LIB_H

#include "stm32f4xx.h"  // Inclure les fichiers d'en-t�te de la biblioth�que STM32F4
#define SIM808_OK        0  // Op�ration r�ussie
#define SIM808_ERROR    -1


// D�finir des variables globales si n�cessaire
extern USART_TypeDef* GPRS_USART;  // Pointeur vers l'USART pour la communication
extern UART_HandleTypeDef huart1;
extern char response[1024];
#define SIM808_RX_BUFFER_SIZE 1024
#define SIM808_TX_BUFFER_SIZE 512

extern char sim808_rx_buffer[SIM808_RX_BUFFER_SIZE];
extern char sim808_tx_buffer[SIM808_TX_BUFFER_SIZE];

// Fonctions
int SIM808_Init(UART_HandleTypeDef *huart);
void GPRS_Init(UART_HandleTypeDef *huart);
int GPRS_TestCommunication(UART_HandleTypeDef *huart);
void GPRS_SendCommand(UART_HandleTypeDef *huart, const char *command);
int GPRS_HTTP_Get(UART_HandleTypeDef *huart, const char *url, char *response);
int GPRS_HTTP_Post(UART_HandleTypeDef *huart, const char *url, const char *data, char *response);




#endif // GPRS_LIB_H
