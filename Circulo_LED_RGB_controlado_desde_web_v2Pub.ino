#include "WiFiEsp.h" //Libreria para comunicarse facilmente con el modulo ESP01
#include "SoftwareSerial.h"
#include <Adafruit_NeoPixel.h>

#define PIN_RGB_LED            6
#define NUMPIXELS      24
#define PIN_RX 12
#define PIN_TX 11

// Conexiones de ESP8266
// TX Pin 11
// RX Pin 12
// Poner 3,3v con fuente externa a VCC, RST y a CH_PD
// GND a GND
 
char ssid[] = "EL_SSID_DE_TU_WIFI";            // SSID (Nombre de la red WiFi)
char pass[] = "LA_PASS_DE_TU_WIFI";        // Contraseña
int status = WL_IDLE_STATUS;     // Estado del ESP. No tocar

String HTTP_req;          // almacena la petición HTTP

WiFiEspServer server(80);
SoftwareSerial esp8266(PIN_RX, PIN_TX);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);

bool rotar = false;
 
void setup() {
   Serial.begin(9600); //Monitor serie
   pixels.begin();
   setupWifi();
   imprimeIP();
   comprobarLeds();

   // Iniciamos el servidor web en el puerto 80
   server.begin();
}

void setupWifi(){
  esp8266.begin(9600); //ESP01

   WiFi.init(&esp8266);
   
   //intentar iniciar el modulo ESP
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Modulo no presente. Reinicie el Arduino y el ESP01 (Quite el cable que va de CH_PD a 3.3V y vuelvalo a colocar)");
    //Loop infinito
    while (true);
  }

  //Intentar conectar a la red wifi
  while ( status != WL_CONNECTED) {
    Serial.print("Intentando conectar a la red WiFi: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }
}

void imprimeIP(){
  IPAddress ip = WiFi.localIP();
  String ipString = String(ip[0]);
  for (byte octet = 1; octet < 4; ++octet) {
     ipString += '.' + String(ip[octet]);
  }
  Serial.println("IP asignada: " + ipString);
}
 
void loop() {
  WiFiEspClient client = server.available();
  if (client) {  // tenemos un cliente?
        boolean lineaActualEnBlanco = true;
        while (client.connected()) {
            if (client.available()) {   // podemos leer la información
                char c = client.read();
                HTTP_req += c;  // cada vez leemos un caracter
                // esperamos a que una línea me venga en blanco y acabada en \n
                if (c == '\n' && lineaActualEnBlanco) {
                    // enviamos la respuesta estandar http
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println("Connection: close");
                    client.println();
                    // elaboramos la página
                    client.println("<!DOCTYPE html>");
                    client.println("<html>");
                    client.println("<head>");
                    client.println("<title>Control LEDs RGB</title>");
                    client.println("<style type=\"text/css\">body {font-size:2rem; font-family: Arial; text-align:center; color: #444; background-color: #cdcdcd;} div{width:85%; background-color:#fff; padding:20px; text-align:left; border:10px solid #bbbb00; margin:25px auto;}</style>");
                    client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                    client.println("</head>");
                    client.println("<body>");
                    client.println("<div>");
                    client.println("<h1>Controlando LEDs RGB desde entorno web</h1>");
                    client.println("<p>Seleccione un color para cada LED</p>");
                    client.println("<form method=\"get\">");
                    LeerParametros(client);
                    for(int i=0;i<NUMPIXELS;i++){ // un control por cada LED
                      client.println("<input type=\"color\" name=\"LED" + String(i) + "\" value=\"#" + String(pixels.getPixelColor(i), HEX) + "\"> LED" + String(i) + "<br/>");
                    }
                    client.println("<br/><input type=\"submit\" value=\"Enviar\">");
                    client.println("<input type=\"button\" onclick=\"location.href='?LED0=%232300E5&LED1=%232900DB&LED2=%233001D1&LED3=%233702C7&LED4=%233E03BD&LED5=%234404B3&LED6=%234B05A9&LED7=%2352069F&LED8=%23590795&LED9=%2360088B&LED10=%23660981&LED11=%236D0A77&LED12=%23740B6D&LED13=%237B0C63&LED14=%23810D59&LED15=%23880E4F&LED16=%238F0F45&LED17=%2396103B&LED18=%239D1131&LED19=%23A31227&LED20=%23AA131D&LED21=%23B11413&LED22=%23B81509&LED23=%23BF1600';\" value=\"Colorear\">");
                    client.println("<input type=\"button\" onclick=\"location.href='?LED0=%23000000&LED1=%23000000&LED2=%23000000&LED3=%23000000&LED4=%23000000&LED5=%23000000&LED6=%23000000&LED7=%23000000&LED8=%23000000&LED9=%23000000&LED10=%23000000&LED11=%23000000&LED12=%23000000&LED13=%23000000&LED14=%23000000&LED15=%23000000&LED16=%23000000&LED17=%23000000&LED18=%23000000&LED19=%23000000&LED20=%23000000&LED21=%23000000&LED22=%23000000&LED23=%23000000';\" value=\"Apagar todo\">");
                    if(rotar){
                     client.println("<input type=\"checkbox\" name=\"ROTAR\" value=\"S\" checked> Rotar<br>"); 
                    }else{
                      client.println("<input type=\"checkbox\" name=\"ROTAR\" value=\"S\"> Rotar<br>"); 
                    }
                    client.println("</form>");
                    client.println("</div>");
                    client.println("</body>");
                    client.println("</html>");

                    HTTP_req = "";    // limpiamos
                    break;
                }
                // todas las líneas que envía el cliente acaban en \r\n
                if (c == '\n') {
                    lineaActualEnBlanco = true;
                } 
                else if (c != '\r') {
                    // lo que hemos recibido es un caracter
                    lineaActualEnBlanco = false;
                }
            } 
        }
        delay(1);      // le damos algo de tiempo al servidor para recibir la respuesta
        client.stop(); // cerramos la conexión
    }
    if(rotar){
     rotarLeds(50); 
    }
}

void LeerParametros(WiFiEspClient cliente){
  if(HTTP_req.indexOf("GET /favicon.ico HTTP/1.1") == -1){
    String lineaDeInteres = HTTP_req.substring(HTTP_req.indexOf("GET /?"), HTTP_req.indexOf("\n"));  // necesitamos situarnos en la línea adecuada para leer los parámetros
    int posicion;
    String color;
    for(int i=0;i<NUMPIXELS;i++){
      posicion = lineaDeInteres.indexOf("LED" + String(i));
      if (posicion > -1) {  // hemos encontrado el parámetro para el LED en cuestión
        color = lineaDeInteres.substring(lineaDeInteres.indexOf("=%23", posicion) + 4, lineaDeInteres.indexOf("=%23", posicion) + 10);
        //Serial.println("Enciende " + String(i) + " de color " + color );
      }else{
        color = "000000";
        //Serial.println("Apaga " + String(i));
      }
      Colorea(i,color);
    }

    // rotacion
    posicion = lineaDeInteres.indexOf("ROTAR");
    if (posicion > -1) {
      rotar = true;
    }else{
      rotar = false;
    }
  }
}

void Colorea(int idLed, String color){
  long number = (long) strtol( &color[0], NULL, 16);
  int r = number >> 16;
  int g = number >> 8 & 0xFF;
  int b = number & 0xFF;
  pixels.setPixelColor(idLed, pixels.Color(r,g,b));
  pixels.show();
}

void comprobarLeds(){
  for(int i=0;i<NUMPIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,100));
    pixels.show();
    delay(50);
  }

  for(int i=0;i<NUMPIXELS;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0));
    pixels.show();
    delay(50);
  }
}

void rotarLeds(unsigned long wait){
  uint8_t *ptr = pixels.getPixels(), tmp[3];
  uint16_t numBytes = pixels.numPixels()*3;

  memcpy(tmp, ptr, 3); // guardamos el primer pixel
  memcpy(ptr, ptr+3, numBytes-3); // desplazamos todos los pixels
  memcpy(ptr+numBytes-3, tmp, 3); // restauramos el primer pixel

  pixels.show();
  delay(wait);
}
