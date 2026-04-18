#include "route_optimizer.h"

func f64
GetTime()
{
    struct timespec TimeSpec;
    clock_gettime(CLOCK_MONOTONIC, &TimeSpec);

    f64 Result = (f64)TimeSpec.tv_sec + (f64)TimeSpec.tv_nsec * 1e-9;

    return Result;
}

func bool
IsAsciiDigit(char Ch)
{
    bool Result = (Ch >= '0' && Ch <= '9');

    return Result;
}

func time_min
SecondsToMinutesCeil(int Seconds)
{
    time_min Result = 0;

    if(Seconds > 0)
    {
        Result = (Seconds + 59) / 60;
    }

    return Result;
}

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

func time_min
GetTravelDurationMinutes(travel_matrix *Matrix, int FromIndex, int ToIndex)
{
    int Seconds = GetTravelDuration(Matrix, FromIndex, ToIndex);
    time_min Result = SecondsToMinutesCeil(Seconds);

    return Result;
}

func int
GetJsonIntOrDefault(ast_node *ObjectNode, char *Key, int DefaultValue)
{
    int Result = DefaultValue;
    ast_node *Value = shget(ObjectNode->JsonObj, Key);

    if(Value)
    {
        Result = Value->JsonNum;
    }

    return Result;
}

func char *
ReadEntireFile(char *FilePath, arena *Arena)
{
    char *Result = 0;
    FILE *File = fopen(FilePath, "rb");

    if(File)
    {
        fseek(File, 0, SEEK_END);
        long FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        if(FileSize > 0)
        {
            Result = PushArray(Arena, char, FileSize + 1);
            size_t BytesRead = fread(Result, 1, FileSize, File);
            Result[BytesRead] = 0;
        }

        fclose(File);
    }

    return Result;
}

func bool
TryParseTwoDigitNumber(char *String, int *Index, int *Value)
{
    bool Result = false;

    char FirstChar = String[*Index];
    char SecondChar = String[*Index + 1];

    if(IsAsciiDigit(FirstChar) && IsAsciiDigit(SecondChar))
    {
        *Value = (FirstChar - '0') * 10 + (SecondChar - '0');
        *Index += 2;
        Result = true;
    }

    return Result;
}

func bool
TryParseTime(char *String, int *Index, time_min *Value)
{
    bool Result = false;

    int Hour = 0;
    int Minute = 0;
    int CurrentIndex = *Index;

    if(TryParseTwoDigitNumber(String, &CurrentIndex, &Hour) &&
       String[CurrentIndex] == ':')
    {
        ++CurrentIndex;

        if(TryParseTwoDigitNumber(String, &CurrentIndex, &Minute))
        {
            if(Hour >= 0 && Hour <= 24 && Minute >= 0 && Minute <= 59)
            {
                *Value = Hour * 60 + Minute;
                *Index = CurrentIndex;
                Result = true;
            }
        }
    }

    return Result;
}

func bool
TrySkipTimeRangeSeparator(char *String, int *Index)
{
    bool Result = false;

    if(String[*Index] == '-')
    {
        ++(*Index);
        Result = true;
    }
    else if((u8)String[*Index] == 0xE2 &&
            (u8)String[*Index + 1] == 0x80 &&
            (u8)String[*Index + 2] == 0x93)
    {
        *Index += 3;
        Result = true;
    }

    return Result;
}

func bool
TryParseFirstTimeWindow(char *String, time_min *OpenTime, time_min *CloseTime)
{
    bool Result = false;

    for(int CharIndex = 0;
        String[CharIndex] != 0;
        ++CharIndex)
    {
        int CurrentIndex = CharIndex;
        time_min ParsedOpenTime = 0;
        time_min ParsedCloseTime = 0;

        if(TryParseTime(String, &CurrentIndex, &ParsedOpenTime) &&
           TrySkipTimeRangeSeparator(String, &CurrentIndex))
        {
            if(TryParseTime(String, &CurrentIndex, &ParsedCloseTime))
            {
                *OpenTime = ParsedOpenTime;
                *CloseTime = ParsedCloseTime;
                Result = true;
                break;
            }
        }
    }

    return Result;
}

