package org.me.tulahack.service;

import org.me.tulahack.model.Coordinate;
import org.me.tulahack.model.TravelMatrix;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Service;
import org.springframework.web.reactive.function.client.WebClient;

import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

@Service
public class MatrixService {

    private final WebClient webClient;
    private final String apiKey;
    private final Duration timeout;

    public MatrixService(WebClient.Builder builder,
                         @Value("${ors.api.key}") String apiKey,
                         @Value("${webclient.timeout.seconds}") int timeoutSeconds) {
        this.webClient = builder
                .baseUrl("https://api.openrouteservice.org")
                .build();
        this.apiKey = apiKey;
        this.timeout = Duration.ofSeconds(timeoutSeconds);
    }

    public TravelMatrix fetchMatrix(List<Coordinate> points, String profile) {
        if (apiKey.equals("YOUR_KEY_HERE") || points.size() < 2) {
            return null;
        }

        try {
            List<List<Double>> locations = points.stream()
                    .map(c -> List.of(c.lon(), c.lat()))
                    .collect(Collectors.toList());

            Map<String, Object> requestBody = Map.of(
                    "locations", locations,
                    "metrics", List.of("distance", "duration")
            );

            Map response = webClient.post()
                    .uri("/v2/matrix/" + profile)
                    .header("Authorization", apiKey)
                    .contentType(MediaType.APPLICATION_JSON)
                    .bodyValue(requestBody)
                    .retrieve()
                    .bodyToMono(Map.class)
                    .timeout(timeout)
                    .block();

            if (response == null) return null;

            List<List<Double>> distances = (List<List<Double>>) response.get("distances");
            List<List<Double>> durations = (List<List<Double>>) response.get("durations");

            return new TravelMatrix(distances, durations);

        } catch (Exception e) {
            return null;
        }
    }
}
