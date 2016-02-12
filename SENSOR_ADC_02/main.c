#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "uart.h"

#define EN_SENSOR_PORT_DIR          DDRC
#define EN_SENSOR_STATE             PORTC
#define EN_SENSOR_PIN               PC0

#define SENSOR_PORT_DIR             DDRC
#define SENSOR_STATE                PORTC
#define SENSOR_PIN                  PC4     // ADC4

#define LED1_PORT_DIR               DDRC
#define LED1_STATE                  PORTC
#define LED1_PIN                    PC1

#define LED2_PORT_DIR               DDRC
#define LED2_STATE                  PORTC
#define LED2_PIN                    PC2



void initIO(void) {
    LED1_PORT_DIR |= (1 << LED1_PIN);  // 1 : sortie
	LED2_PORT_DIR |= (1 << LED2_PIN);  // 1 : sortie
    
    LED1_STATE |= (1 << LED1_PIN);        // 1 : OFF
    LED2_STATE |= (1 << LED2_PIN);        // 1 : OFF
    
    EN_SENSOR_PORT_DIR |= (1 << EN_SENSOR_PIN);   // 1 : sortie
    EN_SENSOR_STATE |= (1 << EN_SENSOR_PIN);      // 1 : OFF
}



//-----------------------------------------------------------------------
//                              ADC config
//-----------------------------------------------------------------------

inline void initADC(void){
    ADMUX |= (1 << REFS0) | (1 << REFS1) | (1 << MUX2); // internal ref : 2.56V and ADC4 input selected
    //ADCSRA |= (1 << ADPS1); // ADPS2 = 0 ; ADPS1 = 1 ; ADPS0 = 0 : prescaler = 4 ==> Fcan = 250khz
    //ADCSRA |= (1 << ADFR);  // Free Running mode
    //ADCSRA |= (1 << ADIE);    // Enable ADC interrupt
}

inline void startADCconversion(void){
    ADCSRA |= (1 << ADEN); // enable ADC
    ADCSRA |= (1 << ADSC); // Start conversion
    // delete first measurement
    while (ADCSRA & (1<<ADSC));
    uint16_t res = ADCW; 
}

inline void stopADCconversion(void){
    ADCSRA &=~ (1<<ADEN); //stop ADC
}

//-----------------------------------------------------------------------
//                            Compter0 config
//-----------------------------------------------------------------------

inline void startCOMPTER0(void){
    TCCR0 |= (1<<CS00); //prescaler = 1 Ftimer = 1Mhz
    TCNT0 = 0x00;       //clear compter
}
inline uint8_t stopCOMPTER0(void){
    uint8_t res = TCNT0;
    TCCR0 &=~  (1<<CS00) | (1<<CS01) | (1<<CS02);
    return res;
}

//-----------------------------------------------------------------------
//                            Sensor config
//-----------------------------------------------------------------------

inline void enableSensor(void){
    EN_SENSOR_STATE &=~ (1 << EN_SENSOR_PIN); // 0 : ON
}

inline void disableSensor(void){
    EN_SENSOR_STATE |= (1 << EN_SENSOR_PIN); // 1 : OFF
}


int main(void) {
	initIO();
    initADC();
    uart_init();
    enableSensor();
    startADCconversion();

    
    uint16_t ADC_res = 0;
    uint16_t ADC_prevRes =0;
    uint8_t edgeTrigger = 0;
    
    uint8_t nbrPoint= 10;
    
    uint16_t compter_res[nbrPoint];
    uint8_t index = 0;
    uint16_t compter_res_temp;
    
    uint16_t alpha = 178;
    uint16_t prev_res = 256;

    double sum = 0;
    double avg = 0;
    
    uint8_t int_avg;
    
    char buffer [40];
    uart_puts("this is a test");
    uart_putc('\n');
    while (1) {
        enableSensor();
        startADCconversion();
        ADCSRA |= (1<<ADSC);
        while (ADCSRA & (1<<ADSC));
        ADC_res = ADCW;
        if (ADC_res + 300 < ADC_prevRes && !edgeTrigger) {
            //LED1_STATE |= (1<<LED1_PIN);
            startCOMPTER0();
            edgeTrigger = 1;
        }
        else if (ADC_res + 300 < ADC_prevRes && edgeTrigger) {
            //LED1_STATE &=~ (1<<LED1_PIN);
            compter_res_temp = stopCOMPTER0();
            if (compter_res_temp > 50 && compter_res_temp < 255) {
                compter_res[index] = (alpha*compter_res_temp + (256-alpha)*prev_res)/256;
                prev_res = compter_res[index];
                index++;
            }
            edgeTrigger = 0;
            if(index== nbrPoint){
                stopADCconversion();
                disableSensor();
                for (uint8_t i = 0; i<nbrPoint; i++) {
                    sum += compter_res[i];
                    /*
                    uart_puts(itoa(compter_res[i],buffer,10));
                    uart_putc('\n');
                    */
                }
                avg = sum / nbrPoint;
                int_avg = (int) avg;
                /*
                uart_puts(itoa(int_avg,buffer,10));
                uart_putc('\n');
                */
               
                if (int_avg > 160) {
                    uart_putc('R');
                }
                else if (int_avg > 100 && int_avg < 160) {
                    uart_putc('P');
                }
                else if (int_avg < 100 ) {
                    uart_putc('O');
                }
                uart_putc('\n');
                avg = 0;
                sum = 0;
                //while (1);
            }
        }
        ADC_prevRes = ADC_res;
        
	}
	return 0; // never reached
}
