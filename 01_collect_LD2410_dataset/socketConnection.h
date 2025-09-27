// Выделить память для работы с Json(200 байт).
StaticJsonDocument<200> doc_tx; // для передачи данных.
StaticJsonDocument<200> doc_rx; // для приёма данных.
String jsonString = "";

// Кол-во движений для детекции и классификации.
const int categoriesCount = 6;
// Наименования движений для детекции и классификации.
String categories[categoriesCount] = {"unknown", "empty", "stand", "run", "jump", "squat"};


// Функция отправляет данные на вебстраничку к клиенту.
void sendJson(String jsonString ,StaticJsonDocument<200> doc, String l_type, String l_value) {
    // Создать JSON обьект.
    JsonObject object = doc.to<JsonObject>();
    // Записать данные в JSON обьект.
    object["type"] = l_type;
    object["value"] = l_value;
    // Конвертировать JSON обьект в строку.
    serializeJson(doc, jsonString);
    // Отправить данные на вебстраничку к клиенту.
    webSocket.broadcastTXT(jsonString);
    // Очистить JSON документ.
    doc.clear();
}


/** Функция заполняет select на вебстраничке наименованиями движений, которые будем детектировать.
  - String elements[] - Наименования движений для детекции и классификации.
  - String type - тип данных, которые помогают определить на вебстраничке что пришло.
  - int selectSize - Кол-во движений для детекции и классификации.
  - String jsonString - Строка в которую запишем данные для передачи.
  - StaticJsonDocument<200> doc  - Память выделенная для работы с Json(200 байт).**/
void updateSelect(String elements[], String type, int selectSize, String jsonString, StaticJsonDocument<200> doc) {
  // Очищаем JSON документ.
  doc.clear();
  // Указываем тип данных которые будем передавать в JSON строке.
  doc["type"] = type;
  // Передаём список движений в JSON документ.
  for(int i = 0; i < selectSize; i++){
    doc[elements[i]] = elements[i];
    }
  // Сериализуем и выводим содержимое JSON-документа
  serializeJson(doc, jsonString);
  // Передать список движений пользователю в select.
  webSocket.broadcastTXT(jsonString);
  // Очищаем JSON документ.
  doc.clear();
}


/** Функция webSocketEvent обработывает данные полученные от клиента через соеденение вебсокетов.
    - byte num (номер клиента)
    - WStype_t type (тип данных принятых от клиента)
    - uint8_t * payload (данные принятые от клиента)
    - size_t length (длинна принятых данных)**/
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
  // В зависимости от типа принятых данных выполнить соответствующий блок кода.
  switch (type) {

    // Обработка отключения клиента:
    case WStype_DISCONNECTED: // Если клиент отключился, выполнить следующий блок кода.
      Serial.println("Client " + String(num) + " disconnected");
      break;

    // Обработка подключения клиента:
    case WStype_CONNECTED:    // Если клиент подключился, выполнить следующий блок кода.
      Serial.println("Client " + String(num) + " connected");

      // Заполняем select на вебстраничке наименованиями движений, которые будем детектировать.
      updateSelect(categories, "categories_list", categoriesCount, jsonString, doc_tx);
      // Заполняем select на вебстраничке именами файлов с данными.
      updateFilesSelect("files_list", jsonString, doc_tx);

      /** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////
      // Указываем тип данных которые будем передавать в JSON строке.
      doc_tx["type"] = "table_string";
      // Достанем таблицу из файла и передадим таблицу в JSON документ.
      doc_tx["value"] = getTableString(selected_file);
      // Сериализуем и выводим содержимое JSON-документа
      serializeJson(doc_tx, jsonString);
      // Передать таблицу на вебстраничку.
      webSocket.broadcastTXT(jsonString);
      // Очищаем JSON документ.
      doc_tx.clear();
      /** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////
      break;

    // Обработка текстовых данных, отправленных клиентом:
    case WStype_TEXT:   // Если клиент отправил текстовые данные, обработать их.
      // Записать данные переданные от клиента "payload" в памяти контролера "doc_rx".
      DeserializationError error = deserializeJson(doc_rx, payload);
      // Если произошла ошибка при записи, вывести сообщение об ошибке.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      // При успешной записи JSON строки в память контролера обработать её.
      else {
        // Выведим пользователя от которого были приняты данные.
        Serial.println("Received from user: " + String(num));
        // Определим тип JSON строки обратившись к ней по ключу ["type"].
        const char* msg_type = doc_rx["type"];
        Serial.println("Type: " + String(msg_type));

        // Исходя из типа принятой JSON строки выполним соответствующий блок кода.
        if(String(msg_type) == "detect") {
          // Присвоим bool detect значение true чтобы в void loop() {} 
          // выполнить блок кода отвечающий за захват цвета с датчика.
          detect = doc_rx["value"];
          Serial.println("Detect: " + String(detect));
        }

        if(String(msg_type) == "download") {
          // Присвоим bool download значение true чтобы в void loop() {} 
          // выполнить блок кода отвечающий за скачивание датасета с вебстранички.
          download = doc_rx["value"];
          Serial.println("Download: " + String(download));
        }

        if(String(msg_type) == "selected_category") {
          // Присвоим selected_category текстовое значение категории, которую хотим детектировать
          // чтобы при формировании новой таблицы было понятно захват каких данных будет производится.
          selected_category = String(doc_rx["value"]);
          Serial.println("Selected category: " + selected_category);
        }

        if(String(msg_type) == "selected_file") {
          // Присвоим selected_file текстовое значение файла, который был выбран на вебстраничке в соответствующем select.
          selected_file = String(doc_rx["value"]);
          Serial.println("Selected file: " + selected_file);
          // Создадим таблицу с данными хранящимеся в файле чьё имя было выбрано на вебстраничке.
          String table_update = getTableString(selected_file);
          // Обновим таблицу на вебстраничке.
          sendJson(jsonString, doc_tx, "table_string", table_update);
        }
        if(String(msg_type) == "delete") {
          // Присвоим idx_delete значение индекса полученого с вебстранички строки которую хотим удалить. РУДИМЕНТ! НЕ ХОЧУ МЕНЯТЬ!
          //idx_delete = int(doc_rx["value"]);
          // Присвоим bool delete_row значение true чтобы в void loop() {} выполнить блок кода отвечающий за удалние файла с датасетом.
          delete_row = true;
          Serial.println("Delete: " + selected_file);
        }
      }
      Serial.println("");
      break;
  }
}