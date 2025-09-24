#include <SPI.h>

#define SPI_MOSI 3
#define SPI_MISO 4
#define SPI_SCK 2
#define ESP_CS 5
#define ESP_READY 8
#define ESP_RESET 9

// Command definitions
#define CMD_HD_WRBUF_REG      0x01
#define CMD_HD_RDBUF_REG      0x02
#define CMD_HD_WRDMA_REG      0x03
#define CMD_HD_RDDMA_REG      0x04
#define CMD_HD_WR_END_REG     0x07
#define CMD_HD_INT0_REG       0x08
#define WRBUF_START_ADDR      0x0
#define RDBUF_START_ADDR      0x4

SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0);
uint8_t current_send_seq = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(ESP_CS, OUTPUT);
  pinMode(ESP_READY, INPUT);
  pinMode(ESP_RESET, OUTPUT);
  digitalWrite(ESP_CS, HIGH);

  SPI.setRX(SPI_MISO);
  SPI.setTX(SPI_MOSI);
  SPI.setSCK(SPI_SCK);
  SPI.begin();

  Serial.println("ESP32 SPI AT Bridge");
  Serial.println("==============================");

  resetESP();
  Serial.println("Type AT commands to communicate with ESP32");
}

void resetESP() {
  Serial.println("Resetting ESP32...");
  digitalWrite(ESP_RESET, LOW);
  delay(100);
  digitalWrite(ESP_RESET, HIGH);
  delay(3000);
  Serial.println("ESP32 ready");
}

uint32_t querySlaveStatus() {
  digitalWrite(ESP_CS, LOW);
  SPI.beginTransaction(spiSettings);

  SPI.transfer(CMD_HD_RDBUF_REG);
  SPI.transfer(RDBUF_START_ADDR);
  SPI.transfer(0x00);

  uint32_t status = 0;
  for (int i = 0; i < 4; i++) {
    status |= (uint32_t)SPI.transfer(0x00) << (i * 8);
  }

  SPI.endTransaction();
  digitalWrite(ESP_CS, HIGH);
  return status;
}

void sendWriteRequest(uint16_t send_len) {
  digitalWrite(ESP_CS, LOW);
  SPI.beginTransaction(spiSettings);

  SPI.transfer(CMD_HD_WRBUF_REG);
  SPI.transfer(WRBUF_START_ADDR);
  SPI.transfer(0x00);

  SPI.transfer(send_len & 0xFF);
  SPI.transfer((send_len >> 8) & 0xFF);
  SPI.transfer(current_send_seq + 1);
  SPI.transfer(0xFE);

  SPI.endTransaction();
  digitalWrite(ESP_CS, HIGH);

  current_send_seq++;
}

void sendDataToSlave(const uint8_t* data, uint16_t len) {
  digitalWrite(ESP_CS, LOW);
  SPI.beginTransaction(spiSettings);

  SPI.transfer(CMD_HD_WRDMA_REG);
  SPI.transfer(0x00);
  SPI.transfer(0x00);

  for (uint16_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }

  SPI.endTransaction();
  digitalWrite(ESP_CS, HIGH);
}

void sendEndOfTransmission() {
  digitalWrite(ESP_CS, LOW);
  SPI.beginTransaction(spiSettings);
  SPI.transfer(CMD_HD_WR_END_REG);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(ESP_CS, HIGH);
}

String receiveDataFromSlave(uint16_t len) {
  uint8_t buffer[len + 1];
  String result = "";

  digitalWrite(ESP_CS, LOW);
  SPI.beginTransaction(spiSettings);

  SPI.transfer(CMD_HD_RDDMA_REG);
  SPI.transfer(0x00);
  SPI.transfer(0x00);

  for (uint16_t i = 0; i < len; i++) {
    buffer[i] = SPI.transfer(0x00);
    if (buffer[i] >= 32 && buffer[i] <= 126) { // Printable ASCII
      result += (char)buffer[i];
    }
  }

  SPI.endTransaction();
  digitalWrite(ESP_CS, HIGH);

  // Send end of reception
  digitalWrite(ESP_CS, LOW);
  SPI.beginTransaction(spiSettings);
  SPI.transfer(CMD_HD_INT0_REG);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(ESP_CS, HIGH);

  return result;
}

bool waitForHandshake(unsigned long timeout) {
  unsigned long start = millis();
  while (digitalRead(ESP_READY) == LOW) {
    if (millis() - start > timeout) return false;
    delay(1);
  }
  return true;
}

void sendATCommand(String command) {
  // Format command properly
  if (!command.startsWith("AT")) {
    command = "AT" + command;
  }
  if (!command.endsWith("\r\n")) {
    command += "\r\n";
  }

  uint16_t len = command.length();
  uint8_t cmd_bytes[len];
  command.getBytes(cmd_bytes, len + 1);

  Serial.print(">>> ");
  Serial.print(command);

  // Send write request
  sendWriteRequest(len);

  // Wait for handshake
  if (!waitForHandshake(2000)) {
    Serial.println("No response from ESP");
    return;
  }

  // Check slave status
  uint32_t status = querySlaveStatus();
  uint8_t direct = status & 0xFF;
  uint8_t seq_num = (status >> 8) & 0xFF;
  uint16_t data_len = (status >> 16) & 0xFFFF;

  if (direct == 2 && seq_num == current_send_seq) {
    // Send the command data
    sendDataToSlave(cmd_bytes, len);
    sendEndOfTransmission();

    // Wait for response
    if (waitForHandshake(5000)) {
      status = querySlaveStatus();
      direct = status & 0xFF;
      data_len = (status >> 16) & 0xFFFF;

      if (direct == 1 && data_len > 0) {
        String response = receiveDataFromSlave(data_len);
        Serial.print("<<< ");
        Serial.println(response);
      }
    }
  }
}

void processIncomingData() {
  uint32_t status = querySlaveStatus();
  uint8_t direct = status & 0xFF;
  uint16_t data_len = (status >> 16) & 0xFFFF;

  if (direct == 1 && data_len > 0) {
    String response = receiveDataFromSlave(data_len);
    if (response.length() > 0) {
      Serial.print("<<< ");
      Serial.println(response);
    }
  }
}

void loop() {
  // Process any incoming data from ESP
  if (digitalRead(ESP_READY) == HIGH) {
    processIncomingData();
  }

  // Handle serial commands
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "reset") {
      resetESP();
    } else if (input == "status") {
      uint32_t status = querySlaveStatus();
      Serial.print("ESP Status - Direct: ");
      Serial.print(status & 0xFF);
      Serial.print(", Seq: ");
      Serial.print((status >> 8) & 0xFF);
      Serial.print(", Data Len: ");
      Serial.println((status >> 16) & 0xFFFF);
    } else if (input.length() > 0) {
      sendATCommand(input);
    }
  }

  delay(50);
}
