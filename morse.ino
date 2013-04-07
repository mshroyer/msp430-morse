/*
 * morse.ino - Morse code keyer for the TI MSP430 LaunchPad
 *
 * Mark Shroyer
 * 6 April 2013
 */

#define ENC_SZ 5
#define KEY_PIN PUSH2
#define LED_PIN GREEN_LED
#define CODE_UNIT 100 /* ms */
#define CODE_DEBOUNCE 25 /* ms */
#define BUF_SZ 8

#define MORSE_CHAR(ch, enc) ch enc

#define IS_LONG_KEY(duration) ( (duration) >= 2 * CODE_UNIT )

const char morse_table[][ENC_SZ+2] = {
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

const char *morse_encode_char(char ch)
{
  int i;
  
  /* Convert lower case to upper */
  if ( ch >= 0x61 && ch <= 0x7a )
    ch -= 0x20;
  
  for ( i = 0; i < sizeof(morse_table); i++ ) {
    if ( *(morse_table[i]) == ch )
      return morse_table[i] + 1;
  }
  return NULL;
}

char morse_decode_char(const char *enc)
{
  int i;
  
  for ( i = 0; i < sizeof(morse_table); i++ ) {
    if ( strncmp(morse_table[i]+1, enc, BUF_SZ) == 0 )
      return *morse_table[i];
  }
      
  return '\0';
}

void morse_out(const char *enc)
{
  while ( enc != NULL ) {
    switch ( *(enc++) ) {
    case '.':
      digitalWrite(LED_PIN, HIGH);
      delay(CODE_UNIT);
      digitalWrite(LED_PIN, LOW);
      delay(CODE_UNIT);
      break;
      
    case '-':
      digitalWrite(LED_PIN, HIGH);
      delay(3 * CODE_UNIT);
      digitalWrite(LED_PIN, LOW);
      delay(CODE_UNIT);
      break;
      
    default:
      delay(3 * CODE_UNIT);
      return;
    }
  }
}

void morse_in(int key_state, unsigned long now)
{
  if ( key_state == last_key_state )
    return;

  if ( now - last_key_millis < CODE_DEBOUNCE )
    return;
    
  if ( ! key_state && recv_i < BUF_SZ ) {
    if ( IS_LONG_KEY(now - last_key_millis) )
      buf_recv[recv_i++] = '-';
    else
      buf_recv[recv_i++] = '.';
  }
  
  last_key_state = key_state;
  last_key_millis = now;
}

void setup()
{
  buf_recv[0] = '\0';
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(KEY_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  Serial.begin(9600);
}

void loop()
{
  unsigned long now = millis();
  boolean key_state = ! digitalRead(KEY_PIN);
  char ch;

  morse_in(key_state, now);
  
  if ( ! last_key_state && recv_i != 0 && IS_LONG_KEY(now - last_key_millis) ) {
    buf_recv[recv_i] = '\0';
    ch = morse_decode_char(buf_recv);
    if ( ch )
      Serial.write(ch);
    recv_i = 0;
  }
  
  ch = Serial.read();
  if ( ch >= 0 )
    morse_out(morse_encode_char(ch));
}

