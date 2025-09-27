#ifndef TENSORFLOW_LITE_CONFIG
#define TENSORFLOW_LITE_CONFIG

// TensorFlowLite_ESP32-----------------------------------------------------
// Библиотека позволяет развернуть модели машиного обучения на семействе микроконтроллеров ESP32.
//#include <TensorFlowLite_ESP32.h>
#include <MicroTFLite.h>
// Реализация ряда функций обработки данных моделью необходимых интерпретатору для запуска модели.
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
// Ряд функций предоставляющих отчёт об ошибках и отладочную информацию.
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
//#include "tensorflow/lite/micro/micro_error_reporter.h"
// Интерпретатор содержит код для загрузки и запуска модели без предвапительной компиляции.
// (выполняет программу построчно, читая каждую инструкцию и преобразуя её в исполняемый код)
#include "tensorflow/lite/micro/micro_interpreter.h"
// Cодержит схему преобразования модели в поток байтов для передачи в память на базе библиотеки FlatBuffers.
// (FlatBuffers — эффективная кроссплатформенная библиотека сериализации для C++, C#, C)
#include "tensorflow/lite/schema/schema_generated.h"
// Этот файл определяет общие типы данных и API для реализации операций, делегатов и других конструкций в TensorFlow Lite.
#include "tensorflow/lite/c/common.h"


// Кол-во строк (векторов) передаваемых на вход модели.
constexpr int height = 10;
// Кол-во элементов в одной строке (векторе) передаваемой на вход модели.
constexpr int wide = 18;

// Кол-во параметров передаваемых на вход модели.
constexpr int inputVectoSize = 180;

// Кол-во классов предсказываемых моделью.
constexpr int kCategoryCount = 5;

// Наименования категорий, которые модель может классифицировать.
const char* kCategoryLabels[kCategoryCount] = {"empty", "jump", "run", "squat", "stand"};

// Инициализировать необходимые структуры данных для работы с библиотекой Tensor Flow Lite.
// Обьект ErrorReporter предоставляет отчёт об ошибках и отладочную информацию.
tflite::ErrorReporter* error_reporter = nullptr;
// Обьект для хранения модели машиного обучения осуществляющей классификацию изображения.
const tflite::Model* model = nullptr;
// Обьект для хранения интерпретатора осуществляющего загрузку и запуск модели.
tflite::MicroInterpreter* interpreter = nullptr;
// Обьект (тензор) для хранения изображения передаваемого на вход модели для последующей классификации.
TfLiteTensor* input = nullptr;

// Обьём памяти, который необходимо выделить для хранения массивов модели.
// Для входного, выходного и промежуточных массивов модели.
//constexpr int kTensorArenaSize = 81 * 1024;
constexpr int kTensorArenaSize = 20 * 1024;
// Массив для хранения входных, выходных и промежуточных массивов модели.
static uint8_t *tensor_arena;//[kTensorArenaSize]; // Maybe we should move this to external

#endif  // TENSORFLOW_LITE_CONFIG