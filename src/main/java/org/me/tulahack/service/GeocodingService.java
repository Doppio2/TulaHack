package org.me.tulahack.service;

import org.me.tulahack.model.Coordinate;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.web.reactive.function.client.WebClient;

import java.time.Duration;
import java.util.List;
import java.util.Map;

@Service
public class GeocodingService {

    private final WebClient webClient;
    private final Duration timeout;

    public GeocodingService(WebClient.Builder builder,
                            @Value("${webclient.timeout.seconds}") int timeoutSeconds) {
        this.webClient = builder
                .baseUrl("https://nominatim.openstreetmap.org")
                .build();
        this.timeout = Duration.ofSeconds(timeoutSeconds);
    }

    public Coordinate geocode(String address) {
        List<Map<String, Object>> results = webClient.get()
                .uri(uriBuilder -> uriBuilder
                        .path("/search")
                        .queryParam("q", address)
                        .queryParam("format", "json")
                        .queryParam("limit", 1)
                        .queryParam("addressdetails", 1)
                        .build())
                .retrieve()
                .bodyToFlux(Map.class)
                .cast(Map.class)
                .map(m -> (Map<String, Object>) m)
                .collectList()
                .timeout(timeout)
                .block();

        if (results == null || results.isEmpty()) {
            throw new RuntimeException("Адрес не найден: " + address);
        }

        Map<String, Object> first = results.get(0);
        double lat = Double.parseDouble((String) first.get("lat"));
        double lon = Double.parseDouble((String) first.get("lon"));

        return new Coordinate(lat, lon);
    }
}
