package org.me.tulahack.model;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

import java.util.List;

@Data
@Builder
@NoArgsConstructor
@AllArgsConstructor
public class PointOfInterest {

    private String id;

    private String name;

    private String address;

    private double lat;
    private double lon;

    private String category;

    private String rubric;

    private String workingHours;

    private Double rating;

    private Integer reviews;

    private String contacts;

    private List<String> photos;
}