func int
GetDefaultScore(route_point *Point)
{
    int Result = 1;

    if(Point->Category && strcmp(Point->Category, "restaurant") == 0)
    {
        Result = 5;
    }
    else if(Point->Category && strcmp(Point->Category, "museum") == 0)
    {
        Result = 10;
    }
    else if(Point->Category && strcmp(Point->Category, "park") == 0)
    {
        Result = 7;
    }

    if(Point->Rating > 0)
    {
        Result += (int)Point->Rating;
    }

    if(Point->Reviews > 1000)
    {
        Result += 1;
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
    else if(Point->Category && strcmp(Point->Category, "museum") == 0)
    {
        Result = 90;
    }
    else if(Point->Category && strcmp(Point->Category, "park") == 0)
    {
        Result = 45;
    }

    return Result;
}

func void
SetDefaultRoutePointSchedule(route_point *Point)
{
    time_min OpenTime = 0;
    time_min CloseTime = 1440;

    if(Point->WorkingHours)
    {
        TryParseFirstTimeWindow(Point->WorkingHours, &OpenTime, &CloseTime);
    }

    Point->OpenTime = OpenTime;
    Point->CloseTime = CloseTime;
}

func bool
ValidateInputRouteData(input_route_data *InputRouteData)
{
    bool Result = true;

    if(InputRouteData->PointCount <= 0)
    {
        Result = false;
    }

    if(InputRouteData->Points == 0)
    {
        Result = false;
    }

    if(InputRouteData->TravelMatrix.PointCount <= 0)
    {
        Result = false;
    }

    if(InputRouteData->TravelMatrix.Distances == 0 ||
       InputRouteData->TravelMatrix.Durations == 0)
    {
        Result = false;
    }

    if(InputRouteData->StartMatrixIndex < 0 ||
       InputRouteData->StartMatrixIndex >= InputRouteData->TravelMatrix.PointCount)
    {
        Result = false;
    }

    if(InputRouteData->EndMatrixIndex < 0 ||
       InputRouteData->EndMatrixIndex >= InputRouteData->TravelMatrix.PointCount)
    {
        Result = false;
    }

    if(InputRouteData->StartTime > InputRouteData->EndTime)
    {
        Result = false;
    }

    return Result;
}

func void
BuildInputRouteDataFromJson(ast_node *AST, input_route_data *InputRouteData, arena *Arena)
{
    ast_node *PointsNode = shget(AST->JsonObj, "points");

    InputRouteData->StartMatrixIndex = GetJsonIntOrDefault(AST, "start_index", 0);
    InputRouteData->PointsOffset = GetJsonIntOrDefault(AST, "points_offset", 1);

    InputRouteData->PointCount = (int)arrlen(PointsNode->JsonArr);
    InputRouteData->EndMatrixIndex = GetJsonIntOrDefault(
        AST,
        "end_index",
        InputRouteData->PointCount + InputRouteData->PointsOffset
    );

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
        Point->MatrixIndex = PointIndex + InputRouteData->PointsOffset;

        Point->Score = GetDefaultScore(Point);
        Point->VisitDuration = GetDefaultVisitDuration(Point);
        SetDefaultRoutePointSchedule(Point);
    }

    ast_node *TravelMatrixNode = shget(AST->JsonObj, "travel_matrix");
    ast_node *DistancesNode = shget(TravelMatrixNode->JsonObj, "distances");
    ast_node *DurationsNode = shget(TravelMatrixNode->JsonObj, "durations");

    int MatrixPointCount = (int)arrlen(DurationsNode->JsonArr);
    int MatrixValueCount = MatrixPointCount * MatrixPointCount;

    InputRouteData->TravelMatrix.PointCount = MatrixPointCount;
    InputRouteData->TravelMatrix.Distances = PushArray(Arena, int, MatrixValueCount);
    InputRouteData->TravelMatrix.Durations = PushArray(Arena, int, MatrixValueCount);

    for(int FromIndex = 0;
        FromIndex < MatrixPointCount;
        ++FromIndex)
    {
        ast_node *DistanceRow = DistancesNode->JsonArr[FromIndex];
        ast_node *DurationRow = DurationsNode->JsonArr[FromIndex];

        for(int ToIndex = 0;
            ToIndex < MatrixPointCount;
            ++ToIndex)
        {
            int MatrixIndex = FromIndex * MatrixPointCount + ToIndex;

            InputRouteData->TravelMatrix.Distances[MatrixIndex] = DistanceRow->JsonArr[ToIndex]->JsonNum;
            InputRouteData->TravelMatrix.Durations[MatrixIndex] = DurationRow->JsonArr[ToIndex]->JsonNum;
        }
    }

    InputRouteData->StartTime = GetJsonIntOrDefault(AST, "start_time", 540);
    InputRouteData->EndTime = GetJsonIntOrDefault(AST, "end_time", 1080);
}

