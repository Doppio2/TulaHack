package org.me.tulahack.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class ScheduleEntry {

    @JsonProperty("point_index")
    private int pointIndex;

    @JsonProperty("arrival_time")
    private int arrivalTime;

    @JsonProperty("visit_start_time")
    private int visitStartTime;

    @JsonProperty("visit_end_time")
    private int visitEndTime;

    @JsonProperty("travel_from_previous")
    private int travelFromPrevious;

    @JsonProperty("waiting_time")
    private int waitingTime;
}
