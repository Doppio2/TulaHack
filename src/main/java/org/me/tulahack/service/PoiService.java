package org.me.tulahack.service;

import org.me.tulahack.model.Coordinate;
import org.me.tulahack.model.PointOfInterest;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.web.reactive.function.client.WebClient;

import java.time.Duration;
import java.util.*;
import java.util.stream.Collectors;

@Service
public class PoiService {

    private final WebClient webClient;
    private final String apiKey;
    private final Duration timeout;

    private static final Map<String, String> CATEGORY_RUBRICS = Map.of(
            "restaurant", "Ресторан",
            "cafe",       "Кафе",
            "museum",     "Музей",
            "park",       "Парк",
            "shop",       "Магазин",
            "hotel",      "Отель",
            "pharmacy",   "Аптека",
            "atm",        "Банкомат"
    );

    public PoiService(WebClient.Builder builder,
                      @Value("${twogis.api.url}") String baseUrl,
                      @Value("${twogis.api.key}") String apiKey,
                      @Value("${webclient.timeout.seconds}") int timeoutSeconds) {
        this.webClient = builder.baseUrl(baseUrl).build();
        this.apiKey = apiKey;
        this.timeout = Duration.ofSeconds(timeoutSeconds);
    }

    public List<PointOfInterest> fetchPois(Coordinate start, Coordinate end,
                                           List<String> categories, int maxTotal) {
        double centerLat = (start.lat() + end.lat()) / 2;
        double centerLon = (start.lon() + end.lon()) / 2;

        double latDiff = Math.abs(start.lat() - end.lat());
        double lonDiff = Math.abs(start.lon() - end.lon());
        int radiusMeters = (int) (Math.max(latDiff, lonDiff) * 111000 / 2) + 1000;
        radiusMeters = Math.min(radiusMeters, 10000);

        int perCategory = Math.max(1, maxTotal / categories.size());

        List<PointOfInterest> all = new ArrayList<>();

        for (String category : categories) {
            List<PointOfInterest> pois = fetchByCategory(
                    category, centerLat, centerLon, radiusMeters, perCategory);
            all.addAll(pois);
        }

        return all.stream().limit(maxTotal).collect(Collectors.toList());
    }

    private List<PointOfInterest> fetchByCategory(String category,
                                                   double lat, double lon,
                                                   int radius, int limit) {
        try {
            Map response = webClient.get()
                    .uri(uriBuilder -> uriBuilder
                            .path("/3.0/items")
                            .queryParam("q", CATEGORY_RUBRICS.getOrDefault(category, category))
                            .queryParam("point", lon + "," + lat)
                            .queryParam("radius", radius)
                            .queryParam("page_size", limit)
                            .queryParam("fields", "items.point,items.address,items.rating," +
                                    "items.reviews_count,items.rubrics,items.schedule," +
                                    "items.contact_groups")
                            .queryParam("key", apiKey)
                            .build())
                    .retrieve()
                    .bodyToMono(Map.class)
                    .timeout(timeout)
                    .block();

            if (response == null) return List.of();

            Map result = (Map) response.get("result");
            if (result == null) return List.of();

            List<Map> items = (List<Map>) result.get("items");
            if (items == null) return List.of();

            return items.stream()
                    .map(item -> parseItem(item, category))
                    .filter(Objects::nonNull)
                    .collect(Collectors.toList());

        } catch (Exception e) {
            return List.of();
        }
    }

    private PointOfInterest parseItem(Map item, String category) {
        try {
            Map point = (Map) item.get("point");
            if (point == null) return null;

            double lon = ((Number) point.get("lon")).doubleValue();
            double lat = ((Number) point.get("lat")).doubleValue();

            String name = (String) item.getOrDefault("name", "Без названия");
            String id   = String.valueOf(item.getOrDefault("id", ""));

            // address_name лежит прямо в item, не внутри вложенного address
            String address = (String) item.getOrDefault("address_name", "");

            Double rating = null;
            Object ratingObj = item.get("rating");
            if (ratingObj instanceof Number) {
                rating = ((Number) ratingObj).doubleValue();
            }

            Integer reviews = null;
            Object reviewsObj = item.get("reviews_count");
            if (reviewsObj instanceof Number) {
                reviews = ((Number) reviewsObj).intValue();
            }

            // Парсим расписание в читаемую строку: "Пн-Пт 09:00–22:00, Сб-Вс 10:00–20:00"
            String workingHours = null;
            Map schedule = (Map) item.get("schedule");
            if (schedule != null) {
                workingHours = parseSchedule(schedule);
            }

            String contacts = null;
            List<Map> contactGroups = (List<Map>) item.get("contact_groups");
            if (contactGroups != null && !contactGroups.isEmpty()) {
                List<Map> contactItems = (List<Map>) contactGroups.get(0).get("contacts");
                if (contactItems != null && !contactItems.isEmpty()) {
                    contacts = (String) contactItems.get(0).getOrDefault("value", null);
                }
            }

            // Рубрику берём из массива rubrics, тип primary — это основная категория заведения
            String rubric = CATEGORY_RUBRICS.getOrDefault(category, category);
            List<Map> rubrics = (List<Map>) item.get("rubrics");
            if (rubrics != null) {
                rubric = rubrics.stream()
                        .filter(r -> "primary".equals(r.get("kind")))
                        .map(r -> (String) r.get("name"))
                        .findFirst()
                        .orElse(rubric);
            }

            return PointOfInterest.builder()
                    .id(id)
                    .name(name)
                    .address(address)
                    .lat(lat)
                    .lon(lon)
                    .category(category)
                    .rubric(rubric)
                    .workingHours(workingHours)
                    .rating(rating)
                    .reviews(reviews)
                    .contacts(contacts)
                    .build();

        } catch (Exception e) {
            return null;
        }
    }

    private String parseSchedule(Map schedule) {
        Map<String, String> dayNames = Map.of(
                "Mon", "Пн", "Tue", "Вт", "Wed", "Ср",
                "Thu", "Чт", "Fri", "Пт", "Sat", "Сб", "Sun", "Вс"
        );
        List<String> order = List.of("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun");

        // Группируем дни с одинаковым временем работы
        Map<String, List<String>> timeTodays = new LinkedHashMap<>();
        for (String day : order) {
            Map dayData = (Map) schedule.get(day);
            if (dayData == null) continue;
            List<Map> hours = (List<Map>) dayData.get("working_hours");
            if (hours == null || hours.isEmpty()) continue;
            String time = hours.get(0).get("from") + "–" + hours.get(0).get("to");
            timeTodays.computeIfAbsent(time, k -> new ArrayList<>()).add(day);
        }

        List<String> parts = new ArrayList<>();
        for (Map.Entry<String, List<String>> entry : timeTodays.entrySet()) {
            List<String> days = entry.getValue();
            String daysStr;
            if (days.size() == 1) {
                daysStr = dayNames.get(days.get(0));
            } else {
                daysStr = dayNames.get(days.get(0)) + "–" + dayNames.get(days.get(days.size() - 1));
            }
            parts.add(daysStr + " " + entry.getKey());
        }
        return String.join(", ", parts);
    }

    public static List<String> getAvailableCategories() {
        return new ArrayList<>(CATEGORY_RUBRICS.keySet());
    }
}
