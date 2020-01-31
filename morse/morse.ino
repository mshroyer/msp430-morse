/*
   morse.ino - Morse code keyer for the TI MSP430 LaunchPad

   Mark Shroyer
   5 August 2013
*/

#define ENC_SZ 5
#define KEY_PIN 12
#define LED_PIN 13
#define CODE_DEBOUNCE_MILLIS 25
#define BUF_SZ 8
#define BUF_INT_SZ 16

#define MORSE_CHAR(ch, enc) ch enc

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

int recv_i = 0;
char buf_recv[BUF_SZ];
unsigned long last_key_millis;
boolean last_key_state = false;
unsigned long buf_int[BUF_INT_SZ];
int buf_int_i = 0;
unsigned long centroid[2] = { 100, 300 };

static unsigned long distance(unsigned long a, unsigned long b) {
  return abs(a - b);
}

int get_centroid(unsigned long len) {
  if (distance(len, centroid[1]) < distance(len, centroid[0])) {
    return 1;
  } else {
    return 0;
  }
}

char classify_interval(unsigned long len) {
  if ((centroid[1] < centroid[0]) != (get_centroid(len) == 0)) {
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

  centroid[0] = 100;
  centroid[1] = 300;
  for (i = 0; i < 100; i++) {
    for (j = 0; j < BUF_INT_SZ; j++) {
      k = get_centroid(buf_int[j]);
      centroid_sums[k] += buf_int[j];
      centroid_matches[k]++;
    }
    for (k = 0; k < 2; k++) {
      centroid[k] = centroid_sums[k] / centroid_matches[k];
    }
  }

  buf_int_i = (buf_int_i + 1) % BUF_INT_SZ;
}

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

char morse_decode_char(const char *enc) {
  int i;

  for (i = 0; i < sizeof(morse_table); i++) {
    if (strncmp(morse_table[i] + 1, enc, BUF_SZ) == 0) {
      return *morse_table[i];
    }
  }

  return '\0';
}

void morse_out(const char *enc) {
  boolean inverted = centroid[1] < centroid[0];
  unsigned long dit = inverted ? centroid[1] : centroid[0];
  unsigned long dah = inverted ? centroid[0] : centroid[1];

  while (enc != NULL) {
    switch (*(enc++)) {
      case '.':
        digitalWrite(LED_PIN, HIGH);
        delay(dit);
        digitalWrite(LED_PIN, LOW);
        delay(dit);
        break;

      case '-':
        digitalWrite(LED_PIN, HIGH);
        delay(dah);
        digitalWrite(LED_PIN, LOW);
        delay(dit);
        break;

      default:
        delay(dah);
        return;
    }
  }
}

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
    //Serial.write(ch);
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
  Serial.begin(9600);

  for (i = 0; i < BUF_INT_SZ; i++) {
    buf_int[i] = centroid[i % 2];
  }
}

void loop() {
  unsigned long now = millis();
  boolean key_state = !digitalRead(KEY_PIN);
  char ch;

  morse_in(key_state, now);
  if (!last_key_state && recv_i != 0
      && classify_interval(now - last_key_millis) == '-') {
    buf_recv[recv_i] = '\0';
    ch = morse_decode_char(buf_recv);
    if (ch) {
      Serial.write(ch);
    }
    recv_i = 0;
  }

  ch = Serial.read();
  if (ch >= 0) {
    morse_out(morse_encode_char(ch));
  }

  delay(5);
}
