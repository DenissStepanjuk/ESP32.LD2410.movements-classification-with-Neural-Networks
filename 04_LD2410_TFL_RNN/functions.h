#include <cmath>

// Значение таймера при котором он должен сработать.
unsigned long nextPrint = 0;
// Промежуток времени для срабатывания таймера.
unsigned long printEvery = 1000;  
// Счётчик срабатываний таймера (индекс текущей строки).
int iCount = 0;

// Структура данных для хранения значений сигнала полученных с датчика LD2410.
struct sensorSignal {
  // Наименование для наблюдения.
  String category = "unknown";
  // LD2410 делит пространство перед собой на зоны (гейты), каждая из которых — это определённый диапазон расстояний.
  // Значения по каждому гейту для подвижных обьектов.
  //int move_1, move_2, move_3, move_4, move_5, move_6, move_7, move_8, move_9;
  float movingSignals[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  // Значения по каждому гейту для статичных обьектов.
  //int stat_1, stat_2, stat_3, stat_4, stat_5, stat_6, stat_7, stat_8, stat_9;
  float stationarySignals[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  // Время когда был получен сигнал с датчика.
  int time = 0;
  // Время когда был получен сигнал с датчика.
  int id = 0;
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





/** Функция записывает данные с датчика LD2410 в общий массив.
  - MyLD2410 sensor: абстрактный обьект для взаимодействия с датчиком LD2410.
  - sensorSignal signal[]: Структура данных для хранения значений сигнала полученных с датчика LD2410.
  - int height: Кол-во строк (векторов) передаваемых на вход модели.
  - String category: Наименования движения для детекции.
  - unsigned long time: Время захвата данных.
  - int id: Индекс строки в таблице.
**/
void moveSignal(MyLD2410 sensor, sensorSignal signal[], int height, String category, unsigned long time, int id) {
  // Смещаем данные в общем массиве вверх.
  for (int i = 1; i < height; i++) {
    signal[height - i] = signal[height - i - 1];
  }

  // Добавляем строку со свежими данными с датчика LD2410 в общий массив.
  getData(sensor, signal, category, time, id);
}





/** Функция возвращает наименование категории захваченой датчика.
  int kCategoryCount - Кол-во классов предсказываемых моделью.
  int8_t probabilities [] - Массив для хранения вероятностей для всех классов.
  char* kCategoryLabels[] - Массив наименований категорий, которые модель может классифицировать.   **/
String getPredictedColor(int kCategoryCount, float probabilities [], const char* kCategoryLabels[]){
  // Индекс для категории с наибольшей вероятностью.
  int idx = 0;
  // Максимальная вероятность.
  float maxProbability = probabilities[idx];

  // Пройдём через все сущности "категории" для которых модель возвращает вероятность.
  for (int i = 1; i < kCategoryCount; i++) {
    // Если вероятность для текущей сущности "категории" больше максимальная вероятности назначеной ранее,
    if(probabilities[i] > maxProbability){
      // Обновим максимальную вероятность.
      maxProbability = probabilities[i];
      // Обновим индекс для категории с наибольшей вероятностью.
      idx = i;
    }
  }
  // Вернём наименование категории с наибольшей вероятностью.
  return String(kCategoryLabels[idx]);
}




/** Функция возвращает строку на базе которой будет построена таблица с распределением вероятностей для каждого предсказываемого класса (категории).
  int kCategoryCount - Кол-во классов предсказываемых моделью.
  int8_t probabilities [] - Массив для хранения вероятностей для всех классов.
  char* kCategoryLabels[] - Массив наименований категорий, которые модель может классифицировать.   **/
String getProbabilitiesTable(int kCategoryCount, float probabilities [], const char* kCategoryLabels[]){
  // Строка содержащая наименование категорий.
  String row_1 = "";
  // Строка содержащая распределение вероятностей.
  String row_2 = "";
  // Заполним строки соответствующими значениями.
  for(int i = 0; i < kCategoryCount; i++){
    if(i != kCategoryCount-1){
      row_1 += String(kCategoryLabels[i]) +  ", ";
      row_2 += String(probabilities [i]) +  ", ";
    } else {
      row_1 += String(kCategoryLabels[i]) +  "\n";
      row_2 += String(probabilities [i]);
    }
  }

  // Создадим строку на базе которой будет построена таблица с распределением вероятностей для каждого предсказываемого класса (категории).
  String table = row_1 + row_2;
  // Вернём строку на базе которой будет построена таблица.
  return table;
}




/** Функция возвращает строку на базе которой будет построена таблица с параметрами захвачеными с датчика, а также предсказанием на базе этих параметров.
  sensorSignal signals[] - Структура данных для хранения значений сигналов полученных с датчика LD2410, которые будут переданы на вход модели.
  int height - Кол-во строк (векторов) передаваемых на вход модели.**/
String getSensorTable(sensorSignal signals[], int height){
  // Создадим строку на базе которой будет построена таблица с параметрами захвачеными с датчика, а также предсказанием на базе этих параметров.
  String table = String("PRED_CAT, MOVE_1, MOVE_2, MOVE_3, MOVE_4, MOVE_5, MOVE_6, MOVE_7, MOVE_8, MOVE_9, STAT_1, STAT_2, STAT_3, STAT_4, STAT_5, STAT_6, STAT_7, STAT_8, STAT_9, TIME, ID \n");

  // Пройдём по всем строкам (векторам) передаваемым на вход модели.
  for (int i = 1; i <= height; i++) {
    // Передадим в таблицу предсказания, отдельно выделим текущее предсказание.
    if(i == height){
      table += "CURRENT: " + signals[height - i].category  +  ", ";
    } else{
      table += signals[height - i].category  +  ", ";
    }

    // Передадим в таблицу значения по каждому гейту для подвижных обьектов.
    for (int u = 0; u < 9; u++) {
      table += String(signals[height - i].movingSignals[u]) +  ", ";
    }
    // Передадим в таблицу значения по каждому гейту для статичных обьектов.
    for (int u = 0; u < 9; u++) {
      table +=  String(signals[height - i].stationarySignals[u]) +  ", ";
    }

    // Добавим время захвата сигнала и индекс текущей строки.
    table += String(signals[height - i].time) + ", " + String(signals[height - i].id) + " \n";
  }

  // Вернём строку на базе которой будет построена таблица.
  return table;
}