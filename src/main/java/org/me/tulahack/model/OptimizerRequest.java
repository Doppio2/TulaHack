package org.me.tulahack.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;
import java.util.Map;

@Data
@AllArgsConstructor
public class OptimizerRequest {

    private List<OptimizerPoint> points;

    @JsonProperty("category_priorities")
    private Map<String, Integer> categoryPriorities;

    @JsonProperty("start_index")
    private int startIndex;

    @JsonProperty("end_index")
    private int endIndex;

    @JsonProperty("points_offset")
    private int pointsOffset;

    @JsonProperty("travel_matrix")
    private TravelMatrix travelMatrix;

    @JsonProperty("start_time")
    private int startTime;

    @JsonProperty("end_time")
    private int endTime;
}
