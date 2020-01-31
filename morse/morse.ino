/*
 * morse.ino - Morse encoder and decoder
 *
 * Encodes and decodes morse code sequences on an ATmega32U4-based MCU board
 * like the Adafruit ItsyBitsy. Characters can be input over a serial line,
 * and the corresponding Morse code will be blinked out on an LED; Morse code
 * sequences entered on a pulled-up input pin will be converted to characters.
 * Those characters can be output over the serial port or over an emulated
 * USB HID keyboard.
 *
 * This is ported from code originally written to run on a TI MSP430
 * LaunchPad. It will probably run on most Arduino-compatible boards with
 * little modification, except that keyboard emulation relies on specific
 * functionality of the ATmega32U4.
 *
 * Mark Shroyer <mark@shroyer.name>
 * 30 January 2020
 */

// Enable input over serial to output to the LED.
//#define ENABLE_INPUT 1

// Output decoded characters to USB keyboard instead of serial.
#define ENABLE_KEYBOARD_OUTPUT 1

// Output centroid debugging info over serial.
//#define ENABLE_DEBUG_CENTROIDS 1

#define ENC_SZ 5
#define KEY_PIN 12
#define LED_PIN 13
#define CODE_DEBOUNCE_MILLIS 25
#define BUF_SZ 8
#define BUF_INT_SZ 16

#define MORSE_CHAR(ch, enc) ch enc
#define DIT centroid[0]
#define DAH centroid[1]

#ifdef ENABLE_KEYBOARD_OUTPUT
#include <Keyboard.h>
#endif

const char morse_table[][ENC_SZ + 2] = {
  MORSE_CHAR("A", ".-"),
  MORSE_CHAR("B", "-..."),
  MORSE_CHAR("C", "-.-."),
  MORSE_CHAR("D", "-.."),
  MORSE_CHAR("E", "."),
  MORSE_CHAR("F", "..-."),
  MORSE_CHAR("G", "--."),
  MORSE_CHAR("H", "...."),
  MORSE_CHAR("I", ".."),
  MORSE_CHAR("J", ".---"),
  MORSE_CHAR("K", "-.-"),
  MORSE_CHAR("L", ".-.."),
  MORSE_CHAR("M", "--"),
  MORSE_CHAR("N", "-."),
  MORSE_CHAR("O", "---"),
  MORSE_CHAR("P", ".--."),
  MORSE_CHAR("Q", "--.-"),
  MORSE_CHAR("R", ".-."),
  MORSE_CHAR("S", "..."),
  MORSE_CHAR("T", "-"),
  MORSE_CHAR("U", "..-"),
  MORSE_CHAR("V", "...-"),
  MORSE_CHAR("W", ".--"),
  MORSE_CHAR("X", "-..-"),
  MORSE_CHAR("Y", "-.--"),
  MORSE_CHAR("Z", "--.."),
  MORSE_CHAR("1", ".----"),
  MORSE_CHAR("2", "..---"),
  MORSE_CHAR("3", "...--"),
  MORSE_CHAR("4", "....-"),
  MORSE_CHAR("5", "....."),
  MORSE_CHAR("6", "-...."),
  MORSE_CHAR("7", "--..."),
  MORSE_CHAR("8", "---.."),
  MORSE_CHAR("9", "----."),
  MORSE_CHAR("0", "-----")
};

// Receive buffer storing dits and dahs.
int recv_i = 0;
char buf_recv[BUF_SZ];

// The last keying state and timestamp, set by morse_in.
unsigned long last_key_millis;
boolean last_key_state = false;

// Ring buffer of interval lengths for computing centroids.
unsigned long buf_int[BUF_INT_SZ];
int buf_int_i = 0;
unsigned long centroid[2] = { 100, 300 };

static unsigned long distance(unsigned long a, unsigned long b) {
  if (a > b) {
    return a - b;
  } else {
    return b - a;
  }
}

int get_centroid(unsigned long len) {
  if (distance(len, centroid[1]) < distance(len, centroid[0])) {
    return 1;
  } else {
    return 0;
  }
}

char classify_interval(unsigned long len) {
  if (get_centroid(len) == 0) {
    return '.';
  } else {
    return '-';
  }
}

