package org.me.tulahack.model;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import lombok.Data;

import java.util.List;
import java.util.Map;

@Data
public class RouteRequest {

    private String mode = "categories";

    @NotBlank
    private String startAddress;

    private String endAddress;

    private List<String> categories;

    private Map<String, Integer> categoryPriorities;

    private List<String> places;

    @Min(1) @Max(10)
    private int maxPois = 1;

    private String transportMode = "driving-car";

    private String departureTime = "09:00";

    private String endTime = "18:00";
}
