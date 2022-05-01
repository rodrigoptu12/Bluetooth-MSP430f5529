//// Rodrigo Pereira Couto - 190116510

#include <msp430.h>

#define E_OFF          (P6OUT |= BIT0) //Energia OFF
#define E_ON           (P6OUT &= ~BIT0)//Energia On
#define E_TOGGLE       (P6OUT ^= BIT0)//Energia Toggle
#define SOM            (P2IN&BIT0)

unsigned char ucb0_rxByte;
unsigned int i,status,count=0,soundMode = 0;
char EON[10] = {'E','n','e','r','g','y',':','O','N','\n'};  //Energy On
char EOF[11] = {'E','n','e','r','g','y',':','O','F','F','\n'}; //Energy OFF
char SMA[12] = {'S','o','n','d','M','o','d','e',':','O','N','\n'};  //SoundMode Ativado
char SMD[13] = {'S','o','n','d','M','o','d','e',':','O','F','F','\n'};  //SoundMode Ativado

void Estado();
void gpio(void);
void UART();
void timers(void);
void Sensor_Mic(void);

int main(void)
{
      WDTCTL = WDTPW + WDTHOLD;
      gpio();
      UART();
      timers();
      __enable_interrupt();

      while(1){
          Sensor_Mic();
      }
}
void Sensor_Mic(void){
    //Verifica se o modo som esta ativado
    if(soundMode){
        //Verifica se houve som
        if((SOM) == 0){
            count++;
            __delay_cycles(70000);
        }
        //Se houver detectado 2 sinais de som em menos de 2 segundo desliga ou liga a energia
        if(count == 2){
            count = 0;
            E_TOGGLE;
            status = (status == 1) ? 0 : 1;
            Estado();
        }
    }
}
// Timer
void timers(void){
    TA0CCR0 = 65535;                          //2 sec
    TA0CTL = TASSEL_1 + MC__STOP + TACLR;     // ACLK, upmode, clear TAR
    TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
  count = 0;                                  // Zera o contador de sons
}

// UART
void UART(){
    P3SEL |= BIT3 | BIT4;                    // P3.3 - RX and P3.4 - TX
    //Desliga o módulo
    UCA0CTL1 |= UCSWRST;

    UCA0CTL1 |= UCSSEL_1;
    UCA0BR0 = 0x03;                          // 32,768kHz 9600
    UCA0BR1 = 0x00;
    UCA0MCTL = UCBRS_3 | UCBRF_0;

    //Liga o módulo
    UCA0CTL1 &= ~UCSWRST;

    UCA0IE |= UCRXIE;                        //Interrupt on Reception

}

void gpio(void){
    P6DIR |= BIT0;  // Rele
    P6OUT &= ~BIT0;

    P2DIR &= ~BIT0;  // Modulo de som

}

// Envia o estado se esta ligada ou desligada
void Estado(){
    //Mostra se o SoundMode esta ativado ou desativado
    if(soundMode == 1){
        for( i=0;i<12;i++){
            while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
            UCA0TXBUF = SMA[i];
        }
    }else{
        for( i=0;i<13;i++){
            while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
            UCA0TXBUF = SMD[i];
        }
    }
    //Mostra se a energia esta ativada ou desativada
    if(status == 1){
        for( i=0;i<10;i++){
            while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
            UCA0TXBUF = EON[i];
        }
    }
    else if(status == 0){
        for( i=0;i<11;i++){
            while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
            UCA0TXBUF = EOF[i];
        }
    }
}

// Recebe o dado se liga ou desliga
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void){
    ucb0_rxByte = UCA0RXBUF;
    switch(ucb0_rxByte){
        case '4':
            soundMode = 0;
            Estado();
            TA0CTL = TASSEL_1 |MC__STOP; //Stop timer
            break;
        case '3':
           soundMode = 1;
           Estado();
           TA0CTL |=(MC__UP | TACLR); //Start timer
           break;
        case '2':
            Estado();
            break;
        case '1':
            E_ON;
            status = 1;
            Estado();
            break;
        case '0':
            E_OFF;
            status = 0;
            Estado();
            break;
    }
}
