#include <mando.h>

#define LCD_ENABLE_PIN PIN_E0
#define LCD_RS_PIN PIN_E1
#define LCD_RW_PIN PIN_E2
#define LCD_DATA4 PIN_D4
#define LCD_DATA5 PIN_D5
#define LCD_DATA6 PIN_D6
#define LCD_DATA7 PIN_D7

#include "lcd.c"

//PERMITIR USAR LOS PUERTOS A, B, C, D Y E
#use fast_io(A)
#use fast_io(B)
#use fast_io(C)
#use fast_io(D)
#use fast_io(E)

//VARIABLES DE ESTADO DEL MANDO
#define IDLE         0
#define START        1
#define SELECT       2
#define ARRIBA       3
#define ABAJO        4
#define JOYSTICK     5

//VARIABLES A USAR EL PROGRAMA
int8 cargaTMR0 = 61;          //GUARDA LA CARGA DEL TMR0 PARA CONSEGUIR 0.05 s CON UN DESBORDAMIENTO
int8 tick = 0;                //GUARDA CUANTAS TEMPORIZACIONES HAN PASADO DESDE INICIADO EL CONTEO

int8 x = 16;                  //GUARDA EN QUE POSICI�N DEBE PONERSE EL T�TULO EN EL PRIMER RENGL�N
int1 borrado = 0;             //GUARDA SI EL T�TULO YA HA SIDO BORRADO O NO, PARA EL REINICIO DEL MOVIMIENTO

int16 joyX = 0;               //GUARDA EN QUE POSICI�N DE X EST� EL JOYSTICK
int16 joyY = 0;               //GUARDA EN QUE POSICI�N DE Y EST� EL JOYSTICK
signed int8  estadoJoyX = 0;  //GUARDA LA VELOCIDAD EN EL EJE X GENERADO POR EL JOYSTICK 
signed int8  estadoJoyY = 0;  //GUARDA LA VELOCIDAD EN EL EJE Y GENERADO POR EL JOYSTICK
int8 posJoystick = 0;       //GUARDA EN CONJUNTO EN QUE VELOCIDADES EST� EN X E Y

int1 estadoB4 = 1;            //GUARDA EL ESTADO ACTUAL DEL PIN RB4 (EMPIEZA EN 1 = SIN PRESIONAR)
int1 estadoB5 = 1;            //GUARDA EL ESTADO ACTUAL DEL PIN RB5 (EMPIEZA EN 1 = SIN PRESIONAR)
int1 estadoB6 = 1;            //GUARDA EL ESTADO ACTUAL DEL PIN RB6 (EMPIEZA EN 1 = SIN PRESIONAR)
int1 estadoB7 = 1;            //GUARDA EL ESTADO ACTUAL DEL PIN RB7 (EMPIEZA EN 1 = SIN PRESIONAR)

int8 estadoMando = 0;         //GUARDA EL ESTADO DEL MANDO SEG�N EL BOT�N PRESIONADO

//INTERRUPCI�N POR TMR0 PARA APAGAR UN LED DE PRESIONADO UN BOT�N
#INT_RTCC
void moverTitulo(void) 
{
   //CADA 10 TEMPORIZACIONES (0.5 s) MOVER EL T�TULO A LA IZQUIERDA
   tick++;
   if(tick == 10){
      tick = 0;               //REINICIAR EL CONTEO DE 0.5 s 
      if(x == 1){
         x = 16;              //PONER EL T�TULO AL INICIO
         borrado = 0;
      }
      else{
         x = x - 1;           //MOVER 1 A LA IZQUIERDA AL T�TULO
      }
   }
   set_timer0(cargaTMR0);     // Reponemos el timer0
}

