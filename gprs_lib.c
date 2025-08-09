#include "gprs_lib.h"
#include "string.h"
#include <stdio.h>
#include "main.h"

// D�finir les buffers et variables pour la communication UART
static volatile uint16_t rx_index = 0;
static uint8_t rx_byte;
char sim808_rx_buffer[SIM808_RX_BUFFER_SIZE];  // D�claration du buffer global pour la r�ception UART

// Fonction d'initialisation du module SIM808
int SIM808_Init(UART_HandleTypeDef *huart) {
    char command[SIM808_TX_BUFFER_SIZE];

    // Initialiser la r�ception UART en mode interruption
    HAL_UART_Receive_IT(huart, &rx_byte, 1);

    // 1. V�rifier la communication avec le module SIM808
    snprintf(command, sizeof(command), "AT\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);  // Attendre la r�ponse

    // V�rifier si la r�ponse "OK" est re�ue
    if (strstr((char *)sim808_rx_buffer, "OK") == NULL) {
        // Erreur de communication avec SIM808
        return -1;
    }

    // 2. R�initialiser le module (si n�cessaire)
    snprintf(command, sizeof(command), "AT+CRESET\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(3000);  // Attendre le red�marrage du module

 
    // 7. Pr�parer le module pour l'usage des services HTTP
    snprintf(command, sizeof(command), "AT+HTTPINIT\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);  // Attendre l'initialisation HTTP

    // 8. V�rifier si l'initialisation HTTP a r�ussi
    snprintf(command, sizeof(command), "AT+HTTPPARA=\"CID\",1\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);


    // 9. Le module est maintenant pr�t pour envoyer des requ�tes HTTP
    return 0;  // Initialisation r�ussie
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {  // V�rifier si c'est le bon UART
        sim808_rx_buffer[rx_index++] = rx_byte;  // Ajouter l'octet re�u au buffer

        // Si le buffer est plein, le r�initialiser
        if (rx_index >= SIM808_RX_BUFFER_SIZE) {
            rx_index = 0;
        }

        // Red�marrer la r�ception pour le prochain octet
        HAL_UART_Receive_IT(huart, &rx_byte, 1);
    }
}

int GPRS_HTTP_Post(UART_HandleTypeDef *huart, const char *url, const char *data, char *response) {
    char command[SIM808_TX_BUFFER_SIZE];

    // Initialiser la r�ponse � vide
    memset(response, 0, SIM808_RX_BUFFER_SIZE);

    // 1. Initialiser le module pour la communication HTTP
    snprintf(command, sizeof(command), "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);

    snprintf(command, sizeof(command), "AT+SAPBR=3,1,\"APN\",\"orange.internet.org\"\r\n");  // Remplacez 'your_apn' par votre APN
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);

    snprintf(command, sizeof(command), "AT+SAPBR=1,1\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(3000);

    // 2. Pr�parer l'URL et envoyer la commande HTTP POST
    snprintf(command, sizeof(command), "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);

    // 3. D�finir le type de contenu pour les donn�es
    snprintf(command, sizeof(command), "AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);

    // 4. Envoyer les donn�es � envoyer dans le corps de la requ�te POST
    snprintf(command, sizeof(command), "AT+HTTPDATA=%d,10000\r\n", strlen(data));  // Sp�cifier la longueur des donn�es
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);

    // Envoyer les donn�es
    HAL_UART_Transmit_IT(huart, (uint8_t*)data, strlen(data));
    HAL_Delay(3000);

    // 5. Lancer la requ�te HTTP POST
    snprintf(command, sizeof(command), "AT+HTTPACTION=1\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(3000);  // Attente pour recevoir la r�ponse

    // 6. Lire la r�ponse
    // Red�marrer la r�ception UART en mode interruption
    HAL_UART_Receive_IT(huart, &rx_byte, 1);

    // Attendre que toute la r�ponse soit re�ue
    while (rx_index == 0);  // Boucle tant que rien n'a �t� re�u

    // Copier le buffer de r�ception dans la r�ponse
    memcpy(response, sim808_rx_buffer, rx_index);

    // R�initialiser l'index pour la prochaine r�ception
    rx_index = 0;

    // V�rifier si la requ�te a r�ussi
    if (strstr((char *)response, "OK") != NULL) {
        return 0;  // Succ�s
    } else {
        return -1; // Erreur
    }
}

int GPRS_HTTP_Get(UART_HandleTypeDef *huart, const char *url, char *response) {
    char command[SIM808_TX_BUFFER_SIZE];

    // Initialiser la r�ponse � vide
    memset(response, 0, SIM808_RX_BUFFER_SIZE);

    // 1. Initialiser le module pour la communication HTTP
    snprintf(command, sizeof(command), "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);

    snprintf(command, sizeof(command), "AT+SAPBR=3,1,\"APN\",\"orange.internet.org\"\r\n");  // Remplacez 'your_apn' par votre APN
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(1000);

    snprintf(command, sizeof(command), "AT+SAPBR=1,1\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(3000);


    // 3. Lancer la requ�te HTTP GET
    snprintf(command, sizeof(command), "AT+HTTPACTION=0\r\n");
    HAL_UART_Transmit_IT(huart, (uint8_t*)command, strlen(command));
    HAL_Delay(3000);

    // 4. Lire la r�ponse
    // Red�marrer la r�ception UART en mode interruption
    HAL_UART_Receive_IT(huart, &rx_byte, 1);

    // Attendre que toute la r�ponse soit re�ue
    while (rx_index == 0);  // Boucle tant que rien n'a �t� re�u

    // Copier le buffer de r�ception dans la r�ponse
    memcpy(response, sim808_rx_buffer, rx_index);

    // R�initialiser l'index pour la prochaine r�ception
    rx_index = 0;

    // V�rifier si la requ�te a r�ussi
    if (strstr((char *)response, "OK") != NULL) {
        return 0;  // Succ�s
    } else {
        return -1; // Erreur
    }
}

void GPRS_Init(UART_HandleTypeDef *huart) {
    // Activer les horloges n�cessaires
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    // Configurer les broches TX (PA9) et RX (PA10)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configurer USART1
		
    huart->Instance = USART1;
    huart->Init.BaudRate = 9600;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_1;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(huart);
	}