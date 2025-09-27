/** https://github.com/iavorvel/MyLD2410
    В этой библиотеке реализован полный набор последовательных команд для датчика присутствия LD2410. **/
#include <MyLD2410.h>

/**  UART (Universal Asynchronous Receiver/Transmitter) - это последовательный протокол связи, который обеспечивает простой обмен данными между двумя устройствами. **/
// Определим UART порт через который осуществим связь с датчиком LD2410.
#define sensorSerial Serial1
// Определим пин по которому будем принимать данные с датчика LD2410.
#define RX_PIN 16
// Определим пин по которому будем передавать данные на датчик LD2410.
#define TX_PIN 17
// Создадим абстрактный обьект для взаимодействия с датчиком LD2410.
MyLD2410 sensor(sensorSerial);

// Библиотека для работы с wifi подключением (standard library).
#include <WiFi.h>  
// Библиотека для создания и запуска веб-сервера, обработки HTTP-запросов, формирования и отправки HTTP-ответов клиенту.
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
// Библиотека позволяет устанавливать постоянное соединение между сервером и клиентом, что делает возможным двустороннюю передачу данных в реальном времени.
#include <WebSocketsServer.h> 
// Библиотека используется для работы с JSON (JavaScript Object Notation) данными.
#include <ArduinoJson.h>
// Документ хранящий код HTML странички.
#include "WebPage.h"

// Recurrent Neural Network-----------------------------------------------------
// Параметры модели.
#include "weights.h"
// Модель машиного обучения.
#include "SimpleRNN.h"
// -------------------------------------------------------------------------


// Данные WiFi сети.
const char* ssid = "WiFi_name";
const char* password = "WiFi_password";

// Экземпляр вебсервера ссылается на 80 порт.
AsyncWebServer server(80);
// Экземпляр WebSocket-сервера ссылается на 81 порт.
WebSocketsServer webSocket = WebSocketsServer(81);

// Переменная отображает состояние кнопки 'BTN_Detect' отвечающей за захват данных с датчика и их классификацию.
bool detect = false;

/** 
В документе реализованы функции для работы с датчиком, а так же функции ответственные за обработку данных.
**/
#include "functions.h"

// Структура данных для хранения значений сигналов полученных с датчика LD2410, которые будут переданы на вход модели.
sensorSignal signals[TIMESTEPS];

/** 
В документе реализлвана функция webSocketEvent для обработки данных полученых через соеденение сокетов. 
**/
#include "socketConnection.h"

void setup() {
  // Последовательный порт (serial port).
  Serial.begin(115200);

  // Конфигурируем последовательный порт через который осуществим связь с датчиком LD2410.
  sensorSerial.begin(LD2410_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

  delay(2000);
  Serial.println(__FILE__);
  // Подключение к датчику LD2410.
  if (!sensor.begin()) {
    Serial.println("Failed to communicate with the sensor.");
    while (true) {}
  }

  // Переведём датчик LD2410 в расширенный режим.
  sensor.enhancedMode();


  // Подключение к WiFi.
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // Вывод IP адреса в последовательный порт.
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  
  /** Инициализация вебсервера (загружаем HTML страничку на WebServer, делаем её корневой "/"):
        + "/" - корневая папка, 
        + HTTP_GET - HTTP-метод GET для запроса данных с сервера
        + [](AsyncWebServerRequest *request) {} - лямбда-функция
        + AsyncWebServerRequest *request - указатель на объект, 
          который содержит всю информацию о запросе, поступившем на сервер.**/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // отправить (200 - http код ответа, метод отправки по html, HTML страничка)
    request -> send(200, "text\html", getHTML());
  });

  // Запуск вебсокета.
  webSocket.begin();
  // При приёме данных от клиента контролером через соеденение вебсокетов передать данные функцие webSocketEvent для дальнейшей обработки.
  // Функция webSocketEvent реализована в документе "receiveData.h".
  webSocket.onEvent(webSocketEvent);

  // Запуск вебсервера.
  server.begin();

}

void loop() {

  /** Метод webSocket.loop() обеспечивает:
        - Поддержание активного соединения с клиентами.
        - Обработку входящих данных от клиентов.
        - Обработку новых подключений и отключений клиентов.
        - Отправку данных клиентам, если это необходимо.**/
  webSocket.loop();

  // Если на вебстраничке была нажата кнопка 'BTN_Detect' отвечающая за захват цвета с датчика и его классификацию, то выполняется следующий блок кода.
  if(detect){
    // Переменная отображает состояние кнопки 'BTN_Detect' отвечающей за захват цвета с датчика и его классификацию.
    //detect = false;

    // Таймер на millis() каждую секунду опрашивает датчик LD2410.
    if ((sensor.check() == MyLD2410::Response::DATA) && (millis() > nextPrint)) {

      // Получим текущее время (значение millis()).
      unsigned long time = millis();
      // Обновим время следующего срабатывания датчика.
      nextPrint = time + printEvery;
      // Увеличиваем значение cчётчика срабатывания таймера (индекс текущей строки).
      iCount++;

      // Обновим структуру данных для хранения данных полученных с датчика добавив новый вектор данных.
      moveSignal(sensor, signals, TIMESTEPS, "unknown", time, iCount);

      // Входной тензор модели (вход модели).
      float input[TIMESTEPS][FEATURES];


      // Передадим данные на вход модели:
      for (int i = 1; i <= TIMESTEPS; i++) {
        // Передадим на вход модели данные с динамичных гейтов для текущей строки:
        for (int u = 0; u < 9; u++) {
          // Отмасштабируем данные [0 : 100] --> [-128 : 127]
          input[TIMESTEPS - i][u] = (signals[TIMESTEPS - i].movingSignals[u]);
        }
        // Передадим на вход модели данные со статичных гейтов для текущей строки:
        for (int u = 0; u < 9; u++) {
          // Отмасштабируем данные [0 : 100] --> [-128 : 127]
          input[TIMESTEPS - i][9 + u] = (signals[TIMESTEPS - i].stationarySignals[u]);
        }
      }

      // Массив для хранения вероятностей для всех классов.
      float output[OUTPUT];  

      // Пропускаем данные с датчика LD2410 через реккурентную нейронную сеть.
      simpleRNN(input, output);

      // Получим таблицу с распределением вероятностей для каждого предсказываемого класса (цвета).
      String tableProbabilities = getProbabilitiesTable(kCategoryCount, output, kCategoryLabels);
      // Передадим строку с данными для таблицы на вебстраничку, а также выведем её в последовательный порт.
      Serial.println(tableProbabilities);
      sendJson(jsonString, doc_tx, "table_string_probability", tableProbabilities);

      // Получим наименование предсказаного цвета.
      String prediction = getPredictedColor(kCategoryCount, output, kCategoryLabels);
      signals[0].category = prediction;

      // Получим таблицу с параметрами захвачеными с датчика, а также предсказанием цвета на базе этих параметров.
      String tableSensor = getSensorTable(signals, TIMESTEPS);
      // Передадим строку с данными для таблицы на вебстраничку, а также выведем её в последовательный порт.
      Serial.println(tableSensor);
      sendJson(jsonString, doc_tx, "table_string_sensor", tableSensor);
    }
  }
}
