package org.me.tulahack.service;

import org.me.tulahack.model.Coordinate;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Service;
import org.springframework.web.reactive.function.client.WebClient;
import org.springframework.web.reactive.function.client.WebClientResponseException;

import java.time.Duration;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

@Service
public class DirectionsService {

    private final WebClient webClient;
    private final String apiKey;
    private final Duration timeout;

    public DirectionsService(WebClient.Builder builder,
                             @Value("${ors.api.key}") String apiKey,
                             @Value("${webclient.timeout.seconds}") int timeoutSeconds) {
        this.webClient = builder
                .baseUrl("https://api.openrouteservice.org")
                .build();
        this.apiKey = apiKey;
        this.timeout = Duration.ofSeconds(timeoutSeconds);
    }

    public Map<String, Object> fetchRouteGeojson(List<Coordinate> points, String profile) {
        if (points.size() < 2) {
            return null;
        }

        try {
            List<List<Double>> coordinates = points.stream()
                    .map(c -> List.of(c.lon(), c.lat()))
                    .collect(Collectors.toList());

            List<Double> radiuses = Collections.nCopies(coordinates.size(), -1.0);
            Map<String, Object> requestBody = Map.of("coordinates", coordinates, "radiuses", radiuses);

            return webClient.post()
                    .uri("/v2/directions/" + profile + "/geojson")
                    .header("Authorization", apiKey)
                    .contentType(MediaType.APPLICATION_JSON)
                    .bodyValue(requestBody)
                    .retrieve()
                    .bodyToMono(Map.class)
                    .map(m -> (Map<String, Object>) m)
                    .timeout(timeout)
                    .block();

        } catch (WebClientResponseException e) {
            System.err.println("DirectionsService HTTP " + e.getStatusCode() + " for profile=" + profile
                    + ": " + e.getResponseBodyAsString());
            return null;
        } catch (Exception e) {
            System.err.println("DirectionsService error for profile=" + profile + ": " + e.getMessage());
            return null;
        }
    }
}
