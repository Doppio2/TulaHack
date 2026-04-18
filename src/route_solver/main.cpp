// -- Headers --
#include "route_optimizer.h"

// -- .c --
#include "simple_json.c"
#include "route_optimizer.cpp"

int main(int ArgCount, char **Args)
{
    if(ArgCount < 3)
    {
        printf("usage: %s <input.json> <output.json>\n", Args[0]);

        return 1;
    }

    char *InputFilePath = Args[1];
    char *OutputFilePath = Args[2];

    arena *RouteArena = ArenaAlloc(Megabytes(8));

    char *JsonString = ReadEntireFile(InputFilePath, RouteArena);

    if(JsonString == 0)
    {
        printf("failed to read input file: %s\n", InputFilePath);
        ArenaFree(RouteArena);

        return 1;
    }

    f64 StartTime = GetTime();

    ast_node *AST = Marshal(JsonString);

    input_route_data InputRouteData = {};
    BuildInputRouteDataFromJson(AST, &InputRouteData, RouteArena);

    route_result RouteResult = {};
    RouteResult.Visits = PushArray(RouteArena, visit, InputRouteData.PointCount);

    PrintInputRouteData(&InputRouteData);
    SolveRoute(&InputRouteData, &RouteResult, RouteArena);
    PrintRouteResult(&InputRouteData, &RouteResult);

    f64 EndTime = GetTime();
    u64 ComputationMs = (u64)((EndTime - StartTime) * 1000.0);

    if(!WriteRouteResultJson(OutputFilePath, &RouteResult, ComputationMs))
    {
        printf("failed to write output file: %s\n", OutputFilePath);
        ArenaFree(RouteArena);

        return 1;
    }

    // Too slow right now. Parser arenas can be freed later when ownership is cleaned up.
    // We use all AST data through the program, so we do not really need clean it up.
    // FreeJsonASTRecursively(AST);

    ArenaFree(RouteArena);

    return 0;
}
