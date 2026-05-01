// NOTE: PC4 (SDA), PC5 (SCL)
#define F_CPU 16000000

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h> // Needed for uint_8t
#include <stdio.h> // Needed for string formatting
#include <util/delay.h>
#include <string.h>

#define ArrayCount(Arr) (sizeof(Arr) / sizeof(Arr[0]))

#define BAUD 9600

void InitUSART(int16_t Baud) {
    /* Enabling pull-up resistor on RX */
    DDRD &= ~(1 << PD0);
    PORTD |= (1 << PD0);
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
        Result.LXInput = (int)(Buffer[0] | ((uint32_t)Buffer[1] << 8) | ((uint32_t)Buffer[2] << 16) |
            ((uint32_t)Buffer[3] << 24));
        Result.LYInput = (int)(Buffer[4] | ((uint32_t)Buffer[5] << 8) | ((uint32_t)Buffer[6] << 16) |
            ((uint32_t)Buffer[7] << 24));	
    }
    return (Result);
}

#define BUFFER_SIZE 32
static uint8_t volatile GlobalRXBuffer[BUFFER_SIZE];
static uint8_t volatile GlobalRXHead = 0;
static uint8_t volatile GlobalRXTail = 0;

uint8_t GetByte(void) {
    uint8_t Byte = GlobalRXBuffer[GlobalRXHead];
    GlobalRXHead = (GlobalRXHead + 1) % BUFFER_SIZE;
    return (Byte);
}

uint8_t BytesAvailable(void) {
    uint8_t Result = (GlobalRXTail + BUFFER_SIZE - GlobalRXHead) % BUFFER_SIZE;
    return (Result);
}

ISR (USART_RX_vect) {
    uint8_t ReceivedByte = UDR0;

    uint8_t NextTail = (GlobalRXTail + 1) % BUFFER_SIZE;
    if (NextTail != GlobalRXHead) {
        GlobalRXBuffer[GlobalRXTail] = ReceivedByte;
        GlobalRXTail = NextTail;
    }
}

static volatile uint32_t ReadyToSend = 0;
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
    // TODO: Add a start byte
    // UART_transmit(0xFF);
    UART_transmit((Data.LXInput >> 0) & 0xFF);
    UART_transmit((Data.LXInput >> 8) & 0xFF);
    UART_transmit((Data.LXInput >> 16) & 0xFF);
    UART_transmit((Data.LXInput >> 24) & 0xFF);

    UART_transmit((Data.LYInput >> 0) & 0xFF);
    UART_transmit((Data.LYInput >> 8) & 0xFF);
    UART_transmit((Data.LYInput >> 16) & 0xFF);
    UART_transmit((Data.LYInput >> 24) & 0xFF);
}

int main(void) {
    cli();
    InitUSART(BAUD);

    static drone_data Data;
    static uint8_t buffer[8];

    Data.LXInput = 200;
    Data.LYInput = 420;

    sei();
    while (1) {
        // NOTE: Reading
#if 1
        if (BytesAvailable() >= 7) {
            cli();
            for (int i = 0; i < 8; ++i) {
                buffer[i] = GetByte();
            }
            GlobalRXTail = GlobalRXHead;
            Data = Deserialize(buffer);
#if 1
            char TxtBuffer[256];
            snprintf(TxtBuffer, ArrayCount(TxtBuffer), "LXInput: %d, LYInput: %d\r\n",
                     Data.LXInput, Data.LYInput);
            WriteStr(TxtBuffer);
#endif
            sei();
        }
#endif

        // NOTE: Writing
#if 0
        if (ReadyToSend) {
            cli();
            ReadyToSend = 0;
			//Data.LXInput++;
			if (Data.LXInput == 1000) {
				Data.LXInput = 200;
			}
			//Data.LYInput++;
			if (Data.LYInput == 1000) {
				Data.LYInput = 420;
			}
            Serialize(buffer, Data);

            Transmit(Data);
            sei();
        }
#endif
    }

    return (0);
}
