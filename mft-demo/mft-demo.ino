#include <M5Stack.h>
#include <SPI.h>

SPIClass *vspi = NULL;

#define SampleRate 22050
const float dt = 1.0 / float(SampleRate);

const float tone[128] = {
    0.12231220585508576, 0.11544734923405088, 0.10896778740921317, 0.102851895445316,
    0.09707926212145707, 0.09163062181053622, 0.08648779018201608, 0.08163360351340897,
    0.07705186140794874, 0.07272727272727272, 0.06864540455866863, 0.06479263404657011,
    0.06115610292754288, 0.05772367461702544, 0.05448389370460659, 0.051425947722658,
    0.04853963106072853, 0.04581531090526811, 0.04324389509100804, 0.040816801756704484,
    0.03852593070397437, 0.03636363636363636, 0.03432270227933431, 0.03239631702328507,
    0.03057805146377144, 0.02886183730851272, 0.027241946852303304, 0.025712973861329,
    0.024269815530364256, 0.022907655452634058, 0.02162194754550402, 0.020408400878352235,
    0.019262965351987186, 0.01818181818181818, 0.017161351139667155, 0.016198158511642535,
    0.01528902573188572, 0.01443091865425636, 0.013620973426151652, 0.0128564869306645,
    0.012134907765182128, 0.011453827726317029, 0.01081097377275201, 0.010204200439176117,
    0.009631482675993593, 0.00909090909090909, 0.008580675569833577, 0.008099079255821266,
    0.00764451286594286, 0.00721545932712818, 0.006810486713075825, 0.00642824346533225,
    0.006067453882591066, 0.005726913863158514, 0.005405486886376005, 0.00510210021958806,
    0.0048157413379967965, 0.004545454545454545, 0.004290337784916789, 0.004049539627910633,
    0.00382225643297143, 0.00360772966356409, 0.0034052433565379125, 0.003214121732666125,
    0.003033726941295533, 0.002863456931579257, 0.0027027434431880024, 0.00255105010979403,
    0.0024078706689983982, 0.0022727272727272726, 0.0021451688924583943, 0.0020247698139553164,
    0.001911128216485715, 0.001803864831782045, 0.0017026216782689563, 0.0016070608663330626,
    0.0015168634706477664, 0.0014317284657896286, 0.0013513717215940012, 0.001275525054897015,
    0.0012039353344991991, 0.0011363636363636363, 0.0010725844462291972, 0.0010123849069776582,
    0.0009555641082428575, 0.0009019324158910225, 0.0008513108391344781, 0.0008035304331665313,
    0.0007584317353238832, 0.0007158642328948143, 0.0006756858607970006, 0.0006377625274485074,
    0.0006019676672495996, 0.0005681818181818182, 0.0005362922231145986, 0.0005061924534888292,
    0.00047778205412142875, 0.00045096620794551125, 0.0004256554195672391, 0.00040176521658326564,
    0.0003792158676619415, 0.00035793211644740715, 0.0003378429303985003, 0.00031888126372425367,
    0.0003009838336247998, 0.0002840909090909091, 0.0002681461115572993, 0.0002530962267444146,
    0.00023889102706071437, 0.00022548310397275563, 0.00021282770978361956, 0.00020088260829163282,
    0.00018960793383097075, 0.00017896605822370358, 0.00016892146519925015, 0.00015944063186212683,
    0.0001504919168123999, 0.00014204545454545454, 0.00013407305577864967, 0.00012654811337220725,
    0.00011944551353035719, 0.00011274155198637781, 0.00010641385489180974, 0.00010044130414581641,
    9.480396691548542e-05, 8.948302911185177e-05, 8.446073259962508e-05, 7.972031593106344e-05};

uint8_t control[128];
//  1: stick upper(0: neutral, 127: full swing)
//  2: stick lower(0: neutral, 127: full swing)
// 64: sustain(0: off, 127: sustain)
uint8_t pitch = 64;