func bool
EvaluateCandidate(
    input_route_data *InputRouteData,
    int CurrentMatrixIndex,
    time_min CurrentTime,
    int PointIndex,
    candidate_eval *Eval
)
{
    bool Result = false;
    route_point *CandidatePoint = InputRouteData->Points + PointIndex;

    int TravelTime = GetTravelDurationMinutes(
        &InputRouteData->TravelMatrix,
        CurrentMatrixIndex,
        CandidatePoint->MatrixIndex
    );
    int TravelDistance = GetTravelDistance(
        &InputRouteData->TravelMatrix,
        CurrentMatrixIndex,
        CandidatePoint->MatrixIndex
    );

    time_min ArrivalTime = CurrentTime + TravelTime;
    time_min VisitStartTime = ArrivalTime;

    if(VisitStartTime < CandidatePoint->OpenTime)
    {
        VisitStartTime = CandidatePoint->OpenTime;
    }

    int WaitingTime = VisitStartTime - ArrivalTime;
    time_min VisitEndTime = VisitStartTime + CandidatePoint->VisitDuration;
    time_min EndTravelTime = GetTravelDurationMinutes(
        &InputRouteData->TravelMatrix,
        CandidatePoint->MatrixIndex,
        InputRouteData->EndMatrixIndex
    );
    time_min FinishWithEndTime = VisitEndTime + EndTravelTime;

    if(VisitEndTime <= CandidatePoint->CloseTime &&
       FinishWithEndTime <= InputRouteData->EndTime)
    {
        int AddedTime = TravelTime + WaitingTime + CandidatePoint->VisitDuration;

        if(AddedTime > 0)
        {
            Eval->PointIndex = PointIndex;
            Eval->ArrivalTime = ArrivalTime;
            Eval->VisitStartTime = VisitStartTime;
            Eval->VisitEndTime = VisitEndTime;
            Eval->TravelTime = TravelTime;
            Eval->TravelDistance = TravelDistance;
            Eval->WaitingTime = WaitingTime;
            Eval->AddedTime = AddedTime;
            Eval->Score = CandidatePoint->Score;

            Result = true;
        }
    }

    return Result;
}

func bool
IsCandidateBetter(candidate_eval *Candidate, candidate_eval *Best)
{
    bool Result = false;

    if(Best->PointIndex == -1)
    {
        Result = true;
    }
    else
    {
        s64 Left = (s64)Candidate->Score * (s64)Best->AddedTime;
        s64 Right = (s64)Best->Score * (s64)Candidate->AddedTime;

        if(Left > Right)
        {
            Result = true;
        }
        else if(Left == Right && Candidate->AddedTime < Best->AddedTime)
        {
            Result = true;
        }
    }

    return Result;
}

func void
AppendVisit(input_route_data *InputRouteData, route_result *RouteResult, candidate_eval *Eval)
{
    route_point *SelectedPoint = InputRouteData->Points + Eval->PointIndex;
    visit *NewVisit = RouteResult->Visits + RouteResult->VisitCount;

    NewVisit->PointID = SelectedPoint->ID;
    NewVisit->PointIndex = Eval->PointIndex;
    NewVisit->MatrixIndex = SelectedPoint->MatrixIndex;
    NewVisit->ArrivalTime = Eval->ArrivalTime;
    NewVisit->VisitStartTime = Eval->VisitStartTime;
    NewVisit->VisitEndTime = Eval->VisitEndTime;
    NewVisit->TravelFromPrevious = Eval->TravelTime;
    NewVisit->WaitingTime = Eval->WaitingTime;
    NewVisit->Score = SelectedPoint->Score;

    ++RouteResult->VisitCount;

    RouteResult->TotalScore += SelectedPoint->Score;
    RouteResult->TotalTravelTime += Eval->TravelTime;
    RouteResult->TotalDistance += Eval->TravelDistance;
    RouteResult->TotalWaitingTime += Eval->WaitingTime;
    RouteResult->FinishTime = Eval->VisitEndTime;
}

