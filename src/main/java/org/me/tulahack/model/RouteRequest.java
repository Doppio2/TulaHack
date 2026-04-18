package org.me.tulahack.model;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotEmpty;
import jakarta.validation.constraints.Max;
import jakarta.validation.constraints.Min;
import lombok.Data;

import java.util.List;

@Data
public class RouteRequest {

    @NotBlank
    private String startAddress;

    @NotBlank
    private String endAddress;

    @NotEmpty
    private List<String> categories;

    @Min(1) @Max(10)
    private int maxPois = 1;

    private String transportMode = "driving-car";

    private String departureTime = "09:00";

    private String endTime = "18:00";
}
