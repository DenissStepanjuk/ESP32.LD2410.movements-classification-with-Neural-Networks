// Значение таймера при котором он должен сработать.
unsigned long nextPrint = 0;
// Промежуток времени для срабатывания таймера.
unsigned long printEvery = 1000;  
// Кол-во срабатываний таймера.
int printCount = 60;
// Счётчик срабатываний таймера (индекс текущей строки).
int iCount = 0;


// Структура данных для хранения значений сигнала полученных с датчика LD2410.
struct sensorSignal {
  // Наименование для наблюдения.
  String category = "unknown";
  // LD2410 делит пространство перед собой на зоны (гейты), каждая из которых — это определённый диапазон расстояний.
  // Значения по каждому гейту для подвижных обьектов.
  //int move_1, move_2, move_3, move_4, move_5, move_6, move_7, move_8, move_9;
  int movingSignals[9];
  // Значения по каждому гейту для статичных обьектов.
  //int stat_1, stat_2, stat_3, stat_4, stat_5, stat_6, stat_7, stat_8, stat_9;
  int stationarySignals[9];
  // Время когда был получен сигнал с датчика.
  int time;
  // Время когда был получен сигнал с датчика.
  int id;
};


/** Функция возвращает данные с датчика LD2410.
  - MyLD2410 sensor: абстрактный обьект для взаимодействия с датчиком LD2410.
  - sensorSignal signal[]: Структура данных для хранения значений сигнала полученных с датчика LD2410.
  - String category: Наименования движения для детекции.
  - unsigned long time: Время захвата данных.
  - int id: Индекс строки в таблице.
**/
void getData(MyLD2410 sensor, sensorSignal signal[], String category, unsigned long time, int id) {
  // Если связь с датчиком была установлена
  if (sensor.stationaryTargetDetected() || sensor.movingTargetDetected()) {
    if (sensor.inEnhancedMode()) {
      // Сохраним наименование движения для детекции.
      signal[0].category = category;

      // Сохраним данные со статичных гейтов.
      int i = 0;
      sensor.getStationarySignals().forEach([&](const byte &val) {
        if (i < 9) signal[0].stationarySignals[i++] = val;});

      // Сохраним данные с динамичных гейтов.
      i = 0;
      sensor.getMovingSignals().forEach([&](const byte &val) {
        if (i < 9) signal[0].movingSignals[i++] = val;});

      // Сохраним время захвата данных.
      signal[0].time = time;
      // Сохраним индекс строки.
      signal[0].id = id;
    }
  }
}



// Библиотека EEPROM дл работы с энергонезависимой памятью.
#include "EEPROM.h"

// Выделяем 1 байт памяти EEPROM куда будет записываться кол-во текстовых файлов с датасетом.
#define EEPROM_SIZE 1
// Счётчик кол-ва текстовых файлов с датасетом.
unsigned int datasetNumb = 0;

/* Функция генерирует новое имя файла обращаясь к EEPROM чтобы знать кол-во уже сгенерированых файлов.*/
String getNewDatasetPath() {
  // Счётчик кол-ва текстовых файлов с датасетом.
  datasetNumb = EEPROM.read(0) + 1;
  // Новое имя файла.
  String path = "/LD2410_dataset_" + String(datasetNumb) + ".txt";
  // Обновляем значение в EEPROM.
  EEPROM.write(0, datasetNumb);
  EEPROM.commit();

  return path;
}





/** SPIFFS — это файловая система, предназначенная для флэш-устройств SPI NOR на встроенных объектах. 
Она поддерживает выравнивание износа, проверку целостности файловой системы и многое другое. **/
#include <SPIFFS.h>

/* Функция заполняет select на вебстраничке именами файлов с данными.
  - String type - тип данных, которые помогают определить на вебстраничке что пришло.
  - String jsonString - Строка в которую запишем данные для передачи.
  - StaticJsonDocument<200> doc  - Память выделенная для работы с Json(200 байт).*/
