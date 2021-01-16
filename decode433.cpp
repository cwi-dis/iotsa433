#include "iotsa.h"
#include "decode433.h"

#undef decode433_debug_prints

static const char* bin2tristate(const char* bin);
static char * dec2binWzerofill(unsigned long Dec, unsigned int bitLength);

#ifdef WITH_HEMA
bool decode433_hema(uint32_t dec, int bitLength, String& dip, String& appliance, String& state) {
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
  int appliancebits = (dec >> 4) & 0x3ff;
  switch(appliancebits) {
  case 0b0001010101: appliance = "A"; break;
  case 0b0100010101: appliance = "B"; break;
  case 0b0101000101: appliance = "C"; break;
  case 0b0101010001: appliance = "D"; break;
  case 0b0101010100: appliance = "E"; break;
  default:
    ok = false;
    for(i=4; i>= 0; i--) {
      switch(appliancebits & 0x3) {
        case 0: stringbuf[i] = '1'; break;
        case 1: stringbuf[i] = '0'; break;
        case 2: stringbuf[i] = 'e'; break;
        case 3: stringbuf[i] = 'f'; break;
      }
      appliancebits >>= 2;
    }
    stringbuf[5] = '\0';
    appliance = String(stringbuf);
    break;
  }
  

  int statebits = (dec & 0xf);
  switch(statebits) {
  case 0b0001: state = "on"; break;
  case 0b0100: state = "off"; break;
  default:
    ok = false;
    stringbuf[0] = '0';
    stringbuf[1] = 'x';
    if (statebits < 10) {
      stringbuf[2] = ('0' + statebits);
    } else {
      stringbuf[2] = ('a' + statebits - 10);
    }
    stringbuf[3] = 0;
    state = stringbuf;
  }
  return ok;
}
#endif // WITH_HEMA

#ifdef WITH_ELRO_FLAMINGO
//
// Flamingo encode/decode copied with mods from https://github.com/windkh/flamingoswitch
//
static uint8_t elro_key[17] = { 9, 6, 3, 8, 10, 0, 2, 12, 4, 14, 7, 5, 1, 15, 11, 13, 9 }; //cryptokey 
static uint8_t elro_ikey[16] = { 5, 12, 6, 2, 8, 11, 1, 10, 3, 0, 4, 14, 7, 15, 9, 13 };  //invers cryptokey (exchanged index & value)

bool decode433_elro(uint32_t dec, int bitLength, String& dip, String& appliance, String& state) {
  if (bitLength != 28) return false;
  uint8_t mn[7];	// message separated in nibbles

  dec = ((dec << 2) & 0x0FFFFFFF) | ((dec & 0xC000000) >> 0x1a);		//shift 2 bits left & copy bit 27/28 to bit 1/2
  mn[0] = dec & 0x0000000F;
  mn[1] = (dec & 0x000000F0) >> 0x4;
  mn[2] = (dec & 0x00000F00) >> 0x8;
  mn[3] = (dec & 0x0000F000) >> 0xc;
  mn[4] = (dec & 0x000F0000) >> 0x10;
  mn[5] = (dec & 0x00F00000) >> 0x14;
  mn[6] = (dec & 0x0F000000) >> 0x18;

  mn[6] = mn[6] ^ 9;										// no decryption

  //XOR decryption 2 rounds
  for (uint8_t r = 0; r <= 1; r++)
  {														// 2 decryption rounds
      for (uint8_t i = 5; i >= 1; i--)
      {													// decrypt 4 nibbles
          mn[i] = ((elro_ikey[mn[i]] - r) & 0x0F) ^ mn[i - 1];	// decrypted with predecessor & key
      }
      mn[0] = (elro_ikey[mn[0]] - r) & 0x0F;					//decrypt first nibble
  }

  //Output decrypted message 
  //uint32_t in = (~((input >> 2) | (((input & 3) << 0x1a))) << 4);
  uint16_t receiverId = (uint16_t)mn[0];
  uint16_t value = (((mn[1] >> 1) & 1) + (mn[6] & 0x7) + ((mn[6] & 0x8) >> 3));
  uint16_t rollingCode = (mn[1] >> 2);
  uint16_t transmitterId = (mn[5] << 12) + (mn[4] << 8) + (mn[3] << 4) + (mn[2] << 0);
#ifdef decode433_debug_prints
  IotsaSerial.printf("xxxjack decode_433: receiverID=0x%x, value=0x%x, rollingCode=0x%x, transmitterId=0x%x\n", receiverId, value, rollingCode, transmitterId);
#endif
  dip = String(transmitterId);
  appliance = String(receiverId);
  state = String(value);
  return true;
}

