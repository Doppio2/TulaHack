package org.me.tulahack.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Builder;
import lombok.Data;

@Data
@Builder
public class OptimizerPoint {
    private String id;
    private String name;
    private double lat;
    private double lon;
    private String category;
    private String rubric;

    @JsonProperty("working_hours")
    private String workingHours;

    private Double rating;
    private Integer reviews;
}
