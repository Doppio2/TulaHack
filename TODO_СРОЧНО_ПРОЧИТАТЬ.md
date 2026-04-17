# TODO: Срочно прочитать для Java/C++ интеграции

## Проблема

Java-код уже пытается вызывать C++ optimizer через `OptimizerClient`, но интеграция сейчас не заработает стабильно.

Основные причины:

- в `application.properties` нет нужных путей для C++ optimizer;
- Java пишет input JSON в файл, но C++ пока этот файл не читает;
- Java ждет output JSON из файла, но C++ пока этот файл не пишет;
- формат `points` и `travel_matrix` сейчас индексируется по-разному;
- `cpp.optimizer.url` больше не соответствует текущему `OptimizerClient`, потому что клиент запускает процесс, а не делает HTTP-запрос.

## Что сейчас делает Java

Файл:

```text
src/main/java/org/me/tulahack/service/OptimizerClient.java
```

Код:

```java
Path inputFile  = Path.of(inputFilePath);
Path outputFile = Path.of(outputFilePath);

objectMapper.writeValue(inputFile.toFile(), request);

Process process = new ProcessBuilder(optimizerPath)
        .inheritIO()
        .start();

process.waitFor();

return objectMapper.readValue(outputFile.toFile(), OptimizerResponse.class);
```

То есть Java должна:

1. создать input JSON файл;
2. запустить C++ executable;
3. дождаться завершения;
4. прочитать output JSON файл.

## Что сейчас не так с properties

В файле:

```text
src/main/resources/application.properties
```

сейчас есть:

```properties
cpp.optimizer.url=http://localhost:8081
```

Но текущий `OptimizerClient` это поле не использует.

Вместо этого он ожидает:

```java
@Value("${cpp.optimizer.path}") String optimizerPath
@Value("${cpp.optimizer.input}") String inputFilePath
@Value("${cpp.optimizer.output}") String outputFilePath
```

Нужно добавить:

```properties
cpp.optimizer.path=./build/route_solver
cpp.optimizer.input=./build/optimizer_input.json
cpp.optimizer.output=./build/optimizer_output.json
```

`cpp.optimizer.url` можно удалить позже, если HTTP-вариант больше не нужен.

## Что нужно поменять в запуске C++

Сейчас Java запускает:

```java
new ProcessBuilder(optimizerPath)
```

Лучше передавать пути явно:

```java
new ProcessBuilder(optimizerPath, inputFilePath, outputFilePath)
```

Тогда C++ программа должна запускаться так:

```text
./build/route_solver ./build/optimizer_input.json ./build/optimizer_output.json
```

И внутри C++:

```text
argv[1] = input json file
argv[2] = output json file
```

Это лучше, чем хардкодить пути в C++.

## Input JSON, который Java должна писать

Текущий Java-класс:

```text
src/main/java/org/me/tulahack/model/OptimizerRequest.java
```

формирует JSON:

```json
{
  "points": [
    {
      "id": "123456",
      "name": "Ресторан Пушкин",
      "lat": 55.7558,
      "lon": 37.6176,
      "category": "restaurant",
      "rubric": "Ресторан",
      "working_hours": "Пн-Вс 10:00-22:00",
      "rating": 4.7,
      "reviews": 1234
    }
  ],
  "travel_matrix": {
    "distances": [[0, 1234], [1234, 0]],
    "durations": [[0, 120], [120, 0]]
  }
}
```

Это в целом нормальный формат для C++.

Но есть важная проблема с индексами.

## Главная проблема: points и travel_matrix не совпадают

В `RouteService.java` матрица строится так:

```java
List<Coordinate> allPoints = new ArrayList<>();
allPoints.add(start);
allPoints.addAll(naiveCoords);
allPoints.add(end);

TravelMatrix matrix = matrixService.fetchMatrix(allPoints, request.getTransportMode());
```

То есть `travel_matrix` имеет индексы:

```text
0 = start
1 = poi[0]
2 = poi[1]
3 = poi[2]
...
N = end
```

Но `points` формируется так:

```java
List<OptimizerPoint> optimizerPoints = pois.stream()
```

