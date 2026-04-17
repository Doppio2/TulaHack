package org.me.tulahack.model;

import jakarta.persistence.*;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;

@Entity
@Table(name = "routes")
@Data
@Builder
@NoArgsConstructor
@AllArgsConstructor
public class Route {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    private String startAddress;
    private String endAddress;

    private String categories;

    private double originalDistance;
    private double optimizedDistance;
    private double savingPercent;

    private String createdAt;

    @Column(columnDefinition = "TEXT")
    private String poisJson;
}
