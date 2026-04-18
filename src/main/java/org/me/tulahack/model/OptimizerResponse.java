package org.me.tulahack.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import java.util.List;

@Data
public class OptimizerResponse {

    @JsonProperty("optimized_order")
    private List<Integer> optimizedOrder;

    @JsonProperty("total_distance")
    private double totalDistance;

    @JsonProperty("computation_ms")
    private Long computationMs;

    @JsonProperty("schedule")
    private List<ScheduleEntry> schedule;
}