void update_centroids(unsigned long len) {
  unsigned long centroid_sums[2] = { 0, 0 };
  int centroid_matches[2] = { 0, 0 };
  int i, j, k;

  buf_int[buf_int_i] = len;

  centroid[0] = 50;
  centroid[1] = 350;
  for (i = 0; i < 100; i++) {
    centroid_sums[0] = 0;
    centroid_sums[1] = 0;
    centroid_matches[0] = 0;
    centroid_matches[1] = 0;

    for (j = 0; j < BUF_INT_SZ; j++) {
      k = get_centroid(buf_int[j]);
      centroid_sums[k] += buf_int[j];
      centroid_matches[k]++;
    }
    for (k = 0; k < 2; k++) {
      if (centroid_matches[k] != 0) {
        centroid[k] = centroid_sums[k] / centroid_matches[k];
      }
    }
  }
  normalize_centroids();

#ifdef ENABLE_DEBUG_CENTROIDS
  Serial.write("len = ");
  Serial.print(len);
  Serial.write("; buf_int = [");
  for (j = 0; j < BUF_INT_SZ; j++) {
    Serial.print(buf_int[j]);
    if (j < BUF_INT_SZ - 1) {
      Serial.write(", ");
    }
  }
  Serial.write("]; centroid = (");
  Serial.print(centroid[0]);
  Serial.write(", ");
  Serial.print(centroid[1]);
  Serial.write(")\n");
#endif

  buf_int_i = (buf_int_i + 1) % BUF_INT_SZ;
}

void normalize_centroids() {
  unsigned long tmp;

  if (centroid[0] > centroid[1]) {
    tmp = centroid[0];
    centroid[0] = centroid[1];
    centroid[1] = tmp;
  }
}

char morse_decode_char(const char *enc) {
  int i;

  for (i = 0; i < sizeof(morse_table); i++) {
    if (strncmp(morse_table[i] + 1, enc, BUF_SZ) == 0) {
      return *morse_table[i];
    }
  }

  return '?';
}

#ifdef ENABLE_INPUT

const char *morse_encode_char(char ch) {
  int i;

  // Convert lower case to upper.
  if (ch >= 0x61 && ch <= 0x7a) {
    ch -= 0x20;
  }

  for (i = 0; i < sizeof(morse_table); i++) {
    if (*(morse_table[i]) == ch) {
      return morse_table[i] + 1;
    }
  }
  return NULL;
}

void morse_out(const char *enc) {
  while (enc != NULL) {
    switch (*(enc++)) {
      case '.':
        digitalWrite(LED_PIN, HIGH);
        delay(DIT);
        digitalWrite(LED_PIN, LOW);
        delay(DIT);
        break;

      case '-':
        digitalWrite(LED_PIN, HIGH);
        delay(DAH);
        digitalWrite(LED_PIN, LOW);
        delay(DIT);
        break;

      default:
        delay(DAH);
        return;
    }
  }
}

#endif  /* defined ENABLE_INPUT */

void morse_in(int key_state, unsigned long now) {
  unsigned long len;

  // Input is unchanged, nothing to do here.
  if (key_state == last_key_state) {
    return;
  }

  // Ignore key bounces.
  if (now - last_key_millis < CODE_DEBOUNCE_MILLIS) {
    return;
  }

  if (!key_state && recv_i < BUF_SZ) {
    len = now - last_key_millis;
    char ch = classify_interval(len);
    buf_recv[recv_i++] = ch;
    update_centroids(len);
  }

  last_key_state = key_state;
  last_key_millis = now;
}

void setup() {
  int i;

  buf_recv[0] = '\0';

  pinMode(LED_PIN, OUTPUT);
  pinMode(KEY_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);

#if defined ENABLE_INPUT \
      || !defined ENABLE_KEYBOARD_OUTPUT \
      || defined ENABLE_DEBUG_CENTROIDS
  Serial.begin(9600);
#endif

#ifdef ENABLE_KEYBOARD_OUTPUT
  Keyboard.begin();
#endif

  // Fill the interval buffer from default centroids.
  for (i = 0; i < BUF_INT_SZ; i++) {
    buf_int[i] = centroid[i % 2];
  }
}

void loop() {
  unsigned long now = millis();
  boolean key_state = !digitalRead(KEY_PIN);
  char ch;

  morse_in(key_state, now);
  if (!last_key_state && recv_i != 0 && now - last_key_millis >= DAH) {
    buf_recv[recv_i] = '\0';
    ch = morse_decode_char(buf_recv);
    if (ch) {
#ifdef ENABLE_KEYBOARD_OUTPUT
      Keyboard.write(ch);
#else
      Serial.write(ch);
#ifdef ENABLE_DEBUG_CENTROIDS
      Serial.write('\n');
#endif  /* ENABLE_DEBUG_CENTROIDS */
#endif  /* !defined ENABLE_KEYBOARD_OUTPUT */
    }
    recv_i = 0;
  }

#ifdef ENABLE_INPUT
  ch = Serial.read();
  if (ch >= 0) {
    morse_out(morse_encode_char(ch));
  }
#endif

  delay(5);
}
