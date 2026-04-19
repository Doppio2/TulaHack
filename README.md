# Route Optimization Service

Route Optimization Service - MVP-сервис для планирования маршрута по точкам интереса. Пользователь задает начальный адрес, конечный адрес, интересующие категории мест, приоритеты категорий, тип транспорта и временное окно. Сервис подбирает кандидатные места, строит матрицу перемещений, вызывает C++ оптимизатор и возвращает упорядоченный маршрут с расписанием посещения и метриками.

Задача близка к Orienteering Problem with Time Windows: у каждой точки есть ценность, время посещения, окно работы и стоимость перемещения до других точек. Оптимизатор выбирает допустимый порядок посещения, который укладывается во время пользователя.

## Что Делает Проект

- Геокодирует начальный и конечный адрес.
- Получает POI из 2GIS по выбранным категориям.
- Позволяет пользователю задавать ценность выбранных категорий через приоритеты.
- Строит матрицу расстояний и времени через OpenRouteService.
- Вызывает C++ оптимизатор через JSON-файлы.
- Возвращает оптимальный порядок POI, расписание, метрики расстояния, геометрию маршрута и историю построенных маршрутов.

## Архитектура

```text
Frontend
  -> Spring Boot REST API
     -> внешние картографические сервисы
        -> 2GIS POI search
        -> OpenRouteService matrix/directions
     -> C++ route optimizer executable
     -> SQLite route history
```

### Frontend

Статический frontend лежит в:

```text
src/main/resources/static/index.html
```

Он работает со Spring Boot API и отображает маршрут, выбранные точки, расписание и геометрию на карте.

### Java Backend

Backend написан на Spring Boot.

Основные задачи Java-части:

- REST API.
- Геокодирование адресов.
- Получение POI из 2GIS.
- Получение матрицы расстояний/времени и геометрии маршрута из OpenRouteService.
- Подготовка JSON для C++ оптимизатора.
- Запуск C++ executable.
- Чтение JSON-ответа от C++.
- Сохранение построенных маршрутов в SQLite.

Ключевые файлы:

```text
src/main/java/org/me/tulahack/controller/RouteController.java
src/main/java/org/me/tulahack/service/RouteService.java
src/main/java/org/me/tulahack/service/PoiService.java
src/main/java/org/me/tulahack/service/MatrixService.java
src/main/java/org/me/tulahack/service/DirectionsService.java
src/main/java/org/me/tulahack/service/OptimizerClient.java
```

### C++ Оптимизатор

C++ оптимизатор лежит в:

```text
src/route_solver/
```

После сборки бинарь находится здесь:

```text
build/route_solver
```

C++ часть:

- читает входной JSON из `argv[1]`;
- парсит его локальным `simple_json` parser;
- строит внутренние структуры на массивах и матрице перемещений;
- запускает greedy-эвристику;
- пишет выходной JSON в `argv[2]`.

Ключевые файлы:

```text
src/route_solver/main.cpp
src/route_solver/route_optimizer.h
src/route_solver/route_optimizer.cpp
src/route_solver/simple_json.h
src/route_solver/simple_json.c
src/route_solver/arena_allocator.h
src/route_solver/arena_allocator.c
```

C++ оптимизатор написан в data-oriented стиле: плоские массивы, матричный доступ, arena allocation и минимум лишних абстракций.

## Модель Оптимизации

C++ solver получает:

- `points` - кандидатные POI;
- `travel_matrix.distances` - матрицу расстояний в метрах;
- `travel_matrix.durations` - матрицу времени перемещения в секундах;
- `start_index` - индекс стартовой точки в матрице;
- `end_index` - индекс конечной точки в матрице;
- `points_offset` - смещение от `points[i]` к индексу в матрице;
- `category_priorities` - пользовательские приоритеты категорий от 1 до 10;
- `start_time` / `end_time` - временное окно пользователя в минутах от начала дня.

Текущая эвристика выбирает следующую точку по отношению:

```text
score / added_time
```

Где:

```text
added_time = travel_time + waiting_time + visit_duration
```

`score` учитывает дефолтную ценность категории, рейтинг/отзывы места и пользовательский приоритет категории:

```text
score = default_score + category_priority * 3
```

Если пользовательские приоритеты не переданы, solver работает со стандартными дефолтными score.

Solver учитывает:

- время перемещения;
- ожидание открытия точки;
- часы работы места;
- примерное время посещения;
- конечное время маршрута;
- возможность успеть до конечной точки.

На выходе C++ возвращает:

- оптимальный порядок POI;
- итоговое расстояние;
- время работы оптимизатора;
- расписание посещения с arrival/start/end временем.

## JSON-Контракт Java И C++

Java пишет входной файл для оптимизатора по пути:

```properties
cpp.optimizer.input=./build/optimizer_input.json
```

C++ пишет ответ по пути:

```properties
cpp.optimizer.output=./build/optimizer_output.json
```

Путь до C++ executable:

```properties
cpp.optimizer.path=./build/route_solver
```

### Входной JSON Для Оптимизатора

