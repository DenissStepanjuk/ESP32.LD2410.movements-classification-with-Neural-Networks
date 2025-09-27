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

// TensorFlowLite_ESP32-----------------------------------------------------
// Модель машиного обучения.
#include "TensorFlowLiteModel.h"
// Параметры изображения передаваемого модели, а так же кол-во категорий для классификации.
#include "TensorFlowLiteModelConfig.h"
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
sensorSignal signals[height];

/** 
В документе реализлвана функция webSocketEvent для обработки данных полученых через соеденение сокетов. 
**/
#include "socketConnection.h"

void setup() {
  // TensorFlowLite_ESP32---------------------------------------------------------------------------------------------------------

  // Для ведения журнала ошибок создадим переменную "error_reporter" на базе предоставляемых библиотекой Tensor Flow Lite структур данных.
  //static tflite::MicroErrorReporter micro_error_reporter;
  static tflite::MicroErrorReporter micro_error_reporter;
  // Переменную "error_reporter" необходимо передать в интерпретатор, который будет в свою очередь передавать в неё список ошибок.
  error_reporter = &micro_error_reporter;


  // Создадим экземпляр модели используя массив данных из документа "TensorFlowLiteModel.h" 
  model = tflite::GetModel(model_TFLite);
  // Проверка соответствия версии модели и версии библиотеки.
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  // Выделить обьём памяти для входного, выходного и промежуточных массивов модели,
  if (tensor_arena == NULL) {
    // Выделить более медляную память, но большую по обьёму.
    //tensor_arena = (uint8_t*) ps_calloc(kTensorArenaSize, 1);
    // Выделить более быструю память, но меньшую по обьёму.
    tensor_arena = (uint8_t *) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    //tensor_arena = (uint8_t*) malloc(kTensorArenaSize);
  }
  // Если не удалось выделить обьём памяти для входного, выходного и промежуточных массивов модели, то вывести сообщение об ошибке.
  if (tensor_arena == NULL) {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }


  // Загрузить все методы, что содержит библиотека Tensor Flow Lite, для обработки данных моделью. (Занимает большой обьём памяти)
  // tflite::AllOpsResolver resolver;

  // Загрузить необходимые методы для обработки данных моделью из библиотеки Tensor Flow Lite.
  static tflite::MicroMutableOpResolver<9> micro_op_resolver;
  // AveragePool2D — операция, применяемая в свёрточных нейронных сетях (CNN), для уменьшения ширины и высоты входного тензора.
  micro_op_resolver.AddAveragePool2D();
  // MaxPool2D — операция в свёрточных нейронных сетях (CNN), которая выполняет подвыборку данных, уменьшая ширину и высоту входного тензора.
  micro_op_resolver.AddMaxPool2D();
  // Reshape — операция, используемая в машинном обучении и обработке данных, которая изменяет форму (размерность) тензора без изменения его данных
  micro_op_resolver.AddReshape();
  // FullyConnected (полносвязанный слой) — используется для выполнения нелинейных преобразований данных и играет важную роль в моделях глубокого обучения.
  micro_op_resolver.AddFullyConnected();
  // Conv2D (свёрточный слой) — выполняет операцию свёртки над входными данными, чтобы извлекать локальные признаки, использует их для построения более сложных представлений на следующих слоях.
  micro_op_resolver.AddConv2D();
  // DepthwiseConv2D — разновидность свёрточного слоя, которая применяется для увеличения вычислительной эффективности и уменьшения количества параметров модели.
  micro_op_resolver.AddDepthwiseConv2D();
  // Softmax — функция активации, которая используется в выходных слоях нейронных сетей для задач классификации.
  micro_op_resolver.AddSoftmax();
  // Quantize (квантование) — процесс преобразования данных или моделей глубокого обучения, чтобы снизить их размер и вычислительную сложность, сохраняя при этом приемлемую точность.
  micro_op_resolver.AddQuantize();
  // Dequantize (деквантование) — процесс обратного преобразования данных из квантованного формата обратно в формат с плавающей точкой или в более высокую точность. 
  micro_op_resolver.AddDequantize();


  // Создадим экземпляр интерпретатора передавав необходимые данные для запуска модели.
  static tflite::MicroInterpreter static_interpreter(
    model, micro_op_resolver, tensor_arena, kTensorArenaSize);
      //model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);

  interpreter = &static_interpreter;


  // Выделим память для внутрених тензоров модели из выделеной ранее памяти tensor_arena.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  // При неудачном выделении памяти сообщить об ошибке.
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Получить указатель на входной тензор модели.
  input = interpreter->input(0);
  // TensorFlowLite_ESP32---------------------------------------------------------------------------------------------------------

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
      moveSignal(sensor, signals, height, "unknown", time, iCount);

      // TensorFlowLite_ESP32---------------------------------------------------------------------------------------------------------
      // Входной тензор модели (вход модели).
      int8_t * input_data = input->data.int8;

      // Индекс текущего элемента входного тензора модели.
      int input_idx = 0;

      // Передадим данные на вход модели:
      for (int i = 1; i <= height; i++) {
        // Передадим на вход модели данные с динамичных гейтов для текущей строки:
        for (int u = 0; u < 9; u++) {
          // Отмасштабируем данные [0 : 100] --> [-128 : 127]
          input_data[input_idx] = (int8_t)(((signals[height - i].movingSignals[u]-50)/50)* 127.5);
          // Обновим индекс текущего элемента входного тензора модели.
          input_idx++;
        }
        // Передадим на вход модели данные со статичных гейтов для текущей строки:
        for (int u = 0; u < 9; u++) {
          // Отмасштабируем данные [0 : 100] --> [-128 : 127]
          input_data[input_idx] = (int8_t)(((signals[height - i].stationarySignals[u]-50)/50)* 127.5);
          // Обновим индекс текущего элемента входного тензора модели.
          input_idx++;
        }
      }


      // Вызвать модель (произвести преобразование входного изображения в вероятность принадлежности 
      // данного изображения к каждому из возможных классов).
      if (kTfLiteOk != interpreter->Invoke()) {
        TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed.");
      }

      // Получить выход модели.
      TfLiteTensor* output = interpreter->output(0);

      // Массив для хранения вероятностей для всех классов.
      int8_t probabilities [kCategoryCount];
      // Пройти по каждому элементу выхода модели.
      for(int i = 0; i < kCategoryCount; i++){
        // Получить вероятность для i-го класса.
        probabilities [i] = output->data.uint8[i];
      }

      // Получим таблицу с распределением вероятностей для каждого предсказываемого класса (цвета).
      String tableProbabilities = getProbabilitiesTable(kCategoryCount, probabilities, kCategoryLabels);
      // Передадим строку с данными для таблицы на вебстраничку, а также выведем её в последовательный порт.
      Serial.println(tableProbabilities);
      sendJson(jsonString, doc_tx, "table_string_probability", tableProbabilities);

      // Получим наименование предсказаного цвета.
      String prediction = getPredictedColor(kCategoryCount, probabilities, kCategoryLabels);
      signals[0].category = prediction;

      // Получим таблицу с параметрами захвачеными с датчика, а также предсказанием цвета на базе этих параметров.
      String tableSensor = getSensorTable(signals, height);
      // Передадим строку с данными для таблицы на вебстраничку, а также выведем её в последовательный порт.
      Serial.println(tableSensor);
      sendJson(jsonString, doc_tx, "table_string_sensor", tableSensor);
    }
  }
}