//INTERRUPCI�N POR CAMBIO EXTERNO EN RB4-RB7 PARA LOS BOTONES DEL MANDO
#INT_RB
void botones(void) 
{
   estadoB4 = input(PIN_B4);     //OBTENER EL ESTADO DEL PIN_B4
   estadoB5 = input(PIN_B5);     //OBTENER EL ESTADO DEL PIN_B5
   estadoB6 = input(PIN_B6);     //OBTENER EL ESTADO DEL PIN_B6
   estadoB7 = input(PIN_B7);     //OBTENER EL ESTADO DEL PIN_B7
   
   //BOT�N SIN PRESIONAR
   if(estadoB4 == 1 && estadoB5 == 1 && estadoB6 == 1 && estadoB7 == 1){
      estadoMando = IDLE;
      output_low(PIN_D1);              // APAGAR EL LED QUE INDICA BOT�N PRESIONADO
   }
   
   //BOT�N ARRIBA
   if(estadoB4 == 0){  
      estadoMando = ARRIBA;
      output_high(PIN_D1);             // PRENDER EL LED QUE INDICA BOT�N PRESIONADO
   }
   
   //BOT�N ABAJO
   if(estadoB5 == 0){  
      estadoMando = ABAJO;
      output_high(PIN_D1);             // PRENDER EL LED QUE INDICA BOT�N PRESIONADO
   }
   
   //BOT�N SELECT
   if(estadoB6 == 0){  
      estadoMando = SELECT;
      output_high(PIN_D1);             // PRENDER EL LED QUE INDICA BOT�N PRESIONADO
   }
   
   //BOT�N START
   if(estadoB7 == 0){  
      estadoMando = START;
      output_high(PIN_D1);             // PRENDER EL LED QUE INDICA BOT�N PRESIONADO
   } 
}

