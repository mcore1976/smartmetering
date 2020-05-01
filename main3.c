/* ---------------------------------------------------------------------------
 * IOT device based on ATTINY2313 / ATTINY2313A + SIM800L + DHT22
 * will send temperature and humidity reading over SMS message in response to SMS
 * by Adam Loboda - adam.loboda@wp.pl
 * baudrate 9600
 * please configure SIM800L to fixed 9600 first by AT+IPR=9600 command 
 * to ensure stability ans save config via AT&W command
 * because of sleepmode use on ATTINY2313 : INT0 pin (#6) of ATTINY2313/4313 
 * must be connected to RI/RING ping on SIM800L module
 * RING low voltage state initiates interrupt INT0 and wakes up ATTINY 
 * ---------------------------------------------------------------------------
 */

#include <inttypes.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>
#include <avr/power.h>

#define UART_NO_DATA 0x0100
// internal RC 8MHz oscillator with DIVISION by 8, gives 0.2% error rate for 9600 bps UART speed
// for 1Meg  : -U lfuse:w:0x64:m -U hfuse:w:0xdf:m 
#define F_CPU 1000000UL

#define BAUD 9600
// there is FCPU = 8MHz with division by 8 clock = 1MHz, and we are using U2X bit for better accuracy
// UBRR formula for U2X = 1
#define MYUBBR ((F_CPU / (BAUD * 8L)) - 1)


// DHT22 sensor settings

#define DHT_PIN            PB0

#define DHT_ERR_OK         (0)
#define DHT_ERR_TIMEOUT    (-1)

#define DHT_PIN_INPUT()    (DDRB &= ~_BV(DHT_PIN))
#define DHT_PIN_OUTPUT()   (DDRB |= _BV(DHT_PIN))
#define DHT_PIN_LOW()      (PORTB &= ~_BV(DHT_PIN))
#define DHT_PIN_HIGH()     (PORTB |= _BV(DHT_PIN))
#define DHT_PIN_READ()     (PINB & _BV(DHT_PIN))
#define DHT_TIMEOUT        (80)

// static text needed for SIM800L conversation

const char AT[] PROGMEM = { "AT\n\r" }; // wakeup from sleep mode
const char ISOK[] PROGMEM = { "OK" };
const char ISREG1[] PROGMEM = { "+CREG: 0,1" };  // registered in HPLMN
const char ISREG2[] PROGMEM = { "+CREG: 0,5" };  // registered in ROAMING NETWORK
const char SHOW_REGISTRATION[] PROGMEM = {"AT+CREG?\n\r"};
const char PIN_IS_READY[] PROGMEM = {"+CPIN: READY"};
const char PIN_MUST_BE_ENTERED[] PROGMEM = {"+CPIN: SIM PIN"};

const char SHOW_PIN[] PROGMEM = {"AT+CPIN?\n\r"};
const char ECHO_OFF[] PROGMEM = {"ATE0\n\r"};
const char ENTER_PIN[] PROGMEM = {"AT+CPIN=\"1111\"\n\r"};
const char CFGRIPIN[] PROGMEM = {"AT+CFGRI=1\n\r"};
const char HANGUP[] PROGMEM = {"ATH\n\r"};
const char SMS1[] PROGMEM = {"AT+CMGF=1\r\n"};
const char SMS2[] PROGMEM = {"AT+CMGS=\""};                    // for other networks if they show +XX in CLIP
const char DELSMS[] PROGMEM = {"AT+CMGDA=\"DEL ALL\"\r\n"};    // delete all stored SMS just in case
const char SHOWSMS[] PROGMEM = {"AT+CNMI=1,2,0,0,0\r\n"};      // display automatically SMS when arrives
const char ISSMS[] PROGMEM = {"CMT:"};                         // beginning of mobile terminated SMS identification

const char CRLF[] PROGMEM = {"\"\n\r"};

// Flightmode ON OFF - for GSM nework issues
const char FLIGHTON[] PROGMEM = { "AT+CFUN=4\r\n" };
const char FLIGHTOFF[] PROGMEM = { "AT+CFUN=1\r\n" };

