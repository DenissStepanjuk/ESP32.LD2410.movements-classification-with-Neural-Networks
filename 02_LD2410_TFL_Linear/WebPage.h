// Функция подготавливает и возвращает HTML страничку.
String getHTML() {

  // Строка в которую записана HTML страничка.
  String html = ""
"  <!DOCTYPE html>"
"  <html>"
    /** head **//////////////////////////////////////////////////////////////////////////////////////////////////////
"    <head>"
"      <meta charset='UTF-8'>"
"      <title>ESP32 + LD2410:</title>"
 /** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////
"  <style>"
"    table {"
"      border-collapse: collapse;"
"      width: 100%;"
"    }"
"    th, td {"
"      border: 1px solid #aaa;"
"      padding: 8px;"
"      text-align: center;"
"    }"
"    th {"
"      background-color: #eee;"
"    }"
"  </style>"
 /** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////
"    </head>"



    /** body **//////////////////////////////////////////////////////////////////////////////////////////////
"    <body style='background-color: #EEEEEE;'>"
"      <div>ESP32 + LD2410: TensorFlow Linear model for classification of movements: </div>"
      /** Кнопки:
            - DETECT START - Начать захват параметров с датчика для классификации.
            - DETECT STOP - Остановить захват параметров с датчика для классификации.
            - REFRESH PAGE - Перезагрузить страницу.**/
"      <p><button type='button' id='BTN_Detect_Start'> DETECT START </button></p>"
"      <p><button type='button' id='BTN_Detect_Stop'> DETECT STOP </button></p>"
"      <p><button onclick='location.reload();'>REFRESH PAGE</button></p>"

/** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////
// Таблица с распределением вероятностей для каждого предсказываемого класса.
"      <div><b>Data from LD2410: </b></div>"
"      <div id='table-container-sensor'></div>"
// Таблица с параметрами захвачеными с датчика, а также предсказанием категорий= на базе этих параметров.
"      <div><b>Probability: </b></div>"
"      <div id='table-container-probability'></div>"

/** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////

"    </body>"

    /** script **///////////////////////////////////
"    <script>"

/** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////
// Функция создаёт таблицу на вебстраничке из String строки.
"function createTableFromString(dataString, containerId) {"
"    const lines = dataString.trim().split(`\n`).filter(line => line.trim().length > 0);"
"    const headers = lines[0].split(',').map(h => h.trim());"
"    const rows = lines.slice(1).map(line => line.split(',').map(cell => cell.trim()));"

"    const table = document.createElement('table');"

"    const thead = document.createElement('thead');"
"    const headerRow = document.createElement('tr');"
"    headers.forEach(header => {"
"      const th = document.createElement('th');"
"      th.textContent = header;"
"      headerRow.appendChild(th);"
"    });"
"    thead.appendChild(headerRow);"
"    table.appendChild(thead);"

"    const tbody = document.createElement('tbody');"
"    rows.forEach(rowData => {"
"      const tr = document.createElement('tr');"
"      rowData.forEach(cellData => {"
"        const td = document.createElement('td');"
"        td.textContent = cellData;"
"        tr.appendChild(td);"
"      });"
"      tbody.appendChild(tr);"
"    });"
"    table.appendChild(tbody);"

"    const container = document.getElementById(containerId);"
"    container.innerHTML = '';"// Очистить контейнер перед вставкой
"    container.appendChild(table);"
"  }"

/** TABLE **//////////////////////////////////////////////////////////////////////////////////////////////////////
      // Создаём экземпляр вебсокета.
"      var Socket;"
      // Создаём элемент select (список цветов для детекции).
"      let select = document.createElement('select');"

      /** При каждом клике по кнопке (DETECT START [id: BTN_Detect_Start]) вызывать функцию button_detect_start.**/
"      document.getElementById('BTN_Detect_Start').addEventListener('click', button_detect_start);"

      /** Функция (DETECT START [id: BTN_Detect_Start]) - отправляет JSON строку через соеденение вебсокетов при клике по соответствующей кнопке.**/
"      function button_detect_start() {"
        // JSON строка даст знать контролеру, что надо захватить параметры цвета с датчика и добавить в датасет.
"        var btn_click_json_msg = {type: 'detect', value: true};"
        // Отправить JSON строку через соеденение вебсокетов на контроллер.
"        Socket.send(JSON.stringify(btn_click_json_msg));"
"      }"



      /** При каждом клике по кнопке (DETECT STOP [id: BTN_Detect_Stop]) вызывать функцию button_detect.**/
"      document.getElementById('BTN_Detect_Stop').addEventListener('click', button_detect_stop);"

      /** Функция (DETECT STOP [id: BTN_Detect_Stop]) - отправляет JSON строку через соеденение вебсокетов при клике по соответствующей кнопке.**/
"      function button_detect_stop() {"
        // JSON строка даст знать контролеру, что надо захватить параметры цвета с датчика и добавить в датасет.
"        var btn_click_json_msg = {type: 'detect', value: false};"
        // Отправить JSON строку через соеденение вебсокетов на контроллер.
"        Socket.send(JSON.stringify(btn_click_json_msg));"
"      }"


      /** Функция инициализации вебсокета. -----------------------**/
"      function init() {"
        // Экземпляр вебсокета ссылается на 81 порт.
"        Socket = new WebSocket('ws://' + window.location.hostname + ':81/');"
        // При получении сообщения вебсокетом вызываем функцию ответного вызова.
"        Socket.onmessage = function(event) {"
          // Функция ответного вызова, которая обрабатывает полученное сообщение.
"          processCommand(event);"
"        };"
"      }"

    
      /** Функция обрабатывает данные полученные через соеденение вебсокетов.**/
"      function processCommand(event){"
        // Если на вход пришли данные типа — объект Blob (например, текстовый файл), то пользователь скачает его.
"        if (event.data instanceof Blob) {"
            // Создаем URL для объекта Blob
"            const url = URL.createObjectURL(event.data);"

            // Создаем временный элемент <a> для запуска скачивания
"            const a = document.createElement('a');"
"            a.href = url;"
"            a.download = 'DATASET_ESP32.txt';" // Имя сохраняемого файла
"            document.body.appendChild(a);" // Добавляем в DOM
"            a.click();" // Запускаем скачивание
"            document.body.removeChild(a);" // Удаляем из DOM

            // Освобождаем ресурсы, занятые URL
"            URL.revokeObjectURL(url);"
        // Если на вход пришли данные другого типа
"        } else {"
"          try {"
            // Принимаем json обьект и достаём из него данные.
"            var obj = JSON.parse(event.data);"
            // Определяем тип JSON строки.
"            var type = obj.type;"
      
            // Записываем в select имена всех фото что есть на карте памяти.
"           if(type.localeCompare(\"colors_list\") == 0){"
              // Очистка select
"              select.innerHTML = '';"
              // Заполнение select опциями на основе значений JSON-объекта
"              for (let key in obj) {"
                  // Игнорируем 'type'
"                if (key !== 'type') {"
"                  let option = document.createElement('option');"
"                  option.value = obj[key];"
"                  option.text = obj[key];"
"                  select.appendChild(option);"
"                }"
"              }"
              // Добавление select в контейнер.
"              document.getElementById('container').appendChild(select);"
"            }"
            // Заполним таблицу с параметрами захвачеными с датчика, а также предсказанием цвета на базе этих параметров.
"           else if(type.localeCompare(\"table_string_sensor\") == 0){"
"                 var table = obj.value;"
"                 createTableFromString(table, 'table-container-sensor');}"
            // Заполним таблицу с распределением вероятностей для каждого предсказываемого класса (цвета).
"           else if(type.localeCompare(\"table_string_probability\") == 0){"
"                 var table = obj.value;"
"                 createTableFromString(table, 'table-container-probability');}"
          // В случае ошибки
"          } catch (e) {"
            // вывести сообщение об ошибке в консоль браузера.
"          console.error(\"Received data is neither Blob nor valid JSON:\", event.data);"
"          }"
"        }"
"      }"
      
      /** Первым делом при подключении клиента к серверу должен быть инициализирован вебсокет. ---------------------**/
"      window.onload = function(event) {"
"        init();"
"      }"

"    </script>"

"  </html>";

  return html;
}