void main()
{
   //CONFIGURAR PINES DE ENTRADA Y SALIDA DE LOS PUERTOS A, B, C, D Y E
   set_tris_A(0b11111111);       //TODOS LOS PINES DEL PUERTO A DE ENTRADA                      - ENTRADA DE BOTONES
   set_tris_B(0b11111111);       //TODOS LOS PINES DEL PUERTO B DE ENTRADA                      - ENTRADA DE BOTONES
   set_tris_C(0b10000000);       //PIN 7 DE ENTRADA (RX) Y PIN 6 DE SALIDA (TX) DEL PUERTO C    - TRANSMISI�N Y RECEPCI�N DE DATOS
   set_tris_D(0b00000000);       //TODOS LOS PINES DEL PUERTO D DE SALIDA                       - LED INDICADOR Y LCD
   set_tris_E(0b00000000);       //TODOS LOS PINES DEL PUERTO E DE SALIDA                       - LCD
   
   //ACTIVAR LAS RESISTENCIAS PULL-UPS DEL PUERTO B
   port_b_pullups(1);   
   
   //CONFIGURAR TMR0
   setup_timer_0(T0_INTERNAL|T0_DIV_256);       //CONFIGURAR TIMER 0 COMO TEMPORIZADOR Y CON UN PRESESCALER DE 64
   set_timer0(cargaTMR0);                       //Carga del timerO
   
   //CONFIGURAR CONVERSOR ANAL�GICO - DIGITAL
   setup_adc_ports(AN0_AN1_AN3);                //Definimos AN0 como entrada Anal�gica
   setup_adc(ADC_CLOCK_INTERNAL);               //Reloj de conversi�n - Reloj interno RC 
   
   //ACTIVAR INTERRUPCIONES
   enable_interrupts(INT_TIMER0);            //ACTIVAR INTERRUPCI�N POR TIMER 0
   enable_interrupts(INT_RB);                //ACTIVAR INTERRUPCI�N POR CAMBIO EN RB4-RB7
   enable_interrupts(GLOBAL);                //ACTIVAR INTERRUPCIONES GLOBALES
   
   //INICIAR LCD
   lcd_init();
   
   //PRENDER LED INDICADOR DE FUNCIONAMIENTO E INICIAR APAGADO EL LED QUE INDICA QUE SE HA PRESIONADO UN BOT�N
   output_high(PIN_D0);  
   output_low(PIN_D1);
   
   while(TRUE)
   {
      //TRANSMITIR DATOS
      putc(estadoMando);               //TRANSMITIR EL VALOR DE estadoMando
      delay_ms(50);
      //T�TULO DIN�MICO
      if(x == 16 && borrado == 0){
         lcd_putc("\f");
         borrado = 1;
      }
      //INICIAR EN EL PRIMER RENGL�N PARA ESCRIBIR T�TULO, PERO DE MANERA DIN�MICA
      lcd_gotoxy(x,1);
      lcd_putc("ING. MECATRONICA\n");
      
      //INICIAR EN EL SEGUNDO RENGL�N PARA ESCRIBIR QUE BOT�N SE HA PRESIONADO
      lcd_gotoxy(1,2);
      
      //LECTURA DEL JOYSTICK
      set_adc_channel(0);        //SELECCIONAMOS EL CANAL 0 (EJE X)
      delay_us(20);              //RETARDAR 20 us
      joyX = read_adc();         //VALOR DE LA LECTURA DEL CANAL 0 (EJE X)
      
      set_adc_channel(1);        //SELECCIONAMOS EL CANAL 1 (EJE Y)
      delay_us(20);              //RETARDAR 20 us
      joyY = read_adc();         //VALOR DE LA LECTURA DEL CANAL 1 (EJE Y)
      
      ////////////////////////////////////////////////////////////////////////////////
      //ESTADO DEL MANDO SEG�N LA POSICI�N DEL JOYSTICK
      ////////////////////////////////////////////////////////////////////////////////
      //EJE X
      if(joyX >= 480 && joyX <= 544){
         estadoJoyX = 0;         //VELOCIDAD X IGUAL A 0
         posJoystick = 40;
      }
      else if(joyX >= 0 && joyX <= 170){
         estadoJoyX = -3;         //VELOCIDAD NEGATIVA X ALTA
         posJoystick = 10;
      }
      else if(joyX >= 171 && joyX <= 340){
         estadoJoyX = -2;         //VELOCIDAD NEGATIVA X MEDIA
         posJoystick = 20;
      }
      else if(joyX >= 341 && joyX <= 479){
         estadoJoyX = -1;         //VELOCIDAD NEGATIVA X BAJA
         posJoystick = 30;
      }
      else if(joyX >= 545 && joyX <= 683){
         estadoJoyX = 1;          //VELOCIDAD POSITIVA X BAJA
         posJoystick = 50;
      }
      else if(joyX >= 684 && joyX <= 853){
         estadoJoyX = 2;          //VELOCIDAD POSITIVA X MEDIA
         posJoystick = 60;
      }
      else if(joyX >= 854 && joyX <= 1023){
         estadoJoyX = 3;          //VELOCIDAD POSITIVA X ALTA
         posJoystick = 70;
      }
      /////////////////////////////////////////////////////////////////////////
      //EJE Y
      if(joyY >= 480 && joyY <= 544){
         estadoJoyY = 0;         //VELOCIDAD Y IGUAL A 0
         posJoystick = posJoystick + 4;
      }
      else if(joyY >= 0 && joyY <= 170){
         estadoJoyY = -3;         //VELOCIDAD NEGATIVA Y ALTA
         posJoystick = posJoystick + 1;
      }
      else if(joyY >= 171 && joyY <= 340){
         estadoJoyY = -2;         //VELOCIDAD NEGATIVA Y MEDIA
         posJoystick = posJoystick + 2;
      }
      else if(joyY >= 341 && joyY <= 479){
         estadoJoyY = -1;         //VELOCIDAD NEGATIVA Y BAJA
         posJoystick = posJoystick + 3;
      }
      else if(joyY >= 545 && joyY <= 683){
         estadoJoyY = 1;          //VELOCIDAD POSITIVA Y BAJA
         posJoystick = posJoystick + 5;
      }
      else if(joyY >= 684 && joyY <= 853){
         estadoJoyY = 2;          //VELOCIDAD POSITIVA Y MEDIA
         posJoystick = posJoystick + 6;
      }
      else if(joyY >= 854 && joyY <= 1023){
         estadoJoyY = 3;          //VELOCIDAD POSITIVA Y ALTA
         posJoystick = posJoystick + 7;
      }
      ////////////////////////////////////////////////////////////////////////////////
      
      //MANDAR A ESTADO SIN ACCI�N SI NADA EST� SIENDO PRESIONADO O A ESTADO JOYSTICK SI HAY ALGUNA VELOCIDAD DISTINTA A 0, SEA EN X O Y
      if(estadoJoyX == 0 && estadoJoyY == 0 && estadoB4 == 1 && estadoB5 == 1 && estadoB6 == 1 && estadoB7 == 1){
         estadoMando = IDLE;
      }
      if((estadoJoyX != 0 || estadoJoyY != 0) && estadoB4 == 1 && estadoB5 == 1 && estadoB6 == 1 && estadoB7 == 1){
         estadoMando = JOYSTICK;
      }
      
      //ESTRUCTURA PARA ESCRIBIR EN EL LCD SEG�N EL ESTADO
      switch(estadoMando){
         case IDLE:
            lcd_putc("SIN ACCION      ");
            estadoMando = 0;
            //TRANSMITIR DATOS
            putc(estadoMando);               //TRANSMITIR EL VALOR DE estadoMando
            delay_ms(50);
            break;
            
         case START:
            lcd_putc("START           ");
            estadoMando = 1;
            //TRANSMITIR DATOS
            putc(estadoMando);               //TRANSMITIR EL VALOR DE estadoMando
            delay_ms(50);
            break;
            
         case SELECT:
            lcd_putc("SELECT          ");
            estadoMando = 2;
            //TRANSMITIR DATOS
            putc(estadoMando);               //TRANSMITIR EL VALOR DE estadoMando
            delay_ms(50);
            break;
            
         case ARRIBA:
            lcd_putc("ARRIBA          ");
            estadoMando = 3;
            //TRANSMITIR DATOS
            putc(estadoMando);               //TRANSMITIR EL VALOR DE estadoMando
            delay_ms(50);
            break;
            
         case ABAJO:
            lcd_putc("ABAJO           ");
            estadoMando = 4;
            //TRANSMITIR DATOS
            putc(estadoMando);               //TRANSMITIR EL VALOR DE estadoMando
            delay_ms(50);
            break;
            
         case JOYSTICK:
            //DEFINIR EL ESTADO DEL MANDO COMO UN N�MERO DE 2 CIFRAS EN LA QUE LA PRIMERA INDICA EL EJE X Y LA SEGUNDA INDICA EL EJE Y
            estadoMando = posJoystick;
           
            //INICIAR EN EL SEGUNDO RENGL�N Y EN LA COLUMNA 1 PARA ESCRIBIR LO DEL EJE X
            lcd_gotoxy(1,2);
            //ESCRIBIR DIRECCI�N, SENTIDO Y MAGNITUD DE LA VELOCIDAD EN X
            if(estadoJoyX == -3){
               lcd_putc("-Xalta  ");
            }
            else if(estadoJoyX == -2){
               lcd_putc("-Xmedia ");
            }
            else if(estadoJoyX == -1){
               lcd_putc("-Xbaja  ");
            }
            else if(estadoJoyX == 0){
               lcd_putc("-X_NO   ");
            }
            else if(estadoJoyX == 1){
               lcd_putc("+Xbaja  ");
            }
            else if(estadoJoyX == 2){
               lcd_putc("+Xmedia ");
            }
            else if(estadoJoyX == 3){
               lcd_putc("+Xalta  ");
            }
            
            //INICIAR EN EL SEGUNDO RENGL�N Y EN LA COLUMNA 9 PARA ESCRIBIR LO DEL EJE Y
            lcd_gotoxy(9,2);
            //ESCRIBIR DIRECCI�N, SENTIDO Y MAGNITUD DE LA VELOCIDAD EN Y
            if(estadoJoyY == -3){
               lcd_putc("-Yalta ");
            }
            else if(estadoJoyY == -2){
               lcd_putc("-Ymedia");
            }
            else if(estadoJoyY == -1){
               lcd_putc("-Ybaja ");
            }
            else if(estadoJoyY == 0){
               lcd_putc("-Y_NO  ");
            }
            else if(estadoJoyY == 1){
               lcd_putc("+Ybaja ");
            }
            else if(estadoJoyY == 2){
               lcd_putc("+Ymedia");
            }
            else if(estadoJoyY == 3){
               lcd_putc("+Yalta ");
            }
            break;
      }
   }
}