String encode433_elro(String group, String appliance, int state) {
#ifdef decode433_debug_prints
  IotsaSerial.printf("xxxjack encode_433: group=%s, appliance=%s, state=%d\n", group.c_str(), appliance.c_str(), state);
#endif
  uint8_t mn[7];
  static uint8_t rollingCode;
  rollingCode = (rollingCode+1) & 0x3;
  uint8_t receiverId = appliance.toInt();
  uint16_t transmitterId = group.toInt();
  int value = state;
  mn[0] = receiverId;								// mn[0] = iiiib i=receiver-ID
  mn[1] = (rollingCode << 2) & 15; 				// 2 lowest bits of rolling-code
  if (value > 0)
  {												// ON or OFF
      mn[1] |= 2;
  }												// mn[1] = rrs0b r=rolling-code, s=ON/OFF, 0=const 0?
  mn[2] = transmitterId & 15;						// mn[2..5] = ttttb t=transmitterId in nibbles -> 4x ttttb
  mn[3] = (transmitterId >> 4) & 15;
  mn[4] = (transmitterId >> 8) & 15;
  mn[5] = (transmitterId >> 12) & 15;
  if (value >= 2 && value <= 9)
  {												// mn[6] = dpppb d = dim ON/OFF, p=%dim/10 - 1
      mn[6] = value - 2;							// dim: 0=10%..7=80%
      mn[6] |= 8;									// dim: ON
  }
  else
  {
      mn[6] = 0;									// dim: OFF
  }

  //XOR encryption 2 rounds
  for (uint8_t r = 0; r <= 1; r++)
  {												// 2 encryption rounds
      mn[0] = elro_key[mn[0] - r + 1];					// encrypt first nibble
      for (uint8_t i = 1; i <= 5; i++)
      {											// encrypt 4 nibbles
          mn[i] = elro_key[(mn[i] ^ mn[i - 1]) - r + 1];// crypted with predecessor & key
      }
  }

  mn[6] = mn[6] ^ 9;								// no  encryption

  uint32_t msg = 0;								// copy the encrypted nibbles in output buffer
  msg |= (uint32_t)mn[6] << 0x18;
  msg |= (uint32_t)mn[5] << 0x14;
  msg |= (uint32_t)mn[4] << 0x10;
  msg |= (uint32_t)mn[3] << 0x0c;
  msg |= (uint32_t)mn[2] << 0x08;
  msg |= (uint32_t)mn[1] << 0x04;
  msg |= (uint32_t)mn[0];
  msg = (msg >> 2) | ((msg & 3) << 0x1a);			// shift 2 bits right & copy lowest 2 bits of cbuf[0] in msg bit 27/28
  String rv = String(msg, BIN);
  while (rv.length() < 28) rv = "0" + rv;
#ifdef decode433_debug_prints
  { 
    String _1, _2, _3; 
    decode433_elro(msg, 28, _1, _2, _3);
    IotsaSerial.printf("xxxjack deencode_433: group=%s, appliance=%s, state=%s\n", _1.c_str(), _2.c_str(), _3.c_str());
  }
#endif
  return rv;
}
#endif // WITH_ELRO_FLAMINGO

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

bool decode433_tristate(uint32_t dec, int bitLength, String& telegram_tristate) {
  char *bin_buf = dec2binWzerofill(dec, bitLength);
  if (bin_buf == NULL) return false;
  const char *tri_buf = bin2tristate(bin_buf);
  if (tri_buf == NULL) return false;
  telegram_tristate = String(tri_buf);
  return true;
  }