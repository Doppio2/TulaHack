package org.me.tulahack.controller;

import jakarta.validation.Valid;
import org.me.tulahack.model.Route;
import org.me.tulahack.model.RouteRequest;
import org.me.tulahack.model.RouteResponse;
import org.me.tulahack.model.Coordinate;
import org.me.tulahack.service.GeocodingService;
import org.me.tulahack.service.PoiService;
import org.me.tulahack.service.RouteService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api")
public class RouteController {

    private final RouteService routeService;
    private final GeocodingService geocodingService;
    private final PoiService poiService;

    public RouteController(RouteService routeService, GeocodingService geocodingService, PoiService poiService) {
        this.routeService = routeService;
        this.geocodingService = geocodingService;
        this.poiService = poiService;
    }

    @PostMapping("/route/build")
    public ResponseEntity<?> buildRoute(@Valid @RequestBody RouteRequest request) {
        try {
            RouteResponse response = routeService.buildRoute(request);
            return ResponseEntity.ok(response);
        } catch (Exception e) {
            String msg = e.getMessage() != null ? e.getMessage() : e.getClass().getSimpleName();
            return ResponseEntity.badRequest().body(Map.of("error", msg));
        }
    }

    @GetMapping("/route/history")
    public ResponseEntity<List<Route>> getHistory() {
        return ResponseEntity.ok(routeService.getHistory());
    }

    @DeleteMapping("/route/history")
    public ResponseEntity<Void> clearHistory() {
        routeService.clearHistory();
        return ResponseEntity.ok().build();
    }

    @GetMapping("/poi/categories")
    public ResponseEntity<List<String>> getCategories() {
        return ResponseEntity.ok(PoiService.getAvailableCategories());
    }

    @GetMapping("/poi/search")
    public ResponseEntity<?> searchPoi(@RequestParam String q,
                                       @RequestParam(required = false) String near) {
        try {
            Coordinate center;
            if (near != null && !near.isBlank()) {
                String[] parts = near.split(",");
                center = new Coordinate(Double.parseDouble(parts[0]), Double.parseDouble(parts[1]));
            } else {
                center = new Coordinate(55.7558, 37.6176);
            }
            return ResponseEntity.ok(poiService.searchSuggestions(center, q, 15000));
        } catch (Exception e) {
            String msg = e.getMessage() != null ? e.getMessage() : e.getClass().getSimpleName();
            return ResponseEntity.badRequest().body(Map.of("error", msg));
        }
    }

    @PostMapping("/geocode")
    public ResponseEntity<?> geocode(@RequestBody Map<String, String> body) {
        try {
            String address = body.get("address");
            return ResponseEntity.ok(geocodingService.geocode(address));
        } catch (Exception e) {
            String msg = e.getMessage() != null ? e.getMessage() : e.getClass().getSimpleName();
            return ResponseEntity.badRequest().body(Map.of("error", msg));
        }
    }
}