```json
{
  "points": [
    {
      "id": "123456",
      "name": "Музей",
      "lat": 54.2045,
      "lon": 37.6188,
      "category": "museum",
      "rubric": "Музей",
      "working_hours": "Пн-Вс 10:00-20:00",
      "rating": 4.8,
      "reviews": 2500
    }
  ],
  "category_priorities": {
    "restaurant": 8,
    "museum": 10,
    "park": 4
  },
  "start_index": 0,
  "end_index": 2,
  "points_offset": 1,
  "travel_matrix": {
    "distances": [[0, 1200, 3000], [1200, 0, 1500], [3000, 1500, 0]],
    "durations": [[0, 120, 300], [120, 0, 180], [300, 180, 0]]
  },
  "start_time": 540,
  "end_time": 1080
}
```

Текущая раскладка матрицы:

```text
matrix[0] = start
matrix[i + points_offset] = points[i]
matrix[end_index] = end
```

### Выходной JSON От Оптимизатора

```json
{
  "optimized_order": [0],
  "total_distance": 4200,
  "computation_ms": 1,
  "schedule": [
    {
      "point_index": 0,
      "arrival_time": 560,
      "visit_start_time": 600,
      "visit_end_time": 690,
      "travel_from_previous": 20,
      "waiting_time": 40
    }
  ]
}
```

Все значения времени в `schedule` указаны в минутах от начала дня.

## REST API

Основной endpoint:

```http
POST /api/route/build
```

Пример запроса:

```json
{
  "startAddress": "Тула, площадь Ленина",
  "endAddress": "Тула, Московский вокзал",
  "categories": ["museum", "park"],
  "categoryPriorities": {
    "museum": 10,
    "park": 4
  },
  "maxPois": 5,
  "transportMode": "driving-car",
  "departureTime": "09:00",
  "endTime": "18:00"
}
```

Ответ содержит:

- id сохраненного маршрута;
- исходную и оптимизированную дистанцию;
- процент экономии;
- упорядоченный список POI;
- координаты маршрута;
- расписание;
- GeoJSON оптимизированного маршрута;
- GeoJSON наивного маршрута;
- время работы оптимизатора.

Другие endpoints:

```http
GET /api/route/history
GET /api/poi/categories
POST /api/geocode
```

## Конфигурация

Конфигурация лежит в:

```text
src/main/resources/application.properties
```

Основные параметры:

```properties
server.port=8080

spring.datasource.url=jdbc:sqlite:routes.db

cpp.optimizer.path=./build/route_solver
cpp.optimizer.input=./build/optimizer_input.json
cpp.optimizer.output=./build/optimizer_output.json

ors.api.key=...
twogis.api.key=...
twogis.api.url=https://catalog.api.2gis.com
```

## Зависимости

Для запуска проекта нужны:

- Java 21;
- Maven wrapper из репозитория (`./mvnw`);
- C++ compiler с поддержкой C++ и POSIX API, например `clang++`;
- SQLite JDBC подключается через Maven-зависимости;
- доступ к 2GIS API;
- доступ к OpenRouteService API.

Нативный C++ оптимизатор не требует внешних пакетных зависимостей. Внутри `src/route_solver` уже лежат:

- локальный JSON parser;
- arena allocator;
- `stb_ds.h` для вспомогательных структур parser'а.

## Сборка И Запуск

Собрать C++ оптимизатор:

```bash
cd src/route_solver
./build.sh
```

После сборки появится:

```text
build/route_solver
```

Собрать Java backend:

```bash
./mvnw -DskipTests package
```

Запустить приложение:

```bash
./mvnw spring-boot:run
```

Открыть frontend:

```text
http://localhost:8080
```

## Отдельный Запуск C++ Оптимизатора

C++ optimizer можно запускать отдельно от Java:

```bash
./build/route_solver ./build/optimizer_input.json ./build/optimizer_output.json
```

Где:

```text
argv[1] = путь к input JSON
argv[2] = путь к output JSON
```

Пример минимального input-файла:

```json
{
  "points": [
    {
      "id": "museum-1",
      "name": "Музей",
      "lat": 54.2045,
      "lon": 37.6188,
      "category": "museum",
      "rubric": "Музей",
      "working_hours": "Пн-Вс 10:00-20:00",
      "rating": 4.8,
      "reviews": 2500
    }
  ],
  "start_index": 0,
  "end_index": 2,
  "points_offset": 1,
  "travel_matrix": {
    "distances": [[0, 1200, 3000], [1200, 0, 1500], [3000, 1500, 0]],
    "durations": [[0, 120, 300], [120, 0, 180], [300, 180, 0]]
  },
  "start_time": 540,
  "end_time": 1080
}
```

После запуска C++ создаст output-файл в формате `OptimizerResponse`.

## Полный Локальный Запуск

Обычный порядок запуска всего проекта:

```bash
cd src/route_solver
./build.sh
cd ../..
./mvnw spring-boot:run
```

После старта backend доступен на:

```text
http://localhost:8080
```

При вызове `POST /api/route/build` Java:

1. получает точки и матрицу от внешних сервисов;
2. добавляет пользовательские приоритеты категорий в optimizer request;
3. пишет `build/optimizer_input.json`;
4. запускает `build/route_solver`;
5. читает `build/optimizer_output.json`;
6. возвращает маршрут frontend'у.

## Хранение Данных

История маршрутов хранится в SQLite:

```text
routes.db
```

В базе сохраняются метаданные маршрута и сериализованные POI для уже построенных маршрутов.