// Sleepmode ON OFF
const char SLEEPON[] PROGMEM = { "AT+CSCLK=2\r\n" };
const char SLEEPOFF[] PROGMEM = { "AT+CSCLK=0\r\n" };

// Fix UART speed to 9600 bps
const char SET9600[] PROGMEM = { "AT+IPR=9600\r\n" };

// Save settings to SIM800L
const char SAVECNF[] PROGMEM = { "AT&W\r\n" };

// for sending SMS predefined text 
const char TEMPERATURESMS[] PROGMEM = {" Temperature : "};
const char HUMIDITYSMS[] PROGMEM = {" Humidity : "};


#define BUFFER_SIZE 40
// buffers for number of phone, responses from modem
volatile static uint8_t response[BUFFER_SIZE] = "1234567890123456789012345678901234567890";
volatile static uint8_t response_pos = 0;
volatile static uint8_t dhttxt[6] = "00000\x00";
volatile static uint8_t phonenumber[15] = "123456789012345";
volatile static uint8_t phonenumber_pos = 0;
volatile static uint8_t buf[20];  // buffer to copy string from PROGMEM


// -------------------------------------------------------------------------------------------------------
// ---------------------------------------- DHT22  library CODE ------------------------------------------
// -------------------------------------------------------------------------------------------------------


void  dht_init(void)
{
   DHT_PIN_INPUT();
   DHT_PIN_HIGH();
}

static int8_t   dht_await_state(uint8_t state)
{
    uint8_t counter = 0;
    while ((!DHT_PIN_READ() == state) && (++counter < DHT_TIMEOUT)) 
     { 
     // Delay 1 cycles
     // 1us at 1.0 MHz CLOCK
     asm volatile (
        "    nop"	"\n"
        );
     };
    return counter;
}


int8_t dht_read(uint8_t *temperature_hi, uint8_t *temperature_lo, uint8_t *humidity_hi, uint8_t *humidity_lo)
{
    uint8_t i, j, data[5] = {0, 0, 0, 0, 0};

    // send start sequence 1 LOW pulse for 2 miliseconds
    DHT_PIN_OUTPUT();
    DHT_PIN_LOW();

    // Delay 20 000 cycles
    // 20ms at 1 MHz
    asm volatile (
         "    ldi  r18, 26"	"\n"
         "    ldi  r19, 249"	"\n"
         "1:  dec  r19"	"\n"
         "    brne 1b"	"\n"
         "    dec  r18"	"\n"
         "    brne 1b"	"\n"
          );

    DHT_PIN_HIGH();
    DHT_PIN_INPUT();

    // read response sequence - 1 pulse  as preparation before sending measurement
    if (dht_await_state(0) < 0) return DHT_ERR_TIMEOUT;
    if (dht_await_state(1) < 0) return DHT_ERR_TIMEOUT;
    if (dht_await_state(0) < 0) return DHT_ERR_TIMEOUT;
    

    // read data - 40 bits - HOGH pulse length goves info aboit 0s and 1s 
    for (i = 0; i < 5; ++i) {
        for (j = 0; j < 8; ++j) 
          {
         // wait for next BIT to come
         dht_await_state(1);
         // do left shift
         data[i] <<= 1;
         // received BIT is "1" if waiting for LOW longer than 28 microseconds - 
         // the value = 1 in comparision is experimental for 1MHz clock, 20 for 8MHz
         if (dht_await_state(0) > 1)  data[i] |= 1;   
          };
    };

    // check if checksum matches
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF) ) 
     { 
    *temperature_hi = data[2];
    *temperature_lo = data[3];
    *humidity_hi = data[0];
    *humidity_lo = data[1];

    }
    else
    {
    *temperature_hi = 0;
    *temperature_lo = 0;
    *humidity_hi = 0;
    *humidity_lo = 0;

    }; 

     
    return DHT_ERR_OK;
}


// *********************************************************************************************************
// init_uart
// *********************************************************************************************************
void init_uart(void) {
  // set U2X bit so we can get 9600baud with lowest error rate 0.2%
  UCSRA = (1 << U2X);
  // set baud rate
  UBRRH = (uint8_t)(MYUBBR >> 8); 
  UBRRL = (uint8_t)(MYUBBR);
  // enable receive and transmit and NO INTERRUPT
  UCSRB = (1 << RXEN) | (1 << TXEN) ;
  // set frame format
  UCSRC = (0 << USBS) | (3 << UCSZ0); // asynchron 8n1
  // UCSRC = (1 << USBS) | (3 << UCSZ0);
  }


// *********************************************************************************************************
// send_uart
// Sends a single char to UART without ISR
// *********************************************************************************************************
void send_uart(uint8_t c) {
  // wait for empty data register
  while (!(UCSRA & (1<<UDRE)));
  // set data into data register
  UDR = c;
}


// *********************************************************************************************************
// receive_uart
// Receives a single char without ISR
// *********************************************************************************************************
uint8_t receive_uart() {
  while ( !(UCSRA & (1<<RXC)) )  ; 
  return UDR; 
}


// *********************************************************************************************************
// function to search RX buffer for response  SUB IN RX_BUFFER STR
// *********************************************************************************************************
uint8_t is_in_rx_buffer(char *str, char *sub) {
   uint8_t i, j=0, k;
    for(i=0; i<BUFFER_SIZE; i++)
    {
      if(str[i] == sub[j])
     {
       for(k=i, j=0; str[k] && sub[j]; j++, k++)  // if NOT NULL on each of strings
            if(str[k]!=sub[j])   break; // if different - start comparision with next char
       // if(!sub[j]) return 1; 
        if(j == strlen(sub)) return 1;  // full substring has been found        
      }
     }
     // substring not found
    return 0;
}

// *********************************************************************************************************
// uart_puts
// Sends a string.
// *********************************************************************************************************
void uart_puts(const char *s) {
  while (*s) {
    send_uart(*s);
    s++;
  }
}


// *********************************************************************************************************
// uart_puts_P
// Sends a PROGMEM string.
// *********************************************************************************************************
void uart_puts_P(const char *s) {
  while (pgm_read_byte(s) != 0x00) {
    send_uart(pgm_read_byte(s++));
  }
}


// *********************************************************************************************************
// READLINE from serial port that starts with CRLF and ends with CRLF and put to 'response' buffer what read
// *********************************************************************************************************
uint8_t readline()
{
  uint16_t char1, i , wholeline ;
  // wait for first CR-LF or exit after timeout i cycles
   i = 0;
   wholeline = 0;
   response_pos = 0;
  //
   do {
      // read chars in pairs to find combination CR LF
      char1 = receive_uart();
      // if CR-LF combination detected start to copy the response
      if   (  char1 != 0x0a && char1 != 0x0d && response_pos < BUFFER_SIZE ) 
         { response[response_pos] = char1; 
           response_pos++;
         };

      if    (  char1 == 0x0a || char1 == 0x0d )              
         {  
           // if the line was received and this is only CR/LF ending :
           if (response_pos > 0) // this is EoL
               { response[response_pos] = NULL;
                 response_pos = 0;
                  wholeline = 1;  };
          // just skip this CRLF character and wait for valuable one
               
         };
      // if buffer is empty exit from function there is nothing to read from
      i++;
      } while ( wholeline == 0);

return 1;
}



