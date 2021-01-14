#include "decode433.h"

static const char* bin2tristate(const char* bin);
static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength);

bool decode433_hema(uint32_t dec, int bitLength, String& dip, String& button, String& onoff) {
  static char stringbuf[6];
  if (bitLength != 24) return false;
  int i;
  bool ok = true;
  unsigned int dipbits = (dec >> 14) & 0x3ff;
  
  // Decode dip switches
  for(i=4; i>= 0; i--) {
    switch(dipbits & 0x3) {
      case 0: stringbuf[i] = '1'; break;
      case 1: stringbuf[i] = '0'; break;
      case 2: stringbuf[i] = 'e'; ok = false; break;
      case 3: stringbuf[i] = 'f'; ok = false; break;
    }
    dipbits >>= 2;
  }
  stringbuf[5] = '\0';
  dip = String(stringbuf);
  
  // Decode switch number
  int buttonbits = (dec >> 4) & 0x3ff;
  switch(buttonbits) {
  case 0b0001010101: button = "A"; break;
  case 0b0100010101: button = "B"; break;
  case 0b0101000101: button = "C"; break;
  case 0b0101010001: button = "D"; break;
  case 0b0101010100: button = "E"; break;
  default:
    ok = false;
    for(i=4; i>= 0; i--) {
      switch(buttonbits & 0x3) {
        case 0: stringbuf[i] = '1'; break;
        case 1: stringbuf[i] = '0'; break;
        case 2: stringbuf[i] = 'e'; break;
        case 3: stringbuf[i] = 'f'; break;
      }
      buttonbits >>= 2;
    }
    stringbuf[5] = '\0';
    button = String(stringbuf);
    break;
  }
  

  int onoffbits = (dec & 0xf);
  switch(onoffbits) {
  case 0b0001: onoff = "on"; break;
  case 0b0100: onoff = "off"; break;
  default:
    ok = false;
    stringbuf[0] = '0';
    stringbuf[1] = 'x';
    if (onoffbits < 10) {
      stringbuf[2] = ('0' + onoffbits);
    } else {
      stringbuf[2] = ('a' + onoffbits - 10);
    }
    stringbuf[3] = 0;
    onoff = stringbuf;
  }
  return ok;
}

bool decode433_elro(uint32_t dec, int bitLength, String& dip, String& button, String& onoff) {
  return false;
}

static const char* bin2tristate(const char* bin) {
  static char returnValue[50];
  int pos = 0;
  int pos2 = 0;
  while (bin[pos]!='\0' && bin[pos+1]!='\0') {
    if (bin[pos]=='0' && bin[pos+1]=='0') {
      returnValue[pos2] = '0';
    } else if (bin[pos]=='1' && bin[pos+1]=='1') {
      returnValue[pos2] = '1';
    } else if (bin[pos]=='0' && bin[pos+1]=='1') {
      returnValue[pos2] = 'F';
    } else {
      return NULL;
    }
    pos = pos+2;
    pos2++;
  }
  returnValue[pos2] = '\0';
  return returnValue;
}

static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
  static char bin[32];
  static char tmp[32];
  unsigned int i=0;

  while (Dec > 0) {
    tmp[i++] = ((Dec & 1) > 0) ? '1' : '0';
    Dec = Dec >> 1;
  }

  for (unsigned int j = 0; j< bitLength; j++) {
    if (j >= bitLength - i) {
      bin[j] = tmp[i - 1 - (j - (bitLength - i)) ];
    } else {
      bin[j] = '0';
    }
  }
  bin[bitLength] = '\0';
  
  return bin;
}

bool decode433_tristate(uint32_t dec, int bitLength, String& tristate) {
  char *bin_buf = dec2binWzerofill(dec, bitLength);
  if (bin_buf == NULL) return false;
  const char *tri_buf = bin2tristate(bin_buf);
  if (tri_buf == NULL) return false;
  tristate = String(tri_buf);
  return true;
  }