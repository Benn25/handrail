#include <FastLED.h>

#define SEUIL 40 //light treshold for the IR receivers
#define SPEED_GAP 7 //gap to sense the speed of the detected body
#define LED_PIN     13
#define NUM_LEDS    158
#define LED_TYPE    APA102
#define COLOR_ORDER BGR
#define CLOCK_PIN 8

#define BRIGHTNESS  240
#define FRAMES_PER_SECOND 800

CRGB leds[NUM_LEDS];

CRGBPalette16 gPal;

byte controlPins[] = {B11110000,
                      B11100000,
                      B11010000,
                      B11000000,
                      B10110000,
                      B10100000,
                      B10010000,
                      B10000000,
                      B01110000,
                      B01100000,
                      B01010000,
                      B01000000,
                      B00110000,
                      B00100000,
                      B00010000,
                      B00000000
                     };

// holds incoming values from 74HC4067
const int numetal = 10; //values to keep for average the etalon
int indexetal = 0;
long totaletal[16];
short etalinit[] = {520, 690, 670, 685, 762, 684, 654, 771, 720, 621, 663, 671, 739, 725, 716, 900};
short etal[16][numetal];
short muxValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte chain[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //position of the LED, 0 or 1
byte pastVal[SPEED_GAP];
int val; //value of raw position (total chain value addition)
int LEDpos = 0; //targeted LED to enlight
int counter; //number of activated sensors
int loopCount = 0;
int SpeedFactor = 0;
int accel = 1;
short Spread = NUM_LEDS / 38; //size of the light zone
int oldaverageLEDpos = 0;
int up = 0;
int down = 0;


/*average maker*/
const int numReadings = 4;
int readingsLED[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int totalLED = 0;                  // the running total
int averageLEDpos = 0;                // the average
const int numReadingsSPEED = 110;
int readingsSPEED[numReadingsSPEED];      // the readings from the analog input
int readIndexSPEED = 0;              // the index of the current reading
int totalSPEED = 0;                  // the running total
int averageSPEED = 0;                // the average

int total[16];
const int numReadSensor = 3;
int index = 0;
int MuxValTot[16][numReadSensor];
int average[16];
int etalaverage[16];

/*---------------*/

unsigned long reseter = 0 ; //for debug
int incr = 0; //debug
int posPot; //debug

void setup() {
  DDRD = B11111111; // set PORTD (digital 7~0) to outputs
  

  delay( 1000 ); // power-up safety delay
  //Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );

  gPal = CRGBPalette16( CHSV (150, 120, 190), CRGB::White, CRGB::Yellow, CHSV (0, 150, 255));


//  for (int a = 0; a < 16; a++) {
//    for (int n = 0; n < numetal; n++) {
//      etal[a][numetal] = etalinit[a];
//    }
//  }

  /*
  for(int i = 0; i < NUM_LEDS; i++) {
  	// Set the i'th led to red
  	leds[i] = CHSV (i, 200,230);
  	// Show the leds
  	FastLED.show();
  	// now that we've shown the leds, reset the i'th led to black
  	leds[i] = CRGB::Black;
  	// Wait a little bit before we loop around and do it again
  	//delay(1);
  }
  for(int i = NUM_LEDS; i > 0; i--) {
  	// Set the i'th led to red
  	leds[i] = CHSV (i, 200,230);
  	// Show the leds
  	FastLED.show();
  	// now that we've shown the leds, reset the i'th led to black
  	leds[i] = CRGB::Black;
  	// Wait a little bit before we loop around and do it again
  	//delay(1);
  }
  */
  for (int b = 0; b < 250; b++) {
    for (int a = 0; a < NUM_LEDS; a++) {
      leds[a] = CHSV (b, 254 - b, b);
    }
    delay(1);
    FastLED.show();
  }
}

void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8_video(230 + constrain(map(averageSPEED, 0, 105, 0, 25), 0, 25));
    //  for (int i = 0; i < NUM_LEDS; i++) {
    //    leds[i].fadeLightBy(100);
  }
}