То есть `points` содержит только POI:

```text
0 = poi[0]
1 = poi[1]
2 = poi[2]
...
```

Получается рассинхрон:

```text
travel_matrix[1] соответствует points[0]
travel_matrix[2] соответствует points[1]
```

А `travel_matrix[0]` вообще соответствует start, которого нет в `points`.

## Быстрый вариант решения

Оставить `points` как список только POI.

Тогда нужно явно договориться:

```text
matrix index 0 = start
matrix index i + 1 = points[i]
last matrix index = end
```

C++ тогда должен делать:

```cpp
Point->MatrixIndex = PointIndex + 1;
StartMatrixIndex = 0;
EndMatrixIndex = TravelMatrix.PointCount - 1;
```

А в output возвращать индексы именно для Java `points`, не matrix index:

```json
{
  "optimized_order": [1, 0, 2]
}
```

Где:

```text
1 = points[1]
0 = points[0]
2 = points[2]
```

## Более чистый вариант решения

Поменять input JSON и добавить route metadata:

```json
{
  "start_index": 0,
  "end_index": 4,
  "points_offset": 1,
  "points": [
    { "id": "poi-1", "name": "..." },
    { "id": "poi-2", "name": "..." }
  ],
  "travel_matrix": {
    "distances": [[...]],
    "durations": [[...]]
  }
}
```

Тогда C++ не будет гадать, как связаны `points` и `travel_matrix`.

Для хакатона можно использовать быстрый вариант.

## Output JSON, который должен писать C++

Java ожидает класс:

```text
src/main/java/org/me/tulahack/model/OptimizerResponse.java
```

Формат:

```json
{
  "optimized_order": [1, 0, 2],
  "total_distance": 1234.0,
  "computation_ms": 15
}
```

Обязательное поле:

```json
"optimized_order"
```

Это список индексов в Java-массиве `points`, а не `travel_matrix`.

## Что сделать Java-человеку

1. Добавить properties:

```properties
cpp.optimizer.path=./build/route_solver
cpp.optimizer.input=./build/optimizer_input.json
cpp.optimizer.output=./build/optimizer_output.json
```

2. Удалить или игнорировать старое поле:

```properties
cpp.optimizer.url=http://localhost:8081
```

3. Поменять запуск C++:

```java
Process process = new ProcessBuilder(
        optimizerPath,
        inputFilePath,
        outputFilePath
)
        .inheritIO()
        .start();
```

4. Убедиться, что папка `build` существует перед записью input/output:

```java
Files.createDirectories(inputFile.getParent());
Files.createDirectories(outputFile.getParent());
```

5. Исправить или явно зафиксировать индексацию:

```text
travel_matrix[0] = start
travel_matrix[i + 1] = points[i]
travel_matrix[last] = end
```

6. C++ должен вернуть:

```json
{
  "optimized_order": [индексы points],
  "total_distance": число,
  "computation_ms": число
}
```

7. На время отладки после `objectMapper.writeValue(...)` вывести путь:

```java
System.out.println("Optimizer input: " + inputFile.toAbsolutePath());
System.out.println("Optimizer output: " + outputFile.toAbsolutePath());
```

8. Если C++ упал или output-файл не создан, не молча fallback, а временно вывести exception:

```java
e.printStackTrace();
```

Сейчас `catch` скрывает ошибку и сразу возвращает fallback, поэтому кажется, что "ничего не происходит".

## Почему кажется, что файл не создается

Потому что:

- нужные пути не прописаны в `application.properties`;
- `OptimizerClient` может не стартовать из-за missing properties;
- если `optimize()` падает, exception скрывается;
- output-файл точно не появится, пока C++ не научится его писать;
- input-файл появится только если реально вызывается `/api/route/build` и выполнение дошло до `objectMapper.writeValue(...)`.

## Что C++ человек сделает после этого

После фикса Java-стороны C++ нужно переделать:

```text
main(int ArgCount, char **Args)
```

Ожидать:

```text
Args[1] = input JSON path
Args[2] = output JSON path
```

Потом:

```text
read input file
parse JSON
solve route
write OptimizerResponse JSON
```

