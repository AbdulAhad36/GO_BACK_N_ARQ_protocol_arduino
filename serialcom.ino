#include <Arduino.h>

const int frameSize = 7; // Fixed for simplicity
const int windowSize = 4;
const int timeout = 1000; // 1 second timeout
byte frames[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
int base = 0;
int nextSeqNum = 0;
unsigned long timers[10];

void setup() {
  Serial.begin(9600);
  Serial.println("Transmitter initialized");
}

void loop() {
  sendFrames();
  receiveAck();
  handleTimeout();
}

void sendFrames() {
  while (nextSeqNum < base + windowSize && nextSeqNum < 10) {
    sendFrame(nextSeqNum);
    timers[nextSeqNum] = millis();
    nextSeqNum++;
    delay(100); // Add delay to ensure receiver can keep up
  }
}

void sendFrame(int seqNum) {
  Serial.print("Sending frame: ");
  Serial.println(seqNum);
  byte frame[frameSize] = {0x7E, 0x01, (byte)(0x00 | (seqNum << 1)), frames[seqNum], 0x00, 0x00, 0x7E};
  uint16_t crc = crc16_ccitt(frame + 1, frameSize - 3);
  frame[4] = crc >> 8; // CRC high byte
  frame[5] = crc & 0xFF; // CRC low byte

  Serial.print("Frame: ");
  for (int i = 0; i < frameSize; i++) {
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial.write(frame, frameSize);
  Serial.flush(); // Ensure data is sent out
}

void receiveAck() {
  while (Serial.available() > 0) {
    byte ack = Serial.read();
    if ((ack & 0x80) == 0x80) { // Check if the frame is an ACK
      ack = ack & 0x7F; // Clear the MSB to get the sequence number
      Serial.print("Received ACK for frame: ");
      Serial.println(ack);
      if (ack >= base && ack < base + windowSize) {
        base = ack + 1;
        if (base == nextSeqNum) {
          nextSeqNum = base;
        }
      }
    }
  }
}

void handleTimeout() {
  for (int i = base; i < nextSeqNum; i++) {
    if (millis() - timers[i] > timeout) {
      Serial.print("Timeout for frame: ");
      Serial.println(i);
      nextSeqNum = i; // Re-transmit from the frame that timed out
      break;
    }
  }
}

uint16_t crc16_ccitt(const byte *data, size_t length) {
  uint16_t crc = 0xFFFF;
  while (length--) {
    crc ^= *data++ << 8;
    for (int i = 0; i < 8; i++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}
