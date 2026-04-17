// -- Headers --
#include "simple_json.h"

// -- .c --
#include "simple_json.c"

int main()
{
    char *JsonString =
        "{"
        "\"request_id\":\"demo-route-001\","
        "\"city\":\"tula\","
        "\"movement_mode\":\"walking\","
        "\"start_time\":540,"
        "\"end_time\":960,"
        "\"start\":{"
        "    \"id\":0,"
        "    \"lat_e7\":541930331,"
        "    \"lon_e7\":371796396"
        "},"
        "\"pois\":["
        "    {"
        "        \"id\":101,"
        "        \"matrix_index\":1,"
        "        \"lat_e7\":541969120,"
        "        \"lon_e7\":371844820,"
        "        \"score\":10,"
        "        \"visit_duration\":60,"
        "        \"open_time\":600,"
        "        \"close_time\":1080"
        "    },"
        "    {"
        "        \"id\":102,"
        "        \"matrix_index\":2,"
        "        \"lat_e7\":541878340,"
        "        \"lon_e7\":371743710,"
        "        \"score\":7,"
        "        \"visit_duration\":40,"
        "        \"open_time\":480,"
        "        \"close_time\":1200"
        "    },"
        "    {"
        "        \"id\":103,"
        "        \"matrix_index\":3,"
        "        \"lat_e7\":542012000,"
        "        \"lon_e7\":371900000,"
        "        \"score\":8,"
        "        \"visit_duration\":50,"
        "        \"open_time\":720,"
        "        \"close_time\":1020"
        "    }"
        "],"
        "\"travel_time_matrix\":["
        "    [0,12,8,20],"
        "    [12,0,15,9],"
        "    [8,15,0,18],"
        "    [20,9,18,0]"
        "]"
        "}";

    ast_node *AST = Marshal(JsonString);

    ast_node *RequestID = shget(AST->JsonObj, "request_id");
    ast_node *City = shget(AST->JsonObj, "city");
    ast_node *MovementMode = shget(AST->JsonObj, "movement_mode");
    ast_node *StartTime = shget(AST->JsonObj, "start_time");
    ast_node *EndTime = shget(AST->JsonObj, "end_time");

    printf("request_id: %s\n", RequestID->JsonStr);
    printf("city: %s\n", City->JsonStr);
    printf("movement_mode: %s\n", MovementMode->JsonStr);
    printf("time window: %d..%d\n", StartTime->JsonNum, EndTime->JsonNum);

    ast_node *Start = shget(AST->JsonObj, "start");
    ast_node *StartID = shget(Start->JsonObj, "id");
    ast_node *StartLat = shget(Start->JsonObj, "lat_e7");
    ast_node *StartLon = shget(Start->JsonObj, "lon_e7");

    printf("start: id=%d lat_e7=%d lon_e7=%d\n",
           StartID->JsonNum,
           StartLat->JsonNum,
           StartLon->JsonNum);

    ast_node *Pois = shget(AST->JsonObj, "pois");
    printf("pois: %ld\n", arrlen(Pois->JsonArr));

    for(int PoiIndex = 0; PoiIndex < arrlen(Pois->JsonArr); ++PoiIndex)
    {
        ast_node *Poi = Pois->JsonArr[PoiIndex];
        ast_node *ID = shget(Poi->JsonObj, "id");
        ast_node *MatrixIndex = shget(Poi->JsonObj, "matrix_index");
        ast_node *Score = shget(Poi->JsonObj, "score");
        ast_node *VisitDuration = shget(Poi->JsonObj, "visit_duration");
        ast_node *OpenTime = shget(Poi->JsonObj, "open_time");
        ast_node *CloseTime = shget(Poi->JsonObj, "close_time");

        printf("poi[%d]: id=%d matrix_index=%d score=%d duration=%d window=%d..%d\n",
               PoiIndex,
               ID->JsonNum,
               MatrixIndex->JsonNum,
               Score->JsonNum,
               VisitDuration->JsonNum,
               OpenTime->JsonNum,
               CloseTime->JsonNum);
    }

    ast_node *TravelTimeMatrix = shget(AST->JsonObj, "travel_time_matrix");
    ast_node *FirstRow = TravelTimeMatrix->JsonArr[0];
    ast_node *StartToFirstPoi = FirstRow->JsonArr[1];

    printf("travel start->poi101: %d minutes\n", StartToFirstPoi->JsonNum);

    // Too slow right now. Parser arenas can be freed later when ownership is cleaned up.
    // We use all AST data through the program, so we do not really need clean it up.
    // FreeJsonASTRecursively(AST);

    return 0;
}
