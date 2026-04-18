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
import java.util.stream.IntStream;

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
        List<Integer> orderedIndices = IntStream.range(0, pois.size()).boxed().collect(Collectors.toList());
        List<ScheduleEntry> schedule = new ArrayList<>();
        double optimizedDistance = originalDistance;
        long computationMs = 0;
        TravelMatrix matrix = null;

        if (!pois.isEmpty()) {
            List<Coordinate> allPoints = new ArrayList<>();
            allPoints.add(start);
            allPoints.addAll(naiveCoords);
            allPoints.add(end);

            matrix = matrixService.fetchMatrix(allPoints, request.getTransportMode());

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

            int startTimeMin = parseTimeToMinutes(request.getDepartureTime());
            int endTimeMin = parseTimeToMinutes(request.getEndTime());

            OptimizerRequest optimizerRequest = new OptimizerRequest(
                    optimizerPoints, startIndex, endIndex, pointsOffset, matrix,
                    startTimeMin, endTimeMin);
            OptimizerResponse optimizerResponse = optimizerClient.optimize(optimizerRequest);

            if (optimizerResponse.getOptimizedOrder() != null) {
                orderedIndices = optimizerResponse.getOptimizedOrder().stream()
                        .filter(i -> i < pois.size())
                        .collect(Collectors.toList());
                orderedPois = orderedIndices.stream()
                        .map(pois::get)
                        .collect(Collectors.toList());

                List<Coordinate> optimizedCoords = orderedPois.stream()
                        .map(p -> new Coordinate(p.getLat(), p.getLon()))
                        .collect(Collectors.toList());
                optimizedDistance = calcTotalDistance(start, optimizedCoords, end);
                computationMs = optimizerResponse.getComputationMs() != null
                        ? optimizerResponse.getComputationMs() : 0;

                if (optimizerResponse.getSchedule() != null) {
                    schedule = optimizerResponse.getSchedule();
                }

                if (optimizedDistance >= originalDistance) {
                    orderedPois = pois;
                    orderedIndices = IntStream.range(0, pois.size()).boxed().collect(Collectors.toList());
                    optimizedDistance = originalDistance;
                    schedule = new ArrayList<>();
                }
            }
        }

        if (schedule.isEmpty()) {
            int endTimeMin = parseTimeToMinutes(request.getEndTime());
            schedule = buildFallbackSchedule(request.getDepartureTime(), endTimeMin, orderedIndices, orderedPois, matrix);
        }

        List<Coordinate> naiveRoute = new ArrayList<>();
        naiveRoute.add(start);
        pois.forEach(p -> naiveRoute.add(new Coordinate(p.getLat(), p.getLon())));
        naiveRoute.add(end);

        List<Coordinate> fullRoute = new ArrayList<>();
        fullRoute.add(start);
        orderedPois.forEach(p -> fullRoute.add(new Coordinate(p.getLat(), p.getLon())));
        fullRoute.add(end);

        Map<String, Object> naiveGeojson = directionsService.fetchRouteGeojson(naiveRoute, request.getTransportMode());
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
                .schedule(schedule)
                .geojson(geojson)
                .naiveGeojson(naiveGeojson)
                .computationMs(computationMs)
                .build();
    }

    public List<Route> getHistory() {
        return routeRepository.findAllByOrderByCreatedAtDesc();
    }

    private int parseTimeToMinutes(String time) {
        try {
            String[] parts = time.split(":");
            return Integer.parseInt(parts[0]) * 60 + Integer.parseInt(parts[1]);
        } catch (Exception e) {
            return 540;
        }
    }

    private int getVisitDuration(String category) {
        if (category == null) return 10;
        return switch (category) {
            case "museum" -> 30;
            case "restaurant" -> 15;
            case "park" -> 15;
            case "cafe" -> 10;
            case "hotel" -> 10;
            case "shop" -> 10;
            case "pharmacy" -> 5;
            case "atm" -> 5;
            default -> 10;
        };
    }

    private List<ScheduleEntry> buildFallbackSchedule(String departureTime, int endTimeMin,
                                                       List<Integer> orderedIndices,
                                                       List<PointOfInterest> orderedPois,
                                                       TravelMatrix matrix) {
        if (matrix == null || matrix.getDurations() == null) {
            return List.of();
        }
        try {
            int currentTime = parseTimeToMinutes(departureTime);
            List<ScheduleEntry> result = new ArrayList<>();
            int prevIdx = 0;
            for (int i = 0; i < orderedIndices.size(); i++) {
                int poiIdx = orderedIndices.get(i);
                int currIdx = poiIdx + 1;
                double seconds = matrix.getDurations().get(prevIdx).get(currIdx);
                int travelMin = (int) Math.ceil(seconds / 60.0);
                int arrivalTime = currentTime + travelMin;
                String category = orderedPois.get(i).getCategory();
                int visitDuration = getVisitDuration(category);
                if (arrivalTime + visitDuration > endTimeMin) {
                    break;
                }
                ScheduleEntry entry = new ScheduleEntry();
                entry.setPointIndex(poiIdx);
                entry.setArrivalTime(arrivalTime);
                entry.setVisitStartTime(arrivalTime);
                entry.setVisitEndTime(arrivalTime + visitDuration);
                entry.setTravelFromPrevious(travelMin);
                entry.setWaitingTime(0);
                result.add(entry);
                currentTime = arrivalTime + visitDuration;
                prevIdx = currIdx;
            }
            return result;
        } catch (Exception e) {
            return List.of();
        }
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
