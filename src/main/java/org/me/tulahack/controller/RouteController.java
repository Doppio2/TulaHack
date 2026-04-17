package org.me.tulahack.controller;

import jakarta.validation.Valid;
import org.me.tulahack.model.Route;
import org.me.tulahack.model.RouteRequest;
import org.me.tulahack.model.RouteResponse;
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

    public RouteController(RouteService routeService, GeocodingService geocodingService) {
        this.routeService = routeService;
        this.geocodingService = geocodingService;
    }

    @PostMapping("/route/build")
    public ResponseEntity<?> buildRoute(@Valid @RequestBody RouteRequest request) {
        try {
            RouteResponse response = routeService.buildRoute(request);
            return ResponseEntity.ok(response);
        } catch (Exception e) {
            return ResponseEntity.badRequest().body(Map.of("error", e.getMessage()));
        }
    }

    @GetMapping("/route/history")
    public ResponseEntity<List<Route>> getHistory() {
        return ResponseEntity.ok(routeService.getHistory());
    }

    @GetMapping("/poi/categories")
    public ResponseEntity<List<String>> getCategories() {
        return ResponseEntity.ok(PoiService.getAvailableCategories());
    }

    @PostMapping("/geocode")
    public ResponseEntity<?> geocode(@RequestBody Map<String, String> body) {
        try {
            String address = body.get("address");
            return ResponseEntity.ok(geocodingService.geocode(address));
        } catch (Exception e) {
            return ResponseEntity.badRequest().body(Map.of("error", e.getMessage()));
        }
    }
}
