#include <WiFi.h>
#include <PubSubClient.h>

#define LDR_PIN 15
#define NTC_PIN 2
#define CURRENT_PIN 4
#define outputPin 25
#define outputPin_2 26
#define DISP_1 22
#define DISP_2 23

void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void controle_temp(float kp, float setpoint, float pv);
void controle_lum(float kp, float setpoint, float pv);

// Update these with values suitable for your network.

const char* ssid = "Claro1969";
const char* password = "Vera1969";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int SP_Temp,SP_Lum;
float kp = 25;
float output_antes_lum, output_antes_temp;

void setup() {
  pinMode(DISP_1, OUTPUT);  
  pinMode(DISP_2, OUTPUT);    
  dacWrite(outputPin, 0);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  analogReadResolution(9); // definir resolução para 9 bits
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temp = 24, lum = 80, current = 0.4;
  snprintf (msg, MSG_BUFFER_SIZE, "%f", temp);
  client.publish("Contauto/Temp", msg);
  snprintf (msg, MSG_BUFFER_SIZE, "%f", lum);
  client.publish("Contauto/Lum", msg);
  snprintf (msg, MSG_BUFFER_SIZE, "%f", current);
  client.publish("Contauto/Current", msg);

  controle_temp(kp,SP_Temp,temp);
  controle_lum(kp,SP_Lum,lum);
}

void controle_temp(float Kp, float setpoint, float pv) {
  float error = setpoint - pv;

  // Calcular saída do P
  float output = Kp * error;
  float output_antes = 0;

  // Limitar saída entre 0 e 255 (0 a 3.3V no DAC)
  output = constrain(output, 0, 255);

  if(output != output_antes_temp) {
    output_antes_temp = output;
    Serial.println(output*3.3/255);
  }
  // Enviar saída ao DAC
  dacWrite(outputPin, (int)output);
}

void controle_lum(float Kp, float setpoint, float pv) {
  float error = pv - setpoint;

  // Calcular saída do P
  float output = Kp*error;

  // Limitar saída entre 0 e 255 (0 a 3.3V no DAC)
  output = constrain(output, 0, 255);

  if(output != output_antes_lum) {
    output_antes_lum = output;
    Serial.println(output*3.3/255);
  }

  // Enviar saída ao DAC
  dacWrite(outputPin_2, (int)output);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (topic[9] == 'T') {
    char total[2];
    for (int i = 0; i < length; i++) {
      total[i] = (char)payload[i];
    }
    String nmr = total;
    SP_Temp = nmr.toInt();
  }

   if (topic[9] == 'L') {
    char total_2[2];
    for (int i = 0; i < length; i++) {
      total_2[i] = (char)payload[i];
    }
    String nmr_2 = total_2;
    SP_Lum = nmr_2.toInt();
  }

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == 'A') {
    digitalWrite(DISP_1, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } if ((char)payload[0] == 'a') {
    digitalWrite(DISP_1, LOW);  // Turn the LED off by making the voltage HIGH
  } if ((char)payload[0] == 'J') {
    digitalWrite(DISP_2, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } if ((char)payload[0] == 'j') {
    digitalWrite(DISP_2, LOW);  // Turn the LED off by making the voltage HIGH
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      //IN TOPICS
      client.subscribe("Contauto/Disp_1");
      client.subscribe("Contauto/Disp_2");
      client.subscribe("Contauto/Temp_SP");
      client.subscribe("Contauto/Lum_SP");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

