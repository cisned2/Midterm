/*
 * Midterm.c
 *
 * Created: 3/20/2018 1:54:58 PM
 * Author : Damian Cisneros
 * Description : This program monitors temperature using an LM34 sensor. It
 *				 reads the temperature every 1s and displays it on a serial
 *				 terminal. Using 8Mhz clock
 */ 

#define BAUD 9600
#define F_CPU 8000000UL

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>

//Used to enable printf for use in USART
static int put_char(char c, FILE *stream);
static FILE mystdout = FDEV_SETUP_STREAM(put_char, NULL, _FDEV_SETUP_WRITE);

void init_ADC();
void init_USART();
void USART_tx_string( char*);
//void writeData(unsigned char);

int timer = 0; //send data every so often to Thingspeak based on this value

//AT Commands
 char ATCWMODE[] = "AT+CWMODE=1\r\n";
 char ATCWJAP[] = "AT+CWJAP=\"TEST\",\"\"\r\n";
 char CIPMUX[] = "AT+CIPMUX=1\r\n";
 char CIPSTART[] = "AT+CIPSTART=0,\"TCP\",\"api.thingspeak.com\",80\r\n";
 char CIPSEND[] = "AT+CIPSEND=0,48\r\n";
 char SENDDATA[] = "GET /update?key=SFMSNLTJWNDH5Y6L&field1=";
 char CIPCLOSE[] = "AT+CIPCLOSE\r\n";

int main(void)
{
	float ADCvalue; //holds converted ADC value
	 char c[9]; //holds converted value in string

	stdout = &mystdout; //set the output stream

	init_USART();  
	init_ADC();

	_delay_ms(300);
	printf("AT\r\n");
	//USART_tx_string(AT); //test AT command
	_delay_ms(2000);	
	USART_tx_string(ATCWMODE);//change to client mode
	_delay_ms(2000);
	USART_tx_string(ATCWJAP); //connect to wifi
	_delay_ms(8000);	
	USART_tx_string(CIPMUX); //multiple connection set
	_delay_ms(6000);
	
	while(1){
		ADCSRA |= (1 << ADSC); //start the conversion. while in free running mode it will
		while((ADCSRA&(1 << ADIF))==0); //check if conversion done
		ADCSRA |= (1 << ADIF); //reset flag
		ADCvalue = ADC & 0x03ff; //grab all 10 bits from ADC
		ADCvalue = ((ADCvalue * 5)/1024)*100; //convert to degrees Fahrenheit
		
		//itoa(ADCvalue, c, 10); //convert int to string
		
		if(timer==3){
			timer = 0; //reset timer

			_delay_ms(1000);		
			dtostrf(ADCvalue,3,1,c); //convert double to string
			
			USART_tx_string(CIPSTART);
			_delay_ms(2000);
			USART_tx_string(CIPSEND);
			_delay_ms(4000);
			USART_tx_string(SENDDATA);
			USART_tx_string(c);
			USART_tx_string("\r\n");
			_delay_ms(2000);
			USART_tx_string(CIPCLOSE);
			_delay_ms(1000); //wait 1s
		}
		timer++;
	}
}

void init_USART(){
	unsigned int BAUDrate;

	//set BAUD rate: UBRR = [F_CPU/(16*BAUD)]-1
	BAUDrate = ((F_CPU/16)/BAUD) - 1;
	UBRR0H = (unsigned char) (BAUDrate >> 8); //shift top 8 bits into UBRR0H
	UBRR0L = (unsigned char) BAUDrate; //shift rest of 8 bits into UBRR0L
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0); //enable receiver and trasmitter
	// UCSR0B |= (1 << RXCIE0); //enable receiver interrupt
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); //set data frame: 8 bit, 1 stop
}

void init_ADC(){
	ADMUX = 0; //use ADC0
	ADMUX |= (1 << REFS0); //use AVcc as the reference (5V)
	//ADMUX |= (1 << ADLAR); //set to right adjust for 8-bit ADC

	//ADCSRA |= (1 << ADIE); //ADC interrupt enable
	ADCSRA |= (1 << ADEN); //enable ADC
	
	//set pre-scale to 128 for input frequency
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	ADCSRB = 0; //free running mode
}

void USART_tx_string(char* data){
	while((*data!='\0')){ //print until null
		while(!(UCSR0A &(1<<UDRE0))); //check if transmit buffer is ready for new data
		UDR0=*data; //print char at current pointer
		data++; //iterate char pointer
	}
}

static int put_char(char c, FILE *stream)
{
	while(!(UCSR0A &(1<<UDRE0))); // wait for UDR to be clear
	UDR0 = c;    //send the character
	return 0;
}