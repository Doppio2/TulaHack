package org.me.tulahack.model;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.util.List;

@Data
@NoArgsConstructor
@AllArgsConstructor
public class TravelMatrix {
    private List<List<Double>> distances;
    private List<List<Double>> durations;
}
