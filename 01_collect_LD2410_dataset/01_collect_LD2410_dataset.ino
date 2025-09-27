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

// Данные WiFi сети.
const char* ssid = "WiFi_name";
const char* password = "WiFi_password";

// Экземпляр вебсервера ссылается на 80 порт.
AsyncWebServer server(80);
// Экземпляр WebSocket-сервера ссылается на 81 порт.
WebSocketsServer webSocket = WebSocketsServer(81);



/** 
В документе реализованы функции для работы с датчиком, а так же функции ответственные за обработку данных.
**/
#include "functions.h"


//-------------------------------------------------------------------------------------------------------------------
// Переменная отображает движение выбранное в элементе select (списке движений для детекции) на вебстраничке.
String selected_category = "unknown";
// Переменная отображает имя файла выбранного в элементе select (списке файлов с данными) на вебстраничке.
String selected_file = "";

// Переменная отвечает за создание нового файла.
bool newTXT = true;
// Переменная отображает состояние кнопки 'BTN_Detect' отвечающей за захват данных с датчика.
bool detect = false;
// Переменная отображает состояние кнопки 'BTN_Download' отвечающей за скачивание датасета.
bool download = false;
// Переменная отображает состояние кнопки 'BTN_Delete' отвечающей за удаление файла с данными.
bool delete_row = false;
//-------------------------------------------------------------------------------------------------------------------



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

  // Инициализация файловой системы SPIFFS.
  SPIFFS_init();

  // Инициализация EEPROM.
  EEPROM.begin(EEPROM_SIZE);

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

  // Если на вебстраничке была нажата кнопка 'BTN_Detect' отвечающая за захват данных с датчика, то выполняется следующий блок кода.
  if(detect){
    // Если новый файл ещё не был создан,
    if(newTXT){
      // Переведём переменную отвечающую за создание нового файла в состояние false до полного заполнения нового файла.
      newTXT = false;
      // Сгенерируем имя для нового файла.
      selected_file = getNewDatasetPath();
      Serial.println("New file name: " + selected_file);
      // Создадим новый файл с датасетом.
      createDataset_TXT(selected_file);
      // Отобразим пустую таблицу из нового файла на вебстраничке.
      String table_new = getTableString(selected_file);
      sendJson(jsonString, doc_tx, "table_string", table_new);
    }
    // Обновим select на вебстраничке с именами файлов.
    updateFilesSelect("files_list", jsonString, doc_tx);

    // Структура данных для хранения значений сигнала полученных с датчика LD2410.
    sensorSignal signal[1];

    // Таймер на millis() каждую секунду опрашивает датчик LD2410.
    if ((sensor.check() == MyLD2410::Response::DATA) && (millis() > nextPrint)) {
      // Получим текущее время (значение millis()).
      unsigned long time = millis();
      // Обновим время следующего срабатывания датчика.
      nextPrint = time + printEvery;
      Serial.println("Current row: " + String(iCount));
      // Увеличиваем значение cчётчика срабатывания таймера (индекс текущей строки).
      iCount++;

      // Функция опрашивает датчик LD2410 и возвращает данные с него.
      getData(sensor, signal, selected_category, time, iCount);

      // Добавить строку с новыми данными в созданый файл с датасетом  и вернуть String строку с таблицей.
      String table = addRow(selected_file, signal);
      // Передаём  String строку с таблицей на вебстраничку.
      sendJson(jsonString, doc_tx, "table_string", table);

      // Выводим данные со статичных гейтов в последовательный порт.
      Serial.println("STATIONARY: ");
      for (int j = 0; j < 9; j++) {
        Serial.print(String(signal[0].stationarySignals[j]) + ", ");
      }
      // Выводим данные с динамичных гейтов в последовательный порт.
      Serial.println();
      Serial.println("MOVING: ");
      for (int j = 0; j < 9; j++) {
        Serial.print(String(signal[0].movingSignals[j]) + ", ");
      }
      Serial.println();

      // Если заданное кол-во строк с данными было получено и записано в новый файл,
      if(iCount >= printCount){
        // Переведём переменную отвечающую за создание нового файла в состояние true.
        newTXT = true;
        // Останавливаем захват данных с датчика.
        detect = false;
        // Обнулим значение cчётчика срабатывания таймера (индекс текущей строки).
        iCount = 0;
        Serial.println("EXIT detect -------------------");
      }
  }
}

  // Если на вебстраничке была нажата кнопка 'BTN_Download' отвечающая за скачивание датасета, то выполняется следующий блок кода.
  if(download){
    // Переменная отображает состояние кнопки 'BTN_Download' отвечающей за скачивание датасета.
    download = false;

    // Открыть TXT файл с датасетом для чтения.
    File file_dataset_read = SPIFFS.open(selected_file);
    // Получаем размер файла
    unsigned int dataset_size  = file_dataset_read.size();

    // Выделяем память для буфера
    uint8_t *buf = (uint8_t *)malloc(dataset_size);
    // Чтение данных из файла в буфер
    file_dataset_read.read(buf, dataset_size);
    // Отправляем TXT файл с датасетом из памяти SPIFFS пользователю на вебстраничку.
    webSocket.broadcastBIN(buf, dataset_size);

    // Закрыть TXT файл с датасетом для чтения.
    file_dataset_read.close();
  }

  // Если на вебстраничке была нажата кнопка 'BTN_Delete' отвечающая за удаление файла, то выполняется следующий блок кода.
  if(delete_row){
    // Переменная отображает состояние кнопки 'BTN_Delete' отвечающая за удаление файла.
    delete_row = false;

    // Удалить файл выбранный в элементе select (списке файлов с данными) на вебстраничке.
    SPIFFS.remove(selected_file);
    // Обновим select на вебстраничке с именами файлов.
    updateFilesSelect("files_list", jsonString, doc_tx);
  }
}