void updateFilesSelect(String type, String jsonString, StaticJsonDocument<200> doc) {
  // Открывает корневую директорию файловой системы SPIFFS.
  File root = SPIFFS.open("/");
  // Получает первый файл из директории.
  File file = root.openNextFile();

  // Если файлов в корневой директории нет,
  if(!file){
    // то возвращаем пустой список,
    Serial.println("NO FILES");
    return;
  }

  // Очищаем JSON документ.
  doc.clear();
  // Указываем тип данных которые будем передавать в JSON строке.
  doc["type"] = type;
  
  // Цикл перебора всех оставшихся файлов в корневой директории.
  while(file){
    // Запишем имя файла в выделеную память.
    doc["/" + String(file.name())] = "/" + String(file.name());
    // Переходит к следующему файлу.
    file = root.openNextFile();

  }
  // Закрыть корневую директорию файловой системы SPIFFS.
  root.close();
  // Закрыть файл.
  file.close();

  // Сериализуем и выводим содержимое JSON-документа
  serializeJson(doc, jsonString);
  // Передать список цветов пользователю в select.
  webSocket.broadcastTXT(jsonString);
  // Очищаем JSON документ.
  doc.clear();
}






/** Функция инициализации для файловой системы SPIFFS. Инициализация должна быть проведена в void setup() {}.**/
void SPIFFS_init() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
}


/** Функция создаёт файл и первую строку 
  - String path: имя файла, который нужно создать.
**/
void firstLine(String path) {
  // Выводит в сериал-монитор сообщение о начале создания файла.
  Serial.println("Create file: " + path);
  // Открывает или создаёт файл для записи по указанному пути.
  File file_dataset = SPIFFS.open(path, FILE_WRITE);

  // Пытается записать в файл первую строку заголовка CSV.
  if(file_dataset.print("CATEGORY, MOVE_1, MOVE_2, MOVE_3, MOVE_4, MOVE_5, MOVE_6, MOVE_7, MOVE_8, MOVE_9, STAT_1, STAT_2, STAT_3, STAT_4, STAT_5, STAT_6, STAT_7, STAT_8, STAT_9, TIME, ID \n")){
    // Если запись успешна, выводит сообщение об успешном создании.
    Serial.println("File " + path + " is created.");
  } else {
    // Если запись не удалась, выводит сообщение об ошибке.
    Serial.println("File " + path + " is NOT created.");
  }
  // Закрывает файл, чтобы сохранить изменения.
  file_dataset.close();
}


/** Функция создаёт файл  с датасетом.
  - String path: имя файла, который нужно создать.**/
void createDataset_TXT(String path) {
  // Открывает корневую директорию файловой системы SPIFFS.
  File root = SPIFFS.open("/");
  // Получает первый файл из директории.
  File file = root.openNextFile();
  // Если файлов в корневой директории нет — сразу создаёт новый файл с заголовком.
  if(!file){
    firstLine(path);
  }

  // Цикл перебора всех файлов в корневой директории.
  while(file){
    // Печатает имя текущего файла в сериал-монитор.
    Serial.print("FILE: ");
    Serial.println(file.name());

    // Сравнивает путь текущего файла с заданным.
    if(String(String('/')+ String(file.name())) == path){
      // Если файл с нужным именем найден — сообщает об этом и выходит из цикла.
      Serial.println("This file " + path + " was found.");
      break;
    } else {
      // Если имя не совпадает — сообщает об этом.
      Serial.println("This file " + path + " was NOT found.");
    }
    // Переходит к следующему файлу.
    file = root.openNextFile();
    // Если файлов больше нет — создаёт файл с заголовком.
    if(!file){
      firstLine(path);
    }
  }
  root.close();
  file.close();
}



/** Функция достаёт файл с датасетом(таблицей) из  памяти SPIFFS, конвертирует в String строку и возвращает её.
  - String path: имя файла, который нужно открыть.**/
String getTableString(String path) {
  // Открыть TXT файл с датасетом для чтения.
  File file_dataset_read = SPIFFS.open(path);
  // String в который будет записан датасет из файла.
  String table = "";

  // Запишем датасет из файла в String table.
  while(file_dataset_read.available()) {
    table = file_dataset_read.readString();
  }
  // Закрыть TXT файл с датасетом для чтения.
  file_dataset_read.close();
  // Возвращаем таблицу в виде String строки.
  return table;
}


/** Функция достаёт файл с датасетом(таблицей) из памяти SPIFFS, конвертирует в String строку, добавляет к таблице 
строку с новыми данными, обновляет таблицу в памяти SPIFFS и возвращает String строку.
  - String path: имя файла, который нужно открыть и добвить строку.
  - sensorSignal signal[]: Структура данных храненящая значения сигнала полученные с датчика LD2410.**/
