//
// Client side code to read the potentiometer,
// show the values on a neopixel display
// and send the scaled value via RFM chip to a server that does more work


#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>
#include <SPIFlash.h> //get it here: https://www.github.com/lowpowerlab/spiflash

// Fuer den RFM chip
#define NODEID        2    //unique for each node on same network
#define GATEWAYID     1    // id of the server
#define NETWORKID     100  //the same on all nodes that talk to each other
#define FREQUENCY     RF69_868MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!


#define ANALOG_PIN  3  // potentiometer wiper (middle terminal) connected to analog pin 3
                       // outside leads to ground and +5V


#define NEO_PIN 5    // Pin where the Neopixel ring is on
#define NEO_PIXELS 12  // Number of pixels in the ring


#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

char buf[2];


RFM69 radio;
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network


Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEO_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);

int val = 0;           // variable to store the value read


void setup() {

  Serial.begin(9600);  
  delay(10);
  Serial.println("Start!");
   
  strip.begin();
  strip.setBrightness(25);
  strip.show(); // Initialize all pixels to 'off'
  leds_zeigen(NEO_PIXELS);
  delay(100);
  leds_zeigen(0);
  
  // INitialize the RFM 
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
 radio.setHighPower(); //only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);

  if (flash.initialize())
  {
    Serial.print("SPI Flash Init OK. Unique MAC = [");
    flash.readUniqueId();
    for (byte i=0;i<8;i++)
    {
      Serial.print(flash.UNIQUEID[i], HEX);
      if (i!=8) Serial.print(':');
    }
    Serial.println(']');
    
    //alternative way to read it:
    //byte* MAC = flash.readUniqueId();
    //for (byte i=0;i<8;i++)
    //{
    //  Serial.print(MAC[i], HEX);
    //  Serial.print(' ');
    //}
    
  }
  else
    Serial.println("SPI Flash Init FAIL! (is chip present?)");


  Serial.println("----- Initialisiert");

  radio.readAllRegs();
}

long lastPeriod = 0;
long lastVal = -1;

void loop() {
  
  
  int val2;  
  
  val = analogRead(ANALOG_PIN);    // read the input pin
  val2 = map(val,0,1023,0,NEO_PIXELS);

  // Serial.println(val2);   
  
  leds_zeigen(val2);
  
  
    if (radio.receiveDone())
  {
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    for (byte i = 0; i < radio.DATALEN; i++) {
      Serial.print((char)radio.DATA[i]);
    }
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");

    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent");
    }
    Blink(LED,3);
    Serial.println();
  }

  // We send values from 0..12
  val2 = map(val,0,1023,0,12);
  if (lastVal != val2) {
    buf[0] = char(val2 );
    if (radio.sendWithRetry(GATEWAYID, buf, 1) ) {
      Serial.print(" ok!");
    }
    else { 
      Serial.print(" nothing...");
    }
    delay(100);
  
    lastVal = val2;
  
  }
  
}


// Show 'wieviele' NeoPixels as on
void leds_zeigen(int wieviele) {
  for (uint16_t i=0; i < NEO_PIXELS ; i++) {
    if (i < wieviele) {
      strip.setPixelColor(i,strip.Color(255,   0,   0));
    } else {
      // We need to explicitly switch off the others.
      strip.setPixelColor(i,0);
    }
    strip.show();
  }
}

void Blink(byte PIN, int DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}
