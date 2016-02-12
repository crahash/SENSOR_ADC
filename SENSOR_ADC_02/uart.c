/* Les caractères saisis par Hyperterminal sont renvoyés en écho ou affichés sur le LCD
 *
 * 03/2005 Florian Schaeffer
 */

#include <avr/interrupt.h>

#define UART_BAUD_RATE 9600

#define UART_BAUD_CALC(UART_BAUD_RATE,F_CPU) ((F_CPU)/((UART_BAUD_RATE)*16L)-1)

#define RBUFFLEN 40 // longueur du tampon de réception sérielle

volatile unsigned char rbuff[RBUFFLEN];		// tampon circulaire
volatile uint8_t rbuffpos,	// prochaine position a lire dans le tampon circulaire
			rbuffcnt,	// nombre de caracteres a lire dane le tampon
			udr_data;	// données de l'UART (volatile, 
                        // pour eviter l'optimisation par le preprocesseur)

// Routine d'interruption, lit les caracteres de l'UART
ISR (USART_RXC_vect) 		
{
	udr_data= UDR; 		// toujours lire un octet, sinon interruption sans fin
    
	// ne pas ecrire de caractere dans un tampon circulaire plein
	if(rbuffcnt < RBUFFLEN) rbuff[(rbuffpos+rbuffcnt++) % RBUFFLEN] = udr_data;	
	// quelle position ? caractere lu + nombre caracteres MODULO longueur tampon 
	// (recommencer de zero quand la fin est atteinte)
}


unsigned char ser_getc (void)		
{
	unsigned char c;

	while(!rbuffcnt); 	// attend un caractere disponible 
	
	cli();	// Suspendre interruptions. 
		
	rbuffcnt--;		// un caractere de moins a afficher
	c = rbuff[rbuffpos++];	// aller chercher le caractere pret a etre lu
	if (rbuffpos >= RBUFFLEN)  rbuffpos = 0;	
		// dernier caractere lu (a droite du tampon), recommencer du debut

	sei();			// re-activer les interruptions

	return (c);		// renvoyer le caractère
}

// Emettre un caractere 
void uart_putc(unsigned char c)
{
    while(!(UCSRA & (1 << UDRE)));  // attend UDR pret 
    UDR = c;  				// emet  caractere
}

// Emission d'une chaine
void uart_puts (char *s)
{
    while (*s)
    {   // tant que longueur *s != NULL 
        uart_putc(*s);
        s++;
    }
}

//  initialiser USART
void uart_init(void)
{
	sei();	// activer interruptions 

	UCSRB |= (1 << TXEN);			// activer UART TX (emission)
	UCSRB |= (1 << RXEN );			// activer UART RX (reception) 
	UCSRB |= (1 << RXCIE);			// activer interruption sur données entrantes
	UCSRC |= (1<<URSEL)|(3<<UCSZ0);	// asynchrone, 8N1
	UBRRH=(uint8_t)(UART_BAUD_CALC(UART_BAUD_RATE,F_CPU)>>8);	// choix debit
	UBRRL=(uint8_t)UART_BAUD_CALC(UART_BAUD_RATE,F_CPU);
}
