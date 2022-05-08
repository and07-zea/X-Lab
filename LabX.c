/* 
 * File:   LabX.c
 * Author: Andrea Rodriguez Zea
 *Descripcion:
 */

#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 1000000

uint8_t i = 0;                                                      // Posicion del mensaje 
uint8_t i2 = 0;                                                     // Bandera para potenciometro
uint8_t valor_pot = 0;                                              // Valor del potenciometro 
uint8_t valor_ASCII = 0;                                            // Valor en ASCII
uint8_t modo = 0;                                                   // Modo de operacion 

char    mensaje[3] = {0,0,0};                                       // Mensaje del potenciometro
char    str;                                                        // Cadena de texto
char selection;
 char RX;

void setup(void);                                                   
void obtener_valor(uint8_t valor);                                  // Obtener digitos en ASCII
void imprimir(char *str);                                           // Imprimir una cadena de texto 
void TX_usart(char data);                                           // Enviar valores de datos 

// INT CONFIG
void __interrupt() isr (void){
    if(PIR1bits.RCIF){                                              // Se recibio un dato
        valor_ASCII = RCREG;                                        // Valor recibido
        if(modo == 1){                                              // Si esta en modo enviar
            PORTB = valor_ASCII;                                    // Colocar ASCII en puerto B
            modo = 0;                                               // Modo normal de regreso
            i = 0;                                                  // Se imprime el menu
        }
    }
    else if (PIR1bits.ADIF){                                        // Interrupcion de ADC
        valor_pot = ADRESH;                                         // Valor del potenciometro
        PIR1bits.ADIF = 0;                                          // Se limpia bandera de interrupcion
    }
    return;
}

// MAIN LOOP
void main(void) {
    setup();
    while(1){
        if(ADCON0bits.GO == 0){                                     // No hay proceso de conversion
            ADCON0bits.GO = 1;                                      // Se inicia el proceso de conversion
        }
        if (i == 0){                                                // Mostrar menú
            imprimir("\r\r Que desea realizar \r "                  
                    "   1) Leer el potenciometro \r "
                    "   2) Enviar ASCII \r\r");                     
            i = 1;                                                  // Regresar a menú inicial
        }
        
        selection = RX;
        
        if (valor_ASCII == '1'){                                    // 1) Leer el potenciómetro
            i = 0;                                                  // Imprimir el menu
            modo = 0;                                               // Modo 1
            obtener_valor(valor_pot);                               // Obtener valor en ASCII
            i2 = 0;                                                 // Imprimir valor del potenciometro
            imprimir("\r El valor en potenciometro es ");           // Texto de despliegue
            if (i2 == 0){       
                TX_usart(mensaje[0]);                               // Valor centenas
                TX_usart(mensaje[1]);                               // Valor decenas
                TX_usart(mensaje[2]);                               // Valor unidades
                i2 = 0;                                             // Clear
            }
            valor_ASCII = 0;                                        // Limpiar el valor de ASCII
        }
        
        if (valor_ASCII == '2'){                                    // 2) Enviar ASCII        
            modo = 1;                                               // Cambio de modo 
            imprimir("\r Ingrese un caracter para mostrar en ASCII");
                
            while (PIR1bits.RCIF == 0);                             // Esperar
                PORTB = RX;                                         // Pasar el valor al puerto A
                imprimir(RX);                                       // Mostrar el caracter en LED
                imprimir("\r Listo \r");
                valor_ASCII = 0;                                    // Clear ASCII
                break;
        }
    }
    return;
}

// CONFIG
void setup(void){
    ANSELH = 0;                                                     // I/O digitales)
    ANSEL = 0b00000001;                                             // AN0 como entrada anal?gica
    
    
    TRISA = 0b00000001;                                             // AN0 como entrada
    PORTA = 0; 
    
    TRISB = 0b00000000;                                             // Puerto B como salida
    PORTB = 0b00000000;
    
    // CONFIG INT CLK
    OSCCONbits.IRCF = 0b0100;                                       // 1MHz
    OSCCONbits.SCS = 1;                                             // Oscilador interno
    
    // Configuraci?n ADC
    ADCON0bits.ADCS = 0b00;                                         // 1MHz
    ADCON1bits.VCFG0 = 0;                                           // VDD
    ADCON1bits.VCFG1 = 0;                                           // VSS
    ADCON0bits.CHS = 0b0000;                                        // Seleccionamos el AN0
    ADCON1bits.ADFM = 0;                                            // Justificado a la izquierda
    ADCON0bits.ADON = 1;                                            // Habilitamos modulo ADC
    __delay_us(40);                                                 // Sample time

    // SERIAL COMM
    
    // SYNC = 0,    BRGH = 1,   BRF16 = 1,  SPBRG = 25
    TXSTAbits.SYNC = 0;                                             // Asincr?nica
    TXSTAbits.BRGH = 1;                                             // HIGH Baud rate 
    BAUDCTLbits.BRG16 = 1;                                          // 16 bits para baud rate 
    
    SPBRG = 25;
    SPBRGH = 0;                                                     // Baud rate 9600
    
    RCSTAbits.SPEN = 1;                                             // Habilitamos comuniaci?n 
    TXSTAbits.TX9 = 0;                                              // Solo se utilizan 8 bits
    TXSTAbits.TXEN = 1;                                             // Habilitamos transmisor
    RCSTAbits.CREN = 1;                                             // Habilitamos receptor

    // Configuracion interrupciones
    PIR1bits.ADIF = 0;                                              // Limpiamos bandera de ADC
    PIE1bits.ADIE = 1;                                              // Habilitamos interrupcion de ADC
    INTCONbits.PEIE = 1;                                            // Habilitamos interrupciones de perifericos
    INTCONbits.GIE = 1;                                             // Habilitamos interrupciones globales
    PIE1bits.RCIE = 1;                                              // Habilitamos interrupciones de recepci?n
    
}

/* ---------------------------------------------------------------------------         
 *  FUNCIONES
 ---------------------------------------------------------------------------*/

void obtener_valor(uint8_t valor){
    mensaje[0] = valor/100;                                         // Centenas
    mensaje[1] = (valor-mensaje[0]*100)/10;                         // Decenas
    mensaje[2] = valor-mensaje[0]*100-mensaje[1]*10;                // Unidades
    
    // Convertit a ASCII sumando 30 en hex
    mensaje[0] = mensaje[0]+0x30;
    mensaje[1] = mensaje[1]+0x30;
    mensaje[2] = mensaje[2]+0x30;
}

void imprimir(char *str){
    while (*str != '\0'){                                           // Cuando no est? vac?o
        TX_usart(*str);                                             // Enviar elemento de str
        str++;                                                      // Enviar siguiente elemento
    }
}

void TX_usart(char data){
    while(TXSTAbits.TRMT == 0);                                     // Esperar a que se vacie
    TXREG = data;                                                   // Enviar dato especificado
}