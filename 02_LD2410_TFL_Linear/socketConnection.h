// Выделить память для работы с Json(200 байт).
StaticJsonDocument<200> doc_tx; // для передачи данных.
StaticJsonDocument<200> doc_rx; // для приёма данных.
String jsonString = "";

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
      // Очищаем JSON документ.
      doc_tx.clear();
      /** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////
      // Указываем тип данных которые будем передавать в JSON строке.
      doc_tx["type"] = "table_string_sensor";
      // Передадим строку с наименованиями столбцов таблицы в JSON документ.
      doc_tx["value"] = "CATEGORY, MOVE_1, MOVE_2, MOVE_3, MOVE_4, MOVE_5, MOVE_6, MOVE_7, MOVE_8, MOVE_9, STAT_1, STAT_2, STAT_3, STAT_4, STAT_5, STAT_6, STAT_7, STAT_8, STAT_9, TIME, ID \n";
      // Сериализуем и выводим содержимое JSON-документа
      serializeJson(doc_tx, jsonString);
      // Передать строку с наименованиями столбцов таблицы на вебстраничку.
      webSocket.broadcastTXT(jsonString);
      // Очищаем JSON документ.
      doc_tx.clear();


      // Указываем тип данных которые будем передавать в JSON строке.
      doc_tx["type"] = "table_string_probability";
      // Передадим строку с наименованиями столбцов таблицы в JSON документ.
      doc_tx["value"] = "empty, squat, run, stand, jump \n";
      // Сериализуем и выводим содержимое JSON-документа
      serializeJson(doc_tx, jsonString);
      // Передать строку с наименованиями столбцов таблицы на вебстраничку.
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
          // Присвоим bool detect значение true или false чтобы в void loop() {} 
          // Выполнить блок кода отвечающий за захват информации с датчика и классификации сущностей на базе этой информации.
          detect = doc_rx["value"];
          Serial.println("Detect value: " + String(detect));
        }
      }
      Serial.println("");
      break;
  }
}