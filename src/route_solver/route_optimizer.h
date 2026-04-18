#if !defined(_ROUTE_OPTIMIZER_H)
#define _ROUTE_OPTIMIZER_H

#include <stdio.h>
#include <time.h>

#include "simple_json.h"

typedef int time_min;

struct route_point
{
    char *ID;
    char *Name;

    f64 Lat;
    f64 Lon;

    char *Category;
    char *Rubric;
    char *WorkingHours;

    f32 Rating;
    int Reviews;

    int MatrixIndex;

    int Score;
    time_min VisitDuration;
    time_min OpenTime;
    time_min CloseTime;
};

struct travel_matrix
{
    int PointCount;

    int *Distances;
    int *Durations;
};

struct input_route_data
{
    int PointCount;
    route_point *Points;

    travel_matrix TravelMatrix;

    int StartMatrixIndex;
    int EndMatrixIndex;
    int PointsOffset;

    time_min StartTime;
    time_min EndTime;
};

struct visit
{
    char *PointID;
    int PointIndex;
    int MatrixIndex;

    time_min ArrivalTime;
    time_min VisitStartTime;
    time_min VisitEndTime;

    int TravelFromPrevious;
    int WaitingTime;
    int Score;
};

struct route_result
{
    int VisitCount;
    visit *Visits;

    int TotalScore;
    int TotalTravelTime;
    int TotalWaitingTime;
    int TotalDistance;

    time_min FinishTime;
};

struct candidate_eval
{
    int PointIndex;

    time_min ArrivalTime;
    time_min VisitStartTime;
    time_min VisitEndTime;

    int TravelTime;
    int TravelDistance;
    int WaitingTime;
    int AddedTime;
    int Score;
};

func f64 GetTime();
func char *ReadEntireFile(char *FilePath, arena *Arena);

func int GetTravelDuration(travel_matrix *Matrix, int FromIndex, int ToIndex);
func int GetTravelDistance(travel_matrix *Matrix, int FromIndex, int ToIndex);
func time_min GetTravelDurationMinutes(travel_matrix *Matrix, int FromIndex, int ToIndex);

func void BuildInputRouteDataFromJson(ast_node *AST, input_route_data *InputRouteData, arena *Arena);
func void SolveRoute(input_route_data *InputRouteData, route_result *RouteResult, arena *Arena);

func void PrintInputRouteData(input_route_data *InputRouteData);
func void PrintRouteResult(input_route_data *InputRouteData, route_result *RouteResult);
func bool WriteRouteResultJson(char *FilePath, route_result *RouteResult, long ComputationMs);

#endif
