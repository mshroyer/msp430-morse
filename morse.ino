/*
 * morse.ino - Morse code keyer for the TI MSP430 LaunchPad
 *
 * Mark Shroyer
 * 4 August 2012
 */
 
#define KEY_PIN PUSH2
#define LED_PIN GREEN_LED
#define CODE_UNIT 100
#define CODE_DEBOUNCE 10
#define BUF_SZ 8

#define MORSE_CHAR(ch, enc) ch enc

#define IS_LONG_KEY(duration) ( (duration) >= 2 * CODE_UNIT )

const char *morse_table[] = {
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
  MORSE_CHAR("Z", "--..") 
};

int recv_i = 0;
char buf_recv[BUF_SZ];
char buf_decode[BUF_SZ];
unsigned long last_key_millis;
int last_key_state = HIGH;

const char *morse_encode(char ch)
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

char morse_decode(const char *enc)
{
  int i;
  
  for ( i = 0; i < sizeof(morse_table); i++ ) {
    if ( strncmp(morse_table[i]+1, enc, BUF_SZ) == 0 )
      return *morse_table[i];
  }
      
  return '\0';
}

void morse_send(const char *enc)
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

int morse_recv(int key_state)
{
  unsigned long now = millis();
  
  /* Debounce */
  if ( now - last_key_millis < CODE_DEBOUNCE )
    return false;
    
  if ( key_state == last_key_state )
    return false;
    
  switch ( key_state ) {
  case LOW:
    if ( IS_LONG_KEY( now - last_key_millis ) ) {
      buf_recv[recv_i] = '\0';
      strncpy(buf_decode, buf_recv, BUF_SZ-1);
      recv_i = 0;
    }
    break;
    
  case HIGH:
    if ( IS_LONG_KEY( now - last_key_millis ) ) {
      buf_recv[recv_i++] = '-';
    } else {
      buf_recv[recv_i++] = '.';
    }
    /* TODO better overrun handling */
    if ( recv_i >= BUF_SZ )
      recv_i = 0;
    break;
  }
  
  last_key_state = key_state;
  last_key_millis = now;
  return true;
}

void key_isr_rising()
{
  if ( morse_recv(HIGH) )
    attachInterrupt(KEY_PIN, key_isr_falling, FALLING);
}

void key_isr_falling()
{
  if ( morse_recv(LOW) )
    attachInterrupt(KEY_PIN, key_isr_rising, RISING);
}

void setup()
{
  buf_recv[0] = '\0';
  buf_decode[0] = '\0';
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(KEY_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  attachInterrupt(KEY_PIN, key_isr_falling, FALLING);
  Serial.begin(9600);
}

void loop()
{
  char ch;
 
  ch = Serial.read();
  if ( ch >= 0 )
    morse_send(morse_encode(ch));

  if ( buf_decode[0] != '\0' ) {
    ch = morse_decode(buf_decode);
    if ( ch )
      Serial.write(ch);
    buf_decode[0] = '\0';
  }
}