func void
SolveGreedyRoute(input_route_data *InputRouteData, route_result *RouteResult, arena *Arena)
{
    unsigned char *Visited = PushArray(Arena, unsigned char, InputRouteData->PointCount);

    int CurrentMatrixIndex = InputRouteData->StartMatrixIndex;
    time_min CurrentTime = InputRouteData->StartTime;

    while(true)
    {
        candidate_eval BestEval = {};
        BestEval.PointIndex = -1;

        for(int PointIndex = 0;
            PointIndex < InputRouteData->PointCount;
            ++PointIndex)
        {
            if(Visited[PointIndex])
            {
                continue;
            }

            candidate_eval Eval = {};
            bool IsFeasible = EvaluateCandidate(
                InputRouteData,
                CurrentMatrixIndex,
                CurrentTime,
                PointIndex,
                &Eval
            );

            if(!IsFeasible)
            {
                continue;
            }

            if(IsCandidateBetter(&Eval, &BestEval))
            {
                BestEval = Eval;
            }
        }

        if(BestEval.PointIndex == -1)
        {
            break;
        }

        AppendVisit(InputRouteData, RouteResult, &BestEval);

        route_point *SelectedPoint = InputRouteData->Points + BestEval.PointIndex;

        Visited[BestEval.PointIndex] = 1;
        CurrentMatrixIndex = SelectedPoint->MatrixIndex;
        CurrentTime = BestEval.VisitEndTime;
    }

    if(InputRouteData->EndMatrixIndex >= 0 &&
       InputRouteData->EndMatrixIndex < InputRouteData->TravelMatrix.PointCount)
    {
        int TravelToEnd = GetTravelDurationMinutes(
            &InputRouteData->TravelMatrix,
            CurrentMatrixIndex,
            InputRouteData->EndMatrixIndex
        );
        int DistanceToEnd = GetTravelDistance(
            &InputRouteData->TravelMatrix,
            CurrentMatrixIndex,
            InputRouteData->EndMatrixIndex
        );

        RouteResult->TotalTravelTime += TravelToEnd;
        RouteResult->TotalDistance += DistanceToEnd;
        RouteResult->FinishTime = CurrentTime + TravelToEnd;
    }
}

func void
SolveRoute(input_route_data *InputRouteData, route_result *RouteResult, arena *Arena)
{
    RouteResult->VisitCount = 0;
    RouteResult->TotalScore = 0;
    RouteResult->TotalTravelTime = 0;
    RouteResult->TotalWaitingTime = 0;
    RouteResult->TotalDistance = 0;
    RouteResult->FinishTime = InputRouteData->StartTime;

    if(ValidateInputRouteData(InputRouteData))
    {
        SolveGreedyRoute(InputRouteData, RouteResult, Arena);
    }
}

func void
PrintInputRouteData(input_route_data *InputRouteData)
{
    printf("point_count: %d\n", InputRouteData->PointCount);
    printf("matrix_point_count: %d\n", InputRouteData->TravelMatrix.PointCount);
    printf("start_matrix_index: %d\n", InputRouteData->StartMatrixIndex);
    printf("end_matrix_index: %d\n", InputRouteData->EndMatrixIndex);
    printf("points_offset: %d\n", InputRouteData->PointsOffset);
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

func void
PrintRouteResult(input_route_data *InputRouteData, route_result *RouteResult)
{
    printf("\nroute_result:\n");
    printf("visit_count: %d\n", RouteResult->VisitCount);
    printf("total_score: %d\n", RouteResult->TotalScore);
    printf("total_distance: %d meters\n", RouteResult->TotalDistance);
    printf("total_travel_time: %d minutes\n", RouteResult->TotalTravelTime);
    printf("total_waiting_time: %d minutes\n", RouteResult->TotalWaitingTime);
    printf("finish_time: %d\n", RouteResult->FinishTime);

    for(int VisitIndex = 0;
        VisitIndex < RouteResult->VisitCount;
        ++VisitIndex)
    {
        visit *Visit = RouteResult->Visits + VisitIndex;
        route_point *Point = InputRouteData->Points + Visit->PointIndex;

        printf(
            "visit[%d]: id=%s name=%s arrival=%d start=%d end=%d travel=%d wait=%d score=%d\n",
            VisitIndex,
            Visit->PointID,
            Point->Name,
            Visit->ArrivalTime,
            Visit->VisitStartTime,
            Visit->VisitEndTime,
            Visit->TravelFromPrevious,
            Visit->WaitingTime,
            Visit->Score
        );
    }
}

func bool
WriteRouteResultJson(char *FilePath, route_result *RouteResult, long ComputationMs)
{
    bool Result = false;
    FILE *File = fopen(FilePath, "wb");

    if(File)
    {
        fprintf(File, "{\n");
        fprintf(File, "  \"optimized_order\": [");

        for(int VisitIndex = 0;
            VisitIndex < RouteResult->VisitCount;
            ++VisitIndex)
        {
            visit *Visit = RouteResult->Visits + VisitIndex;

            if(VisitIndex > 0)
            {
                fprintf(File, ", ");
            }

            fprintf(File, "%d", Visit->PointIndex);
        }

        fprintf(File, "],\n");
        fprintf(File, "  \"total_distance\": %d,\n", RouteResult->TotalDistance);
        fprintf(File, "  \"computation_ms\": %ld\n", ComputationMs);
        fprintf(File, "}\n");

        fclose(File);
        Result = true;
    }

    return Result;
}
