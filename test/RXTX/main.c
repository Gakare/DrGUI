#define F_CPU 16000000

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h> // Needed for uint_8t
#include <stdio.h> // Needed for string formatting
#include <util/delay.h>
#include <string.h>

#define ArraySize(Arr) (sizeof(Arr) / sizeof(Arr[0]))

#define BAUD 9600

void InitUSART(int16_t Baud) {
    /* Set Baud Rate */
    // UBBR = (F_CPU / (16 * BAUD)) - 1
    unsigned int BaudRate = ((F_CPU / 16) / Baud) - 1;
    UBRR0H = (uint8_t)(BaudRate >> 8);
    UBRR0L = (uint8_t)BaudRate;
    /* Enable receiver and transmitter */
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << UDRIE0);
    /* Set data frame: 8-1-N */
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);
}

// TODO: Determine what the right buffer size should be
#define BUFFER_SIZE 32
volatile uint8_t rx_buffer[BUFFER_SIZE];
volatile uint8_t head = 0;
volatile uint8_t tail = 0;

uint8_t GetByte(void) {
    uint8_t byte = rx_buffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    return (byte);
}

uint8_t BytesAvailable(void) {
    return (head + BUFFER_SIZE - tail) % BUFFER_SIZE;
}

/* Transmit data */
void UART_transmit(char Data) {
    while (!(UCSR0A & (1 << UDRE0)))
        ;
    UDR0 = Data;
}

/* Helper function */
void WriteStr(const char *Str) {
    while (*Str) { // Traverses string till null
        UART_transmit(*Str++);
    }
}

typedef struct drone_data{
	int LXInput;
	int LYInput;
} __attribute__((packed)) drone_data;

drone_data Deserialize(uint8_t *Buffer) {
    drone_data Result = {};
    if (Buffer) {
        Result.LXInput = (int)Buffer[0] | ((int)Buffer[1] << 8) | ((int)Buffer[2] << 16) |
            ((int)Buffer[3] << 24);
        Result.LYInput = (int)Buffer[4] | ((int)Buffer[5] << 8) | ((int)Buffer[6] << 16) |
            ((int)Buffer[7] << 24);	
    }
    return (Result);
}

ISR (USART_RX_vect) {
    uint8_t ReceivedByte = UDR0;

    uint8_t NextHead = (head + 1) % BUFFER_SIZE;
    if (NextHead != tail) {
        rx_buffer[head] = ReceivedByte;
        head = NextHead;
    }
}

static uint32_t ReadyToSend = 0;
ISR (USART_UDRE_vect) {
    ReadyToSend = 1;
}

void Serialize(uint8_t *Buffer, drone_data Data) {
    if (Buffer) {
        Buffer[0] = (Data.LXInput >> 0) & 0xFF;
        Buffer[1] = (Data.LXInput >> 8) & 0xFF;
        Buffer[2] = (Data.LXInput >> 16) & 0xFF;
        Buffer[3] = (Data.LXInput >> 24) & 0xFF;

        Buffer[4] = (Data.LYInput >> 0) & 0xFF;
        Buffer[5] = (Data.LYInput >> 8) & 0xFF;
        Buffer[6] = (Data.LYInput >> 16) & 0xFF;
        Buffer[7] = (Data.LYInput >> 24) & 0xFF;
    }
}

void Transmit(drone_data Data) {
    UART_transmit((Data.LXInput >> 0) & 0xFF);
    UART_transmit((Data.LXInput >> 8) & 0xFF);
    UART_transmit((Data.LXInput >> 16) & 0xFF);
    UART_transmit((Data.LXInput >> 24) & 0xFF);

    UART_transmit((Data.LYInput >> 0) & 0xFF);
    UART_transmit((Data.LYInput >> 8) & 0xFF);
    UART_transmit((Data.LYInput >> 16) & 0xFF);
    UART_transmit((Data.LYInput >> 24) & 0xFF);
}

static drone_data Data;
static uint8_t buffer[8];

int main(void) {
    InitUSART(BAUD);
    sei();

    Data.LXInput = 200;
    Data.LYInput = 420;

    while (1) {
        if (BytesAvailable() >= 8) {
            for (int i = 0; i < 8; ++i) {
                buffer[i] = GetByte();
            }	
            Data = Deserialize(buffer);
        }

        if (ReadyToSend) {
            ReadyToSend = 0;
			Data.LXInput++;
			if (Data.LXInput == 1000) {
				Data.LXInput = 200;
			}
			Data.LYInput++;
			if (Data.LYInput == 1000) {
				Data.LYInput = 420;
			}
            Serialize(buffer, Data);
            Transmit(Data);
        }
    }
    return (0);
}