void loop() {

  //delay(250);
  for (int i = 0; i < 16; i++) {
    PORTD = controlPins[i];
    delayMicroseconds(500);
    muxValues[i] = analogRead(7);
  }

  //average values//

  for (int v = 0; v < 16; v++) {
    total[v] -= MuxValTot[v][index];
    MuxValTot[v][index] = muxValues[v];
    total[v] += MuxValTot[v][index];
    average[v] = total[v] / numReadSensor;
  }
  index++;
  if (index >= numReadSensor)
    index = 0;

  for (int e = 0; e < 16; e++) {
    totaletal[e] -= etal[e][indexetal];
    etal[e][indexetal] = average[e];
    totaletal[e] += etal[e][indexetal];
    etalaverage[e] = totaletal[e] / numetal;
  }
  if ((millis() % 1000) < 50 && (millis() % 1000) >=0 )
    indexetal++;
  if (indexetal >= numetal)
    indexetal = 0;
  //Serial.println (indexetal);
  //end average values//


  for (int p = 0; p < 16; p++) {
    if (average[p] < etalaverage[p] - SEUIL) {
      chain[p] = 1;
    }
    else chain[p] = 0;
  }

 // for (int b = 0; b < 16; b++) {

//    Serial.print(average[b]);
//    Serial.print(", ");
//  }
//  Serial.println("");

  /*----tester----
  for (int a = 0; a < 16; a++) {
    chain[a] = 0;
  }
  posPot = map(analogRead(1), 0, 1023, 0, 15);
  chain[posPot] = 1;
  /*
  if(millis()>=reseter+5){
  for(int a=0; a<16; a++){
    chain[a]=0;}


  chain[incr]=1;
  if (incr < 12)
  chain[incr+1]=1;*/
  //  for (int b = 0; b < 16; b++) {
  //
  //    Serial.print(chain[b]);
  //    Serial.print(", ");
  //  }

  //reseter=millis();

  /*----tester end----*/

  for (int a = 0; a < 16; a++) {
    if (chain[a] == 1) {
      val += (NUM_LEDS * a) / 16;
      counter ++;
    }
  }
  if (chain[0] == 1)
    val += 1;

  if (counter != 0) {
    //counter = 1; //to avoid the division by 0 oh shi..
    LEDpos = val / counter;
  }


  /*average LEDpos*/

  // subtract the last reading:
  totalLED = totalLED - readingsLED[readIndex];
  // read from the sensor:
  readingsLED[readIndex] = LEDpos;
  // add the reading to the total:
  totalLED = totalLED + readingsLED[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;
  // if we're at the end of the array...
  if (readIndex >= numReadings)
    // ...wrap around to the beginning:
    readIndex = 0;

  averageLEDpos = constrain(totalLED / numReadings, oldaverageLEDpos - 10, oldaverageLEDpos + 10) ;
  /*
   if(averageLEDpos+1>oldaverageLEDpos){
     up =0;
     down ++;
     }
  if(averageLEDpos-1<oldaverageLEDpos){
     down =0;
     up ++;
     }

     if (down>4){
       averageLEDpos+=10;
  }

    if (up>4){
       averageLEDpos-=10;
       }
       constrain(averageLEDpos,0,NUM_LEDS);

  Serial.print("up= ");
  Serial.print(up);
  Serial.print(" | down= ");
  Serial.println(down);
  */
  //Serial.println(averageLEDpos);
  /*average SPEED*/

  // subtract the last reading:
  totalSPEED = totalSPEED - readingsSPEED[readIndexSPEED];
  // read from the sensor:
  readingsSPEED[readIndexSPEED] = SpeedFactor * accel;
  // add the reading to the total:
  totalSPEED = totalSPEED + readingsSPEED[readIndexSPEED];
  // advance to the next position in the array:
  readIndexSPEED = readIndexSPEED + 1;
  // if we're at the end of the array...
  if (readIndexSPEED >= numReadingsSPEED)
    // ...wrap around to the beginning:
    readIndexSPEED = 0;

  averageSPEED = totalSPEED / numReadingsSPEED;


  // Serial.println(averageLEDpos);
  /* Serial.print("  speedfactor = ");
   Serial.println(SpeedFactor);
   Serial.print("      AVERAGE speedfactor = ");
   Serial.println(averageSPEED);
   Serial.println("=========================");
  */
  //if(counter==1)
  //counter = 2;

  for (int ONLED = averageLEDpos - Spread; ONLED <= averageLEDpos + Spread; ONLED++) {
    int decal = abs(ONLED - averageLEDpos);
    //leds[ONLED + Spread+(counter-2)*2] = CHSV(140+averageSPEED, 100+averageSPEED, 255);
    leds[ONLED + Spread] = ColorFromPalette( gPal, constrain(averageSPEED * 2.5, 0, 240) );
  }

  fadeall();

  FastLED.show();
  FastLED.delay(100 / FRAMES_PER_SECOND);




  /*================END PART=================*/

  oldaverageLEDpos = averageLEDpos;
  counter = 0;
  val = 0;
  //  incr += 1;
  //  if (incr == 16) {
  //    incr = 0;
  //  }

  if (loopCount < SPEED_GAP)
    pastVal[loopCount] = averageLEDpos;

  if (loopCount >= SPEED_GAP - 1) {
    loopCount = -1;
  }

  int oldSpeed = SpeedFactor;

  if ((abs(pastVal[loopCount + 1] - averageLEDpos)) > 0) { //body is moving
    SpeedFactor = abs(pastVal[loopCount + 1] - averageLEDpos);
    accel = 9 ;
  }
  else {
    SpeedFactor /= 1.3;
    accel = 1;
  }

  loopCount++;
  //  Serial.print("pastval = ");
  //  Serial.println(pastVal[loopCount]);
}