struct Params
{
  float attack;
  float decay;
  float sustainLevel;
  float sustainRate;
  float sustain;
  float release;
  float form[64];
};

struct Note
{
  uint8_t vel;
  bool top;
  float gain;
  float phase;
};

Params params;
struct Note notes[128];
float gains[128];
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

float env(struct Note *n);

void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);
  for (int i = 0; i < 128; i++)
  {
    gains[i] = env(&notes[i]);
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup()
{
  Serial.begin(115200);
  digitalWrite(5, HIGH);
  pinMode(5, OUTPUT);
  //initialise vspi with default pins
  //SCLK = 18, MISO = 19, MOSI = 23, SS = 5
  vspi = new SPIClass(VSPI);
  vspi->begin();

  M5.begin();
  M5.Lcd.print("Hello World M5Stack-MusicIF");

  params.attack = 5000. / SampleRate;
  params.decay = 1000. / SampleRate;
  params.sustainLevel = 0.7;
  params.sustainRate = 10.0;
  params.sustain = 100. / SampleRate;
  params.release = 500. / SampleRate;

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 10000, true);
  timerAlarmEnable(timer);
}

float env(struct Note *n)
{
  if (n->gain == 0.0 && n->vel == 0)
  {
    n->top = false;
    return 0.0;
  }
  if (n->vel > 0)
  {
    float topLevel = float(n->vel) / 127.0;
    if (!n->top)
    {
      n->gain += params.attack;
      if (n->gain > topLevel)
      {
        n->top = true;
        n->gain = topLevel;
      }
    }
    else
    {
      if (n->gain > params.sustainLevel * topLevel)
      {
        n->gain -= params.decay;
      }
      else
      {
        n->gain -= params.sustain;
      }
      if (n->gain < 0.0)
      {
        n->gain = 0.0;
      }
    }
  }
  else
  {
    float release = params.release / (1.0 + float(control[64]) * params.sustainRate / 127.0);
    n->top = false;
    n->gain -= release;
    if (n->gain < 0.0)
    {
      n->gain = 0.0;
    }
  }
  return n->gain;
}

uint8_t read()
{
  uint8_t v;
  vspi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  digitalWrite(5, LOW); //pull SS slow to prep other end for transfer
  v = vspi->transfer(0xFE);
  digitalWrite(5, HIGH); //pull ss high to signify end of data transfer
  vspi->endTransaction();
  delayMicroseconds(250);
  return v;
}

void loop()
{
  //portENTER_CRITICAL(&timerMux);
  for (int i = 0; i < 128; i++)
  {
    int h = int(gains[i] * 128);
    M5.Lcd.fillRect(i * 2, 10, 2, 128 - h, TFT_BLACK);
    M5.Lcd.fillRect(i * 2, 138 - h, 2, h, TFT_WHITE);
  }
  //portEXIT_CRITICAL(&timerMux);
  uint8_t note, num;
  uint8_t first = read();
  if (first & 0x80 == 0)
  {
    return;
  }
  int ch = first & 0xf;
  int cmd = first >> 4;
  switch (cmd)
  {
  case 0x9: // note-on
    note = read();
    if (note & 0x80)
    {
      return;
    }
    notes[note].vel = read() & 0x7f;
    break;
  case 0x8: // note-off
    note = read();
    if (note & 0x80)
    {
      return;
    }
    notes[note].vel = 0;
    read();
    break;
  case 0xb: // controll-change
    num = read();
    if (num & 0x80)
    {
      return;
    }
    control[num] = read() & 0x7f;
    M5.Lcd.setCursor(0, 160);
    M5.Lcd.printf("ctrl: %03d, %03d\n", num, control[num]);
    break;
  case 0xe: // pitch-bend
    num = read();
    pitch = read() & 0x7f;
  default:
    return;
  }
  //M5.update();
}
