package org.me.tulahack.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.me.tulahack.model.*;
import org.me.tulahack.repository.RouteRepository;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

@Service
public class RouteService {

    private final GeocodingService geocodingService;
    private final PoiService poiService;
    private final OptimizerClient optimizerClient;
    private final MatrixService matrixService;
    private final DirectionsService directionsService;
    private final RouteRepository routeRepository;
    private final ObjectMapper objectMapper;

    public RouteService(GeocodingService geocodingService,
                        PoiService poiService,
                        OptimizerClient optimizerClient,
                        MatrixService matrixService,
                        DirectionsService directionsService,
                        RouteRepository routeRepository,
                        ObjectMapper objectMapper) {
        this.geocodingService = geocodingService;
        this.poiService = poiService;
        this.optimizerClient = optimizerClient;
        this.matrixService = matrixService;
        this.directionsService = directionsService;
        this.routeRepository = routeRepository;
        this.objectMapper = objectMapper;
    }

    public RouteResponse buildRoute(RouteRequest request) throws JsonProcessingException {
        Coordinate start = geocodingService.geocode(request.getStartAddress());
        Coordinate end = geocodingService.geocode(request.getEndAddress());

        List<PointOfInterest> pois = poiService.fetchPois(
                start, end, request.getCategories(), request.getMaxPois());

        List<Coordinate> naiveCoords = pois.stream()
                .map(p -> new Coordinate(p.getLat(), p.getLon()))
                .collect(Collectors.toList());
        double originalDistance = calcTotalDistance(start, naiveCoords, end);

        List<PointOfInterest> orderedPois = pois;
        double optimizedDistance = originalDistance;
        long computationMs = 0;

        if (!pois.isEmpty()) {
            List<Coordinate> allPoints = new ArrayList<>();
            allPoints.add(start);
            allPoints.addAll(naiveCoords);
            allPoints.add(end);

            TravelMatrix matrix = matrixService.fetchMatrix(allPoints, request.getTransportMode());

            List<OptimizerPoint> optimizerPoints = pois.stream()
                    .map(p -> OptimizerPoint.builder()
                            .id(p.getId())
                            .name(p.getName())
                            .lat(p.getLat())
                            .lon(p.getLon())
                            .category(p.getCategory())
                            .rubric(p.getRubric())
                            .workingHours(p.getWorkingHours())
                            .rating(p.getRating())
                            .reviews(p.getReviews())
                            .build())
                    .collect(Collectors.toList());

            int startIndex = 0;
            int endIndex = optimizerPoints.size() + 1;
            int pointsOffset = 1;

            OptimizerRequest optimizerRequest = new OptimizerRequest(
                    optimizerPoints, startIndex, endIndex, pointsOffset, matrix);
            OptimizerResponse optimizerResponse = optimizerClient.optimize(optimizerRequest);

            if (optimizerResponse.getOptimizedOrder() != null) {
                orderedPois = optimizerResponse.getOptimizedOrder().stream()
                        .filter(i -> i < pois.size())
                        .map(pois::get)
                        .collect(Collectors.toList());

                List<Coordinate> optimizedCoords = orderedPois.stream()
                        .map(p -> new Coordinate(p.getLat(), p.getLon()))
                        .collect(Collectors.toList());
                optimizedDistance = calcTotalDistance(start, optimizedCoords, end);
                computationMs = optimizerResponse.getComputationMs() != null
                        ? optimizerResponse.getComputationMs() : 0;
            }
        }

        List<Coordinate> fullRoute = new ArrayList<>();
        fullRoute.add(start);
        orderedPois.forEach(p -> fullRoute.add(new Coordinate(p.getLat(), p.getLon())));
        fullRoute.add(end);

        Map<String, Object> geojson = directionsService.fetchRouteGeojson(fullRoute, request.getTransportMode());

        double savingPercent = originalDistance > 0
                ? (originalDistance - optimizedDistance) / originalDistance * 100
                : 0;

        Route route = Route.builder()
                .startAddress(request.getStartAddress())
                .endAddress(request.getEndAddress())
                .categories(String.join(",", request.getCategories()))
                .originalDistance(originalDistance)
                .optimizedDistance(optimizedDistance)
                .savingPercent(savingPercent)
                .createdAt(LocalDateTime.now().toString())
                .poisJson(objectMapper.writeValueAsString(orderedPois))
                .build();

        Route saved = routeRepository.save(route);

        return RouteResponse.builder()
                .routeId(saved.getId())
                .originalDistance(originalDistance)
                .optimizedDistance(optimizedDistance)
                .savingPercent(Math.round(savingPercent * 10.0) / 10.0)
                .pois(orderedPois)
                .orderedCoordinates(fullRoute)
                .geojson(geojson)
                .computationMs(computationMs)
                .build();
    }

    public List<Route> getHistory() {
        return routeRepository.findAllByOrderByCreatedAtDesc();
    }

    private double calcTotalDistance(Coordinate start, List<Coordinate> middle, Coordinate end) {
        double total = 0;
        Coordinate prev = start;
        for (Coordinate curr : middle) {
            total += haversine(prev, curr);
            prev = curr;
        }
        total += haversine(prev, end);
        return total;
    }

    private double haversine(Coordinate a, Coordinate b) {
        final double R = 6_371_000;
        double dLat = Math.toRadians(b.lat() - a.lat());
        double dLon = Math.toRadians(b.lon() - a.lon());
        double sinLat = Math.sin(dLat / 2);
        double sinLon = Math.sin(dLon / 2);
        double h = sinLat * sinLat
                + Math.cos(Math.toRadians(a.lat()))
                * Math.cos(Math.toRadians(b.lat()))
                * sinLon * sinLon;
        return 2 * R * Math.asin(Math.sqrt(h));
    }
}
