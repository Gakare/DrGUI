#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define ESC1 PB1
#define ESC2 PC1  // moved from PB2 to PC1 (Arduino A1)
#define ESC3 PD5
#define ESC4 PD6

volatile uint16_t esc_ticks[4];
volatile uint8_t  esc_phase = 0;

typedef struct { uint16_t ticks; uint8_t index; } EscEvent;
EscEvent events[4];

void sort_events(void) {
    for (uint8_t i = 1; i < 4; i++) {
        EscEvent key = events[i];
        int8_t j = i - 1;
        while (j >= 0 && events[j].ticks > key.ticks) {
            events[j+1] = events[j];
            j--;
        }
        events[j+1] = key;
    }
}

void rebuild_events(void) {
    for (uint8_t i = 0; i < 4; i++) {
        events[i].ticks = esc_ticks[i];
        events[i].index = i;
    }
    sort_events();
}

void drop_pin(uint8_t idx) {
    switch(idx) {
        case 0: PORTB &= ~(1 << ESC1); break;
        case 1: PORTC &= ~(1 << ESC2); break;  // changed to PORTC
        case 2: PORTD &= ~(1 << ESC3); break;
        case 3: PORTD &= ~(1 << ESC4); break;
    }
}

ISR(TIMER1_COMPA_vect) {
    if (esc_phase == 0) {
        // Start of frame: pull all pins high
        PORTB |= (1 << ESC1);
        PORTC |= (1 << ESC2);  // changed to PORTC
        PORTD |= (1 << ESC3) | (1 << ESC4);

        rebuild_events();

        OCR1A = events[0].ticks;
        esc_phase = 1;
        } else {
        uint8_t i = esc_phase - 1;

        uint16_t current_ticks = events[i].ticks;
        while (i < 4 && events[i].ticks == current_ticks) {
            drop_pin(events[i].index);
            i++;
        }
        esc_phase = i + 1;

        if (i >= 4) {
            uint32_t frame_ticks = (uint32_t)20000 * (F_CPU / 8UL) / 1000000UL;
            uint32_t used = current_ticks;
            OCR1A = (uint16_t)(frame_ticks - used);
            esc_phase = 0;
            } else {
            OCR1A = events[i].ticks - current_ticks;
            esc_phase = i + 1;
        }
    }
}

void timer1_init(void) {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11);
    OCR1A = (uint32_t)20000 * (F_CPU / 8UL) / 1000000UL;
    TIMSK1 = (1 << OCIE1A);
    esc_phase = 0;
}

void esc_set_all(uint16_t us) {
    uint16_t ticks = (uint32_t)us * (F_CPU / 8UL) / 1000000UL;
    cli();
    for (uint8_t i = 0; i < 4; i++) esc_ticks[i] = ticks;
    sei();
}

int main(void) {
    // Set ESC pins as outputs
    DDRB |= (1 << ESC1);
    DDRC |= (1 << ESC2);           // changed to DDRC
    DDRD |= (1 << ESC3) | (1 << ESC4);

    // Start all pins low
    PORTB &= ~(1 << ESC1);
    PORTC &= ~(1 << ESC2);         // changed to PORTC
    PORTD &= ~((1 << ESC3) | (1 << ESC4));

    esc_set_all(1000);
    timer1_init();
    sei();

    _delay_ms(5000);    // Arming wait at 1000us

    while (1) {
        esc_set_all(1200);
        _delay_ms(3000);

        esc_set_all(1000);
        _delay_ms(3000);
    }
}
