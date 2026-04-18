package org.me.tulahack.model;

import lombok.Builder;
import lombok.Data;

import java.util.List;
import java.util.Map;

@Data
@Builder
public class RouteResponse {

    private Long routeId;

    private double originalDistance;

    private double optimizedDistance;

    private double savingPercent;

    private List<PointOfInterest> pois;

    private List<Coordinate> orderedCoordinates;

    private List<ScheduleEntry> schedule;

    private Map<String, Object> geojson;

    private Map<String, Object> naiveGeojson;

    private Long computationMs;
}
