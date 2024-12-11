#include <Arduino.h>



const int frameSize = 7; // Fixed for simplicity
byte expectedSeqNum = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Receiver initialized");
}

void loop() {
  receiveFrame();
}

void receiveFrame() {
  static byte frame[frameSize];
  static int index = 0;
  static bool receiving = false;

  while (Serial.available() > 0) {
    byte b = Serial.read();

    if (b == 0x7E && !receiving) {
      receiving = true;
      index = 0;
    }

    if (receiving) {
      frame[index++] = b;
      if (index == frameSize) {
        if (frame[0] == 0x7E && frame[6] == 0x7E) {
          processFrame(frame);
        } else {
          Serial.println("Frame error");
        }
        receiving = false; // reset for next frame
        index = 0; // reset index for next frame
      }
    }
  }
}

void processFrame(byte *frame) {
  Serial.print("Received raw frame: ");
  for (int i = 0; i < frameSize; i++) {
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  if (frame[0] == 0x7E && frame[6] == 0x7E) {
    uint16_t crcReceived = (frame[4] << 8) | frame[5];
    uint16_t crcCalculated = crc16_ccitt(frame + 1, frameSize - 3);

    if (crcReceived == crcCalculated) {
      Serial.print("CRC received: ");
      Serial.print(crcReceived, HEX);
      Serial.print(" CRC calculated: ");
      Serial.println(crcCalculated, HEX);
    } else {
      Serial.print("CRC received: ");
      Serial.print(crcReceived, HEX);
      Serial.print("  ; CRC calculated: ");
      Serial.print(crcCalculated, HEX);
      Serial.println("  \n");
    }

    byte seqNum = (frame[2] >> 1) & 0x07;
    byte data = frame[3];

    if (seqNum == expectedSeqNum) {
      Serial.print("Received frame: ");
      Serial.print(seqNum);
      Serial.print(" Data: ");
      Serial.println(data);
      sendAck(seqNum);
      expectedSeqNum++;
    } else {
      Serial.print("Discarded frame: ");
      Serial.println(seqNum);
      sendAck(expectedSeqNum - 1);
    }
  } else {
    Serial.println("Frame error");
  }
}

void sendAck(byte seqNum) {
  byte ackFrame = seqNum | 0x80; // ACK frame with seqNum
  Serial.print("Sending ACK for frame: ");
  Serial.println(seqNum);
  Serial.write(ackFrame);
  Serial.flush(); // Ensure ACK is sent out
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