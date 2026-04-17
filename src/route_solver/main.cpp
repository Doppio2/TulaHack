// -- Headers --
#include "simple_json.h"

// -- .c --
#include "simple_json.c"

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

    time_min FinishTime;
};

func int
GetMatrixValue(int *Matrix, int PointCount, int FromIndex, int ToIndex)
{
    int Result = Matrix[FromIndex*PointCount+ToIndex];

    return Result;
}

func int
GetTravelDuration(travel_matrix *Matrix, int FromIndex, int ToIndex)
{
    int Result = GetMatrixValue(Matrix->Durations, Matrix->PointCount, FromIndex, ToIndex);

    return Result;
}

func int
GetTravelDistance(travel_matrix *Matrix, int FromIndex, int ToIndex)
{
    int Result = GetMatrixValue(Matrix->Distances, Matrix->PointCount, FromIndex, ToIndex);

    return Result;
}

// ???????
func int
GetDefaultScore(route_point *Point)
{
    int Result = 1;

    if(Point->Rating > 0)
    {
        Result = (int)(Point->Rating * 2.0f);
    }

    return Result;
}

func time_min
GetDefaultVisitDuration(route_point *Point)
{
    time_min Result = 30;

    if(Point->Category && strcmp(Point->Category, "restaurant") == 0)
    {
        Result = 60;
    }

    return Result;
}

func void
SetDefaultRoutePointSchedule(route_point *Point)
{
    // TODO: Parse WorkingHours later. Current JSON has human-readable text.
    Point->OpenTime = 0;
    Point->CloseTime = 1440;
}

func void
BuildInputRouteDataFromJson(ast_node *AST, input_route_data *InputRouteData, arena *Arena)
{
    ast_node *PointsNode = shget(AST->JsonObj, "points");

    InputRouteData->PointCount = (int)arrlen(PointsNode->JsonArr);
    InputRouteData->Points = PushArray(Arena, route_point, InputRouteData->PointCount);

    for(int PointIndex = 0;
        PointIndex < InputRouteData->PointCount;
        ++PointIndex)
    {
        ast_node *PointNode = PointsNode->JsonArr[PointIndex];
        route_point *Point = InputRouteData->Points + PointIndex;

        ast_node *ID = shget(PointNode->JsonObj, "id");
        ast_node *Name = shget(PointNode->JsonObj, "name");
        ast_node *Lat = shget(PointNode->JsonObj, "lat");
        ast_node *Lon = shget(PointNode->JsonObj, "lon");
        ast_node *Category = shget(PointNode->JsonObj, "category");
        ast_node *Rubric = shget(PointNode->JsonObj, "rubric");
        ast_node *WorkingHours = shget(PointNode->JsonObj, "working_hours");
        ast_node *Rating = shget(PointNode->JsonObj, "rating");
        ast_node *Reviews = shget(PointNode->JsonObj, "reviews");

        Point->ID = ID->JsonStr;
        Point->Name = Name->JsonStr;
        Point->Lat = Lat->JsonFloat;
        Point->Lon = Lon->JsonFloat;
        Point->Category = Category->JsonStr;
        Point->Rubric = Rubric->JsonStr;
        Point->WorkingHours = WorkingHours->JsonStr;
        Point->Rating = (f32)Rating->JsonFloat;
        Point->Reviews = Reviews->JsonNum;
        Point->MatrixIndex = PointIndex;

        Point->Score = GetDefaultScore(Point);
        Point->VisitDuration = GetDefaultVisitDuration(Point);
        SetDefaultRoutePointSchedule(Point);
    }

    ast_node *TravelMatrixNode = shget(AST->JsonObj, "travel_matrix");
    ast_node *DistancesNode = shget(TravelMatrixNode->JsonObj, "distances");
    ast_node *DurationsNode = shget(TravelMatrixNode->JsonObj, "durations");

    int MatrixValueCount = InputRouteData->PointCount * InputRouteData->PointCount;

    InputRouteData->TravelMatrix.PointCount = InputRouteData->PointCount;
    InputRouteData->TravelMatrix.Distances = PushArray(Arena, int, MatrixValueCount);
    InputRouteData->TravelMatrix.Durations = PushArray(Arena, int, MatrixValueCount);

    for(int FromIndex = 0;
        FromIndex < InputRouteData->PointCount;
        ++FromIndex)
    {
        ast_node *DistanceRow = DistancesNode->JsonArr[FromIndex];
        ast_node *DurationRow = DurationsNode->JsonArr[FromIndex];

        for(int ToIndex = 0;
            ToIndex < InputRouteData->PointCount;
            ++ToIndex)
        {
            int MatrixIndex = FromIndex * InputRouteData->PointCount + ToIndex;

            InputRouteData->TravelMatrix.Distances[MatrixIndex] = DistanceRow->JsonArr[ToIndex]->JsonNum;
            InputRouteData->TravelMatrix.Durations[MatrixIndex] = DurationRow->JsonArr[ToIndex]->JsonNum;
        }
    }

    // NOTE(denis): Current JSON has no explicit route request fields.
    InputRouteData->StartMatrixIndex = 0;
    InputRouteData->StartTime = 540;
    InputRouteData->EndTime = 1080;
}