String addRow(String path, sensorSignal signal[]) {
  // Получить String строку с таблицей из памяти SPIFFS.
  String table = getTableString(path);
  // Получить индекс последней запятой во всём файле.
  int comma = table.lastIndexOf(',');
  // Зная индекс последней запятой получим индекс последнего экземпляра данных из таблицы.
  String idxString = table.substring(comma+2);
  int idx = idxString.toInt();

  // Открыть TXT файл с датасетом для записи.
  File file_dataset_write = SPIFFS.open(path, FILE_WRITE);
  // Добавим к таблице строку с новыми данными.
  table += String(signal[0].category) + ", ";

  // Добавим данные со статичных гейтов.
  for (int j = 0; j < 9; j++) {
    //Serial.print(String(signal[0].movingSignals[j]) + ", ");
    table += String(signal[0].movingSignals[j]) + ", ";
    }

  // Добавим данные с динамичных гейтов.
  for (int j = 0; j < 9; j++) {
    //Serial.print(String(signal[0].stationarySignals[j]) + ", ");  +" \n";
    table += String(signal[0].stationarySignals[j]) + ", ";
    }

  // Добавим время захвата сигнала и индекс текущей строки.
  table += String(signal[0].time) + ", " + String(signal[0].id) + " \n";

  // Записать обновлённую таблицу в файл.
  if(file_dataset_write.print(table)){
    Serial.println("Row to " + path + " is added.");
  } else {
    Serial.println("Row to " + path + " is NOT added.");
  }
  // Закрыть TXT файл с датасетом для записи.
  file_dataset_write.close();

  // Возвращаем таблицу в виде String строки.
  return table;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Не используется:

/** Функция достаёт файл с датасетом(таблицей) из памяти SPIFFS, конвертирует в String строку, удаляет из таблицы
строку с заданым индексом, обновляет таблицу в памяти SPIFFS и возвращает String строку.**/
String deleteRowByIdx(String path, int idx) {
  // Получить String строку с таблицей из памяти SPIFFS.
  String table = getTableString(path);

  // String в который будет записан датасет с удалённой строкой.
  String shortMemory = "";

  // В цикле while пройдём по всем строкам таблицы.
  // Индекс первого элемента текущей строки в String table.
  int start = 0;
  while (start < table.length()) {
    // Индекс последнего элемента текущей строки в String table.
    int end = table.indexOf('\n', start);
    // Если текущая строка последняя в String table.
    if (end == -1) end = table.length();

    // Текущая строка в String table.
    String line = table.substring(start, end);
    //line.trim(); // убрать лишние пробелы

    // Индекс последней запятой в текущей строке.
    int lastComma = line.lastIndexOf(',');
    // Проверка существует ли запятая.
    if (lastComma != -1) {
      // Получить индекс текущей строки в формате String.
      String idxStr = line.substring(lastComma + 2);
      //indexStr.trim();
      // Если индекс текущей строки не равен индексу строки, которую надо удалить,
      if (idxStr.toInt() != idx) {
        // Оставим текущую строку в таблице.
        shortMemory += line + "\n";
      }
    }
    // Обновим индекс первого элемента текущей строки в String table.
    start = end + 1;
  }

  // Открыть TXT файл с датасетом для записи.
  File file_dataset_write = SPIFFS.open(path, FILE_WRITE);

  // Записать обновлённую таблицу в файл.
  if(file_dataset_write.print(shortMemory)){
    Serial.println("Row with index " + String(idx) + " was deleted.");
  } else {
    Serial.println("Row with index " + String(idx) + " was NOT deleted.");
  }
  // Закрыть TXT файл с датасетом для записи.
  file_dataset_write.close();

  // Возвращаем таблицу в виде String строки.
  return shortMemory;
}



int getExampleIndex(String row) {
  // Получим индекс запятой в строке.
  int comma = row.indexOf(',');
  // Получим из всей строки число.
  String idxString = row.substring(0, comma);
  // Полученное число переведём в формат int.
  int idx = idxString.toInt();
  return idx;
}


String getRowByIdx(String table, int idx) {

  int start = 0;
  while (start < table.length()) {
    int end = table.indexOf('\n', start);
    if (end == -1) end = table.length(); // последняя строка

    String line = table.substring(start, end);
    line.trim(); // убрать лишние пробелы

    int lastComma = line.lastIndexOf(',');
    if (lastComma != -1) {
      String indexStr = line.substring(lastComma + 1);
      indexStr.trim();
      if (indexStr.toInt() == idx) {
        return line;
      }
    }

    start = end + 1;
  }

  return ""; // если не найдено
}






















/* Функция отпр
String getFirstFilePath() {


  // Открывает корневую директорию файловой системы SPIFFS.
  File root = SPIFFS.open("/");
  // Получает первый файл из директории.
  File file = root.openNextFile();

    // Если файлов в корневой директории нет,
  if(!file){
    // то возвращаем пустой список,
    Serial.println("NO FILES");
    return "NO FILES";
  }

  String pathh = "/" + String(file.name());

  

  root.close();
  file.close();

  return pathh;

}*/



/**

void printValue(const byte &val) {
  Serial.print(' ');
  Serial.print(val);
}

/** Функция для вывода данных полученных с датчика LD2410.
void printData() {
  //Serial.println("------------NEW------------");
  Serial.print(sensor.statusString());
  if (sensor.presenceDetected()) {
    Serial.print(", distance: ");
    Serial.print(sensor.detectedDistance());
    Serial.print("cm");
  }

  Serial.println();

  if (sensor.stationaryTargetDetected() || sensor.movingTargetDetected()) {
    if (sensor.inEnhancedMode()) {

      int stationarySignalsTRUE[9];
      int movingSignalsTRUE[9];

      Serial.println(" STATIONARY: ");

      Serial.print("signals->[");
      //sensor.getStationarySignals().forEach(printValue);

      int i = 0;
      sensor.getStationarySignals().forEach([&](const byte &val) {
        if (i < 9) stationarySignalsTRUE[i++] = val;
      });


      for (int j = 0; j < 9; j++) {
        Serial.print(stationarySignalsTRUE[j]);
        Serial.print(' ');
      }



      Serial.print(" ] thresholds:[");
      sensor.getStationaryThresholds().forEach(printValue);
      Serial.print(" ]");

      Serial.println();
      Serial.println(" MOVING: ");


      Serial.print("signals->[");
      //sensor.getMovingSignals().forEach(printValue);

      i = 0;
      sensor.getMovingSignals().forEach([&](const byte &val) {
        if (i < 9) movingSignalsTRUE[i++] = val;
      });

      for (int j = 0; j < 9; j++) {
        Serial.print(movingSignalsTRUE[j]);
        Serial.print(' ');
      }


      Serial.print(" ] thresholds:[");
      sensor.getMovingThresholds().forEach(printValue);
      Serial.print(" ]");
    }
    Serial.println();
  }
}






// Структура данных для хранения образцов цвета полученных с датчика.
struct Color {
  // Наименование цвета.
  String name;
  // Значения компонент цвета.
  int r, g, b, w;
};

**/


/** Функция для подсчёта кол-ва файлов записаных во внутрению память микроконтролера SPIFFS.
    Возвращает список файлов записанных во внутрению память микроконтролера SPIFFS. 
    int &list_Len - кол-ва файлов записаных во внутрению память микроконтролера SPIFFS.
String [] getFilesList(int &list_Len) {
  // Счётчик кол-ва файлов.
  int filesCounter = 0;

  // Открывает корневую директорию файловой системы SPIFFS.
  File root = SPIFFS.open("/");
  // Получает первый файл из директории.
  File file = root.openNextFile();

  // Если файлов в корневой директории нет,
  if(!file){
    // то возвращаем пустой список,
    Serial.println("NO FILES");
    list_Len = 1;
    return {"NO FILES"};
  } else {
    // иначе учитываем первый файл.
    Serial.print("FILE: ");
    Serial.println(file.name());
    filesCounter++;
  }

  // Цикл перебора всех оставшихся файлов в корневой директории.
  while(file){
    // Печатает имя текущего файла в сериал-монитор.
    Serial.print("FILE: ");
    Serial.println(file.name());
    // Учитываем текущий файл.
    filesCounter++;
    // Переходит к следующему файлу.
    file = root.openNextFile();
  }
  // Создаём массив для хранения списка файлов записанных во внутрению память микроконтролера SPIFFS. 
  String filesList[filesCounter];

  // Заполняем массив списком файлов записанных во внутрению память микроконтролера SPIFFS. 
  for (int j = 0; j < filesCounter; j++) {
    file = root.openNextFile();
    filesList[filesCounter] = file.name();
  }

  // Возвращаем список файлов.
  list_Len = filesCounter;
  return filesList;
}



**/