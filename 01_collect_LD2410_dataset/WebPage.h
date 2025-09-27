// Функция подготавливает и возвращает HTML страничку.
String getHTML() {

  // Строка в которую записана HTML страничка.
  String html = ""
"  <!DOCTYPE html>"
"  <html>"
    /** head **//////////////////////////////////////////////////////////////////////////////////////////////////////
"    <head>"
"      <title>ESP32 + LD2410: collect movements dataset</title>"
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
"      <div>ESP32 + LD2410: collect movements dataset: </div>"

"      <div>Choose category: </div>"
"      <div style='display: flex; align-items: center; gap: 10px;'>"
//          - Кнопка DETECT - Захватить параметры с датчика LD2410 и добавить в датасет.
"            <button type='button' id='BTN_Detect'> DETECT </button>"
//          - Выпадающий список (select) движений для детекции.
"            <div id='container_categories'></div>"
"      </div>"

"      <div>Choose file: </div>"
      /** Выпадающий список (select) файлов содержащихся в памяти SPIFFS. **/
"      <div id='container_files'></div>"
      /** Кнопки:
        - DELETE - отвечающая за удаление файла.
        - DOWNLOAD - Скачать датасет.**/
"      <p><button type='button' id='BTN_Delete'> DELETE </button> <button type='button' id='BTN_Download'> DOWNLOAD </button></p>"

      /** Кнопки:
        - Окно для ввода индекса удаляемой строки.
        - REFRESH PAGE - перезагрузить страничку **/
//"      <input id='idx_delete' type='text' maxlength='14'>"
"      <p><button onclick='location.reload();'>REFRESH PAGE</button></p>"

// Таблица с параметрами захвачеными  с датчика.
"      <div id='table-container'></div>"
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
"      let select_categories = document.createElement('select');"
"      let select_files = document.createElement('select');"

      /** При каждом клике по кнопке (DETECT [id: BTN_Detect]) вызывать функцию button_detect.**/
"      document.getElementById('BTN_Detect').addEventListener('click', button_detect);"
      /** При каждом клике по кнопке (DOWNLOAD [id: BTN_Download]) вызывать функцию button_download.**/
"      document.getElementById('BTN_Download').addEventListener('click', button_download);"

      /** При каждом клике по кнопке 'BTN_Delete' вызывать функцию send_value.**/ 
"      document.getElementById('BTN_Delete').addEventListener('click', send_value);"
      /** Функция отправляет JSON строку через соединение сокетов при клике по кнопке 'BTN_Delete'.**/ 
"      function send_value() {"
      // Получаем значение из поля ввода
//"      var inputValue = document.getElementById('idx_delete').value;"
      // Формируем JSON-объект
"      var btn_del = {type: 'delete', value: 0};"
      // Отправляем JSON через WebSocket
"      Socket.send(JSON.stringify(btn_del));}"


      /** Обработка события выбора цвета в select. **/
"      select_categories.addEventListener('change', function () {"
        // Записать выбранный цвет в переменную selectedValue.
"        let selectedValue = select_categories.value;"
        // Вывести выбранный цвет в консоль браузера.
"        console.log('Selected value:', selectedValue);"
        // JSON строка даст знать контролеру, какой цвет был выбран пользователем для детекции
"        var msg = {type: 'selected_category', value: selectedValue};"
        // Отправить JSON строку через соеденение вебсокетов на контроллер.
"        Socket.send(JSON.stringify(msg));"
"      });"



      // Переменная хранит имя файла с датасетом.
"      let datasetName = 'DATASET_ESP32_LD2410.txt';"

      /** Обработка события выбора цвета в select. **/
"      select_files.addEventListener('change', function () {"
        // Записать выбранный цвет в переменную selectedValue.
"        let selectedValue = select_files.value;"
        // Вывести выбранный цвет в консоль браузера.
"        console.log('Selected value:', selectedValue);"
        // JSON строка даст знать контролеру, какой цвет был выбран пользователем для детекции
"        var msg = {type: 'selected_file', value: selectedValue};"
        // Отправить JSON строку через соеденение вебсокетов на контроллер.
"        Socket.send(JSON.stringify(msg));"

"        datasetName = selectedValue.slice(1);"
"      });"



      /** Функция (DETECT [id: BTN_Detect]) - отправляет JSON строку через соеденение вебсокетов при клике по соответствующей кнопке.**/
"      function button_detect() {"
        // JSON строка даст знать контролеру, что надо захватить параметры цвета с датчика и добавить в датасет.
"        var btn_click_json_msg = {type: 'detect', value: true};"
        // Отправить JSON строку через соеденение вебсокетов на контроллер.
"        Socket.send(JSON.stringify(btn_click_json_msg));"
"      }"

      /** Функция (DOWNLOAD [id: BTN_Download]) - отправляет JSON строку через соеденение вебсокетов при клике по соответствующей кнопке.**/
"      function button_download() {"
        // JSON строка даст знать контролеру, что пользователь хочет скачать датасет.
"        var btn_click_json_msg = {type: 'download', value: true};"
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
//"            a.download = 'DATASET_ESP32.txt';" // Имя сохраняемого файла
"            a.download = datasetName;" // Имя сохраняемого файла
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
      
            // Записываем в select наименование всех цветов с которыми будет работать датчик.
"           if(type.localeCompare(\"categories_list\") == 0){"
              // Очистка select
"              select_categories.innerHTML = '';"
              // Заполнение select опциями на основе значений JSON-объекта
"              for (let key in obj) {"
                  // Игнорируем 'type'
"                if (key !== 'type') {"
"                  let option = document.createElement('option');"
"                  option.value = obj[key];"
"                  option.text = obj[key];"
"                  select_categories.appendChild(option);"
"                }"
"              }"
              // Добавление select в контейнер.
"              document.getElementById('container_categories').appendChild(select_categories);"
"            }"
            // Записываем в select наименование всех цветов с которыми будет работать датчик.
"           else if(type.localeCompare(\"files_list\") == 0){"
              // Очистка select
"              select_files.innerHTML = '';"
              // Заполнение select опциями на основе значений JSON-объекта
"              for (let key in obj) {"
                  // Игнорируем 'type'
"                if (key !== 'type') {"
"                  let option = document.createElement('option');"
"                  option.value = obj[key];"
"                  option.text = obj[key];"
"                  select_files.appendChild(option);"
"                }"
"              }"
              // Добавление select в контейнер.
"              document.getElementById('container_files').appendChild(select_files);"
"            }"
"           else if(type.localeCompare(\"table_string\") == 0){"
"                 var table = obj.value;"
"                 createTableFromString(table, 'table-container');}"
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




