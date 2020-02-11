#include <DFRobot_HX711.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define DEBUG 1
#define NB_INGREDIENT 4

typedef enum {
    STATUS_QUEUE = 0,
    STATUS_PREPARE = 1,
    STATUS_DONE = 2,
    STATUS_SERVED = 3,
    STATUS_ERROR = 4
} Status_t;

typedef struct {
  char* name;
  int value;
  int pin;
} Ingredient_t;

char buffer_url[100] = {};
char buffer_msg[100] = {};
const char* status_name[] = {"queue", "prepare", "done", "served", "error"};

// WiFi Config
const char* SSID = "mi_tybau";
const char* password = "azertyuiop";
const char* URL_GET_FIRST = "http://vps127565.ovh.net:8080/drink/first";
const char* URL_CHANGE_STATUS = "http://vps127565.ovh.net:8080/drink/%s";

// Web Client
WiFiClient client;
HTTPClient http;

// Weighing scale
const float weight_init = 5.5;
const float weight_full = 200.0;
float weight;
DFRobot_HX711 MyScale(D1, D2);

//const char* key_ingredients[NB_INGREDIENT] = {"water", "lemonade", "grenadine", "mint"};
Ingredient_t ingredients[NB_INGREDIENT] = {
  {
    .name= "water",
    .value= 0,
    .pin= D3
  }, {
    .name= "lemonade",
    .value= 0,
    .pin= D5
  }, {
    .name= "grenadine",
    .value= 0,
    .pin= D6
  }, {
    .name= "mint",
    .value= 0,
    .pin= D7
  }
};

const char device_id[200] = {};
Status_t drink_status = STATUS_QUEUE;
const int values[NB_INGREDIENT] = {};

void printDebug(char* s) {
  #if DEBUG == 1
    Serial.println(s);
  #endif 
}

/***************** API HTTP *********************/
char getValuesDrink() {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(URL_GET_FIRST);
    http.addHeader("accept", "text/plain");
    int http_post_code = http.GET();
    int ret_code = 1;
    if (analyzeHttpResponse(URL_GET_FIRST, http_post_code) == 0) {
      const String& payload = http.getString();
      int nb_grab = sscanf(payload.c_str(), "%s %d,%d,%d,%d", device_id, &(ingredients[0].value), &(ingredients[1].value), &(ingredients[2].value), &(ingredients[3].value));
      
      if(nb_grab == NB_INGREDIENT + 1) {
        ret_code = 0;

        #if DEBUG == 1
          Serial.print("Ingredients from payload ");
          Serial.print(payload);
          Serial.println(":");
          for(int i = 0; i < NB_INGREDIENT; i++) {
            Serial.print("\t");
            Serial.print(ingredients[i].name);
            Serial.print(": ");
            Serial.println(ingredients[i].value);
          }
          Serial.println(device_id);
        #endif
      }
      else {
        Serial.println(device_id);
        Serial.print("Grab: ");
        Serial.print(nb_grab);
        Serial.print(", ");
        Serial.println(payload);
        ret_code = 2;
      }
    }
    http.end();
    return ret_code;
  }

  return 1;
}

char changeStatus() {
  int ret_code = 0;
  int length_device = strlen(device_id);
  
  Serial.print("ChangeStatus: '");
  Serial.print(device_id);
  Serial.print("', strlen=");
  Serial.print(length_device);
  Serial.print(", device_id[0]=");
  Serial.println(device_id[0]);

  // WTF NEED LENGTH DEVICE VARIABLE, WITHOUT VARIABLE STRLEN == 0 !?
  if(length_device == 0) {
    Serial.println("Error changeStatus device_id = 0");
    return 1;
  }

  sprintf(buffer_url, URL_CHANGE_STATUS, device_id);
  http.begin(buffer_url);
  http.addHeader("Content-Type", "application/json");

  sprintf(buffer_msg, "{\"status\":\"%s\"}", status_name[drink_status]);
  Serial.print("Send POST:");
  Serial.println(buffer_msg);
  int code = http.POST(buffer_msg);
  if (analyzeHttpResponse(URL_GET_FIRST, code) != 0)
    ret_code = 1;
  http.end();

  return ret_code;
}

char analyzeHttpResponse(const char* url, int code) {
  if (code > 0) {
    if (code != 200) {
      Serial.print("ERROR: Http error code:");
      Serial.print(code);
      Serial.print(" ");
      Serial.println(url);
      return 1;
    }
    return 0;
  }
  else {
    Serial.print("ERROR Http error:");
    Serial.println(url);
    Serial.print("Code ");
    Serial.print(code);
    Serial.print(": ");
    Serial.println(http.errorToString(code).c_str());
    return 2;
  }
}

void fillGlass() {
  for(int i = 0; i < NB_INGREDIENT; i++) {
    fillIngredient(&(ingredients[i]));
  }
}

void fillIngredient(Ingredient_t* ing) {
  char error = 0;
  float threshold = ing->value/100.0 * (weight_full - weight_init);
  float weight_before = MyScale.readWeight();
  
  weight = weight_before;
  
  
  Serial.print("Fill ");
  Serial.print(ing->value);
  Serial.print("% with:");
  Serial.print(ing->name);
  Serial.print(" from ");
  Serial.print(weight_before);
  Serial.print(" to ");
  Serial.println(threshold + weight_before);
  

  while(weight < threshold + weight_before) {
    if(weight <= weight_init) {
      Serial.println("");
      Serial.print("ERROR weight < weight init: ");
      Serial.print(weight);
      Serial.print(" < ");
      Serial.println(weight_init);
      error = 1;
      break;
    }
    
    if(ing->pin != -1)
      digitalWrite(ing->pin, HIGH);
    weight = MyScale.readWeight();
    Serial.print(weight);
    Serial.println("g loop");
    yield();
  }

  if(ing->pin != -1)
    digitalWrite(ing->pin, LOW);
  Serial.println("Filled");
}

void setup() {
  Serial.begin(9600);

  // Connection to Wifi
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to WiFi");

  // Declare pinMode
  for(int i = 0; i < NB_INGREDIENT; i++) {
    if(ingredients[i].pin != -1)
      pinMode(ingredients[i].pin, OUTPUT);
  }
}

void loop() {
  /*float weight = MyScale.readWeight();
  Serial.print(weight, 1);
  Serial.println(" g");*/

/*
  if(weight > 20)
  {
    digitalWrite(D2, HIGH);
    Serial.println("glouglou");
  }
  else
  {
    digitalWrite(D2, LOW);
    Serial.println("apusoif");
  }*/

  int cpt = 0;
  do {
    weight = MyScale.readWeight();
    Serial.print(weight, 1);
    Serial.println(" g");
  
    if(weight >= weight_init)
      cpt++;
    else
      cpt = 0;
    delay(500);
  } while(cpt < 3);
  Serial.println("Glass detected");
  
  if (getValuesDrink() == 0) {
    Serial.println("getValuesDrink OK");

    // Prepare a new drink
    drink_status = STATUS_PREPARE;
    changeStatus();

    // Fill Glass
    fillGlass();

    // Glass Ready
    weight = MyScale.readWeight();
    drink_status = STATUS_DONE;
    changeStatus();
    while(weight > weight_init) {
      delay(500);
      weight = MyScale.readWeight();
    }

    // Glass served
    drink_status = STATUS_SERVED;
    changeStatus();
  }
  else if(getValuesDrink() == 2) {
    // No new drinks or Error
    Serial.println("No new Drink");
  }
  else {
    // Error request
    Serial.println("getValuesDrink KO");
  }
  delay(500);
}