// ----------------------------------------------------------------------------------------------------------------------------
// read SMS message PHONE NUMBER from +CMT: output and response buffer and copy it to buffer 'phonenumber' for SMS sending
// ----------------------------------------------------------------------------------------------------------------------------
uint8_t readsmsphonenumber()
{
  // need 8bit ascii 
  uint8_t char1;
  uint8_t i;

  // 'i' is a safe fuse not to get out of response buffer
  i = 0;
  phonenumber_pos = 0;

  // reading from RESPONSE buffer of modem output, rewind to beginning of buffer
  response_pos = 0;

  // wait for "+CMT:" and space
      do { 
           char1 = response[response_pos];
           response_pos++;
           i++;
         } while ( (char1 != ':') && (i < BUFFER_SIZE) );

      // wait for first quotation sign - there will be MSISDN number of sender
      do { 
           char1 = response[response_pos];
           response_pos++;
           i++;
         } while ( (char1 != '\"') && (i < BUFFER_SIZE) );
      // if quotation detected start to copy the response - phonenumber 
      do  { 
           char1 = response[response_pos];
           response_pos++;
           phonenumber[phonenumber_pos] = char1; 
           phonenumber_pos++;
           i++;
         } while ( (char1 != '\"') && (i < BUFFER_SIZE) );    // until end of quotation
     // put NULL to end the string phonenumber
           phonenumber[phonenumber_pos-1] = NULL; 
           phonenumber_pos=0;
           response_pos = 0;

 return (1);
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// delay procedure ASM based because _delay_ms() is working bad for 8 MHz clock MCU
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

// delay partucular number of seconds 

void delay_sec(uint8_t i)
{
while(i > 0)
{
// Generated by delay loop calculator
// at http://www.bretmulvey.com/avrdelay.html
//
// Delay 1 000 000 cycles
// 1s at 1 MHz

asm volatile (
    "    ldi  r18, 6"	"\n"
    "    ldi  r19, 19"	"\n"
    "    ldi  r20, 174"	"\n"
    "1:  dec  r20"	"\n"
    "    brne 1b"	"\n"
    "    dec  r19"	"\n"
    "    brne 1b"	"\n"
    "    dec  r18"	"\n"
    "    brne 1b"	"\n"
    "    rjmp 1f"	"\n"
    "1:"	"\n"
);

i--;  // decrease another second

};    // repeat until i not zero

}


//////////////////////////////////////////
// SIM800L initialization procedures
//////////////////////////////////////////

// *********************************************************************************************************
// wait for first AT in case SIM800L is starting up
// *********************************************************************************************************
uint8_t checkat()
{
  uint8_t initialized2;

// wait for first OK while sending AT - autosensing speed on SIM800L, but we are working 9600 bps
// SIM 800L can be set by AT+IPR=9600  to fix this speed
// which I do recommend by connecting SIM800L to PC using putty and FTD232 cable

                 initialized2 = 0;
              do { 
               uart_puts_P(AT);
                if (readline()>0)
                   {
                    memcpy_P(buf, ISOK, sizeof(ISOK));                     
                   if (is_in_rx_buffer(response, buf) == 1)  initialized2 = 1;                  
                   };
               delay_sec(1);
               } while (initialized2 == 0);

        // send ECHO OFF
              uart_puts_P(ECHO_OFF);

             return initialized2;
}

// *********************************************************************************************************
// check if PIN is needed and enter PIN 1111 
// *********************************************************************************************************
uint8_t checkpin()
{

  uint8_t initialized2;
     // readline and wait for PIN CODE STATUS if needed send PIN 1111 to SIM card if required
                  initialized2 = 0;
              do { 
		delay_sec(2);
               uart_puts_P(SHOW_PIN);
                if (readline()>0)
                   {
                    memcpy_P(buf, PIN_IS_READY, sizeof(PIN_IS_READY));
                  if (is_in_rx_buffer(response, buf ) == 1)       initialized2 = 1;                                         
                    memcpy_P(buf, PIN_MUST_BE_ENTERED, sizeof(PIN_MUST_BE_ENTERED));
                  if (is_in_rx_buffer(response, buf) == 1)     
                        {  uart_puts_P(ENTER_PIN);   // ENTER PIN 1111
                           delay_sec(1);
                        };                  
                    };
                  
              } while (initialized2 == 0);
   return initialized2;
}


// *********************************************************************************************************
// check if registered to the network
// *********************************************************************************************************
uint8_t checkregistration()
{
  uint8_t initialized2, nbrminutes;
     // readline and wait for STATUS NETWORK REGISTRATION from SIM800L
     // first 2 networks preferred from SIM list are OK
                  initialized2 = 0;
                  nbrminutes = 0;
              do { 
                   delay_sec(3);
                 // check now if registered
                   uart_puts_P(SHOW_REGISTRATION);
                if (readline()>0)
                   {			   
                    memcpy_P(buf, ISREG1, sizeof(ISREG1));
                   if (is_in_rx_buffer(response, buf) == 1)  initialized2 = 1; 
                    memcpy_P(buf, ISREG2, sizeof(ISREG2));
                   if (is_in_rx_buffer(response, buf) == 1)  initialized2 = 1; 
                   }
                // if not registered or something wrong turn off RADIO for some time (battery) and turn it on again
                // this is not to drain battery in underground garage 
                else
                   {
                   delay_sec(1);
                   uart_puts_P(FLIGHTON);    // enable airplane mode - turn off radio for 3 minutes
                   delay_sec(60);                      
                   uart_puts_P(FLIGHTOFF);   // disable airplane mode - turn on radio and start to search for networks
                   delay_sec(60);                      
                 // give reasonable time to search for GSM network - 3 minutes is sufficient
                   };
                // end of DO loop
                } while (initialized2 == 0);

      return initialized2;
};
 


//////////////////////////////////////////////////////////////////
// POWER SAVING mode handling to reduce the battery consumption
//////////////////////////////////////////////////////////////////

void sleepnow(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    sleep_enable();

    MCUCR &= ~(_BV(ISC01) | _BV(ISC00));      //INT0 on low level - INT0 must be connected to RING pin on SIM800L
    GIMSK |= _BV(INT0);                       //enable INT0

    cli();                                    //stop interrupts to ensure the BOD timed sequence executes as required

    sei();                                    //ensure interrupts enabled so we can wake up again

    sleep_cpu();                              //go to sleep

    sleep_disable();                          //wake up here
}

// when interrupt from INT0 disable next interrupts from RING pin of SIM800L ( incoming SMS or URC) 
ISR(INT0_vect)
{
    GIMSK = 0;                     //disable external interrupts (only need one to wake up)
}



// *********************************************************************************************************
//
//                                                    MAIN PROGRAM
//
// *********************************************************************************************************

int main(void) {

  uint8_t initialized;

  uint8_t belowzero;
  uint8_t temperature_hi, temperature_lo, humidity_hi, humidity_lo;
  uint16_t humidity = 0;
  uint16_t temperature = 0;
  uint16_t temporary;

  //char buf[20];  // buffer to copy string from PROGMEM

 
  // initialize 9600 baud 8N1 RS232
  init_uart();

  // delay 10 seconds for safe SIM800L startup and network registration
  delay_sec(10);

          
       // try to communicate with SIM800L over AT
        initialized = checkat();
	delay_sec(2);

       // Fix UART speed to 9600 bps to disable autosensing
        uart_puts_P(SET9600); 
	delay_sec(2);

       // configure RI PIN activity for URC ( unsolicited messages like restart of the modem or battery low)
        uart_puts_P(CFGRIPIN);
        delay_sec(2);


       // Save settings to SIM800L
        uart_puts_P(SAVECNF);
        delay_sec(3);

       // check pin status, registration status
        checkpin();
        checkregistration();
        delay_sec(2);
 
     // neverending LOOP

       while (1) {

             do { 

                // delete all SMSes and SMS confirmation to keep SIM800L memory empty   
                   uart_puts_P(SMS1);
                   delay_sec(2); 
                   uart_puts_P(DELSMS);
                   delay_sec(2);
                  // configure to display immediately content of SMS
                   uart_puts_P(SHOWSMS);
                   delay_sec(2);

                // WAIT FOR RING message - incoming voice call and send SMS or restart RADIO module if no signal
                   initialized = 0;

               // enter SLEEP MODE of SIM800L for power saving ( will be interrupted by incoming voice call or SMS ) 
                   uart_puts_P(SLEEPON); 
                   delay_sec(2);
     
               // enter SLEEP MODE on ATTINY2313 for power saving
                   sleepnow(); // sleep function called here 

               // THERE WAS RI / INT0 INTERRUPT AND SOMETHING WAS SEND OVER SERIAL WE NEED TO GET OFF SLEEPMODE AND READ SERIAL PORT
                if (readline()>0)
                   {
                   // check if this is an SMS message first
                    memcpy_P(buf, ISSMS, sizeof(ISSMS));  
                    if  ( is_in_rx_buffer(response, buf) == 1 )  
                       { 
                         // we need to extract phone number from SMS message RESPONSE buffer
                         readsmsphonenumber(); 
                         // clear the flags first
                         initialized = 0;
                         // disable SLEEPMODE  and proceed with sending SMS                  
                         uart_puts_P(AT);
                         delay_sec(1);
                         uart_puts_P(SLEEPOFF);
                         delay_sec(1);
                        // there was SMS received so we need to set appropriate flag 
                         initialized = 1;
                        } // end of IF

                     // if some other message than RING check if network is avaialble and SIM800L is operational  
                     else 
                      {
                      // disable SLEEPMODE                  
                       uart_puts_P(AT);
                       delay_sec(1);
                       uart_puts_P(SLEEPOFF);
                       delay_sec(1);
                      // check status of all functions 
                       checkpin();
                       checkregistration();
                       delay_sec(1);
                    // there was something different than SMS so we need to go back to the beginning 
                       initialized = 0;
                      }; // end of ELSE

                    }; // END of READLINE IF
                 
                } while ( initialized == 0);    // end od DO-WHILE, go to begging and enter SLEEPMODE again 

           // if SMS was received we need to send response 
           if (initialized == 1)
           {

               // initialize DHT22 temperature & humidity sensor, we will get reading after 4 seconds
               dht_init();
               delay_sec(2);

               // send SMS preamble
               uart_puts_P(SMS1);
               delay_sec(1); 
               // compose an SMS from fragments - interactive mode CTRL Z at the end
               uart_puts_P(SMS2);
               uart_puts(phonenumber);  // send phone number received from SMS
               uart_puts_P(CRLF);   			   
               delay_sec(1); 

               // Read data from DHT22 sensor
               // read value from DHT22 sensor, humidity amd temperature are encoded on 16 bits each
               dht_read(&temperature_hi, &temperature_lo,  &humidity_hi, &humidity_lo);
               // Reading correction - check if most significant bit 15 is 1 from DHT 22 temperature reading
               // if so - temperature is below zero Celsius Degrees
               if (temperature_hi > 127)  
                    { // put 'minus' sign at the end instead of degrees, and clear first significant bit of temperature
                      temperature_hi = temperature_hi - 128;
                      belowzero = 45;  // MINUS character
                    } 
               else  
                    {
                      // do not change temperature reading, put 'space' instead of minus sign
                      belowzero = 32;  // SPACE character
                    };

               // calculate 16bit temperature ( 10 times real temperature value)   
               temperature = ( temperature_hi * 256 ) + temperature_lo;
               // calculate 16bit Humidity ( 10 times real humidity value)   
               humidity = ( humidity_hi * 256 ) + humidity_lo;

               // send details about sensor status

              // calculate 3 digits for temperature
              uart_puts_P(TEMPERATURESMS); // send info
              dhttxt[0] = belowzero;
              dhttxt[1] = (temperature / 100) + 48;  // calculate ASCII code for digits
              temporary = temperature % 100; 
              dhttxt[2] = (temporary / 10) + 48;
              dhttxt[3] = 46 ;  // the DOT character
              dhttxt[4] = (temporary % 10) + 48; 

              uart_puts(dhttxt);   // send DHT22 temperature readings

              // calculate 3 digits for humidity
              uart_puts_P(HUMIDITYSMS); // send info
              dhttxt[0] = 32;  // empty 'space'
              dhttxt[1] = (humidity / 100) + 48;
              temporary = humidity % 100; 
              dhttxt[2] = (temporary / 10) + 48;
              dhttxt[3] = 46 ;   // the DOT character
              dhttxt[4] = (temporary % 10) + 48; 

              uart_puts(dhttxt);   // send DHT22 temperature readings

              // send SMS end sequence
              delay_sec(1); 
              send_uart(26);   // ctrl Z to end SMS
              initialized = 0;
          } /// end of commands when GPRS is working

       
        delay_sec(5);
        // go to the beginning and enter sleepmode on SIM800L and ATTINY2313 again for power saving

        // end of neverending loop
        };

 
    // end of MAIN code 
}