func void
SolveRoute(input_route_data *InputRouteData, route_result *RouteResult, arena *Arena)
{
    // TODO: This is the first algorithm-level function to write.
    // NOTE(denis): Expected future call order:
    // 1. ValidateInputRouteData(InputRouteData)
    // 2. PrepareSolverScratch(InputRouteData, Arena)
    // 3. SolveGreedyRoute(InputRouteData, RouteResult, Arena)
    // 4. MaybeImproveRouteByLocalSearch(InputRouteData, RouteResult, Arena)
    // 5. BuildJsonRouteResponse(RouteResult)

    RouteResult->VisitCount = 0;
    RouteResult->TotalScore = 0;
    RouteResult->TotalTravelTime = 0;
    RouteResult->TotalWaitingTime = 0;
    RouteResult->FinishTime = InputRouteData->StartTime;

    (void)Arena;
}

func void
PrintInputRouteData(input_route_data *InputRouteData)
{
    printf("point_count: %d\n", InputRouteData->PointCount);
    printf("start_matrix_index: %d\n", InputRouteData->StartMatrixIndex);
    printf("time window: %d..%d\n", InputRouteData->StartTime, InputRouteData->EndTime);

    for(int PointIndex = 0;
        PointIndex < InputRouteData->PointCount;
        ++PointIndex)
    {
        route_point *Point = InputRouteData->Points + PointIndex;

        printf(
            "point[%d]: id=%s name=%s lat=%f lon=%f category=%s rating=%f reviews=%d score=%d duration=%d window=%d..%d\n",
            PointIndex,
            Point->ID,
            Point->Name,
            Point->Lat,
            Point->Lon,
            Point->Category,
            Point->Rating,
            Point->Reviews,
            Point->Score,
            Point->VisitDuration,
            Point->OpenTime,
            Point->CloseTime
        );
    }

    printf(
        "distance[0][1]: %d meters\n",
        GetTravelDistance(&InputRouteData->TravelMatrix, 0, 1)
    );

    printf(
        "duration[0][1]: %d seconds\n",
        GetTravelDuration(&InputRouteData->TravelMatrix, 0, 1)
    );
}

int main()
{
    char *JsonString =
        "{"
        "    \"points\": ["
        "        {"
        "            \"id\": \"123456\","
        "            \"name\": \"Ресторан Пушкин\","
        "            \"lat\": 55.7558,"
        "            \"lon\": 37.6176,"
        "            \"category\": \"restaurant\","
        "            \"rubric\": \"Ресторан\","
        "            \"working_hours\": \"Пн-Вс 10:00-22:00\","
        "            \"rating\": 4.7,"
        "            \"reviews\": 1234"
        "        },"
        "        {"
        "            \"id\": \"789012\","
        "            \"name\": \"Музей Оружия\","
        "            \"lat\": 54.2045,"
        "            \"lon\": 37.6188,"
        "            \"category\": \"museum\","
        "            \"rubric\": \"Музей\","
        "            \"working_hours\": \"Пн-Вс 10:00-20:00\","
        "            \"rating\": 4.8,"
        "            \"reviews\": 2500"
        "        },"
        "        {"
        "            \"id\": \"345678\","
        "            \"name\": \"Центральный парк\","
        "            \"lat\": 54.1810,"
        "            \"lon\": 37.5900,"
        "            \"category\": \"park\","
        "            \"rubric\": \"Парк\","
        "            \"working_hours\": \"Пн-Вс 00:00-23:59\","
        "            \"rating\": 4.6,"
        "            \"reviews\": 3100"
        "        }"
        "    ],"
        "    \"travel_matrix\": {"
        "        \"distances\": ["
        "            [0, 1234, 5678],"
        "            [1234, 0, 4321],"
        "            [5678, 4321, 0]"
        "        ],"
        "        \"durations\": ["
        "            [0, 120, 300],"
        "            [120, 0, 250],"
        "            [300, 250, 0]"
        "        ]"
        "    }"
        "}";

    arena *RouteArena = ArenaAlloc(Megabytes(1));

    ast_node *AST = Marshal(JsonString);

    input_route_data InputRouteData = {};
    BuildInputRouteDataFromJson(AST, &InputRouteData, RouteArena);

    route_result RouteResult = {};
    RouteResult.Visits = PushArray(RouteArena, visit, InputRouteData.PointCount);

    PrintInputRouteData(&InputRouteData);
    SolveRoute(&InputRouteData, &RouteResult, RouteArena);

    // Too slow right now. Parser arenas can be freed later when ownership is cleaned up.
    // We use all AST data through the program, so we do not really need clean it up.
    // FreeJsonASTRecursively(AST);

    ArenaFree(RouteArena);

    return 0;
}
