#include "sierrachart.h"

SCDLLName("JIGSAW Export")

/*
    Written by Malykubo and Frozen Tundra in FatCat's Discord Room
*/

SCSFExport scsf_JigsawExport(SCStudyInterfaceRef sc)
{

    SCInputRef vwapStudyRef = sc.Input[0];

    // Configuration
    if (sc.SetDefaults)
    {
        vwapStudyRef.Name = "Select VWAP Study";
        vwapStudyRef.SetStudyID(0);

        return;
    }

    int vwapValueIndex = 0;
    int vwapTopStdDevIndex = 1;
    int vwapBottomStdDevIndex = 2;

    SCFloatArray vwapValueStudyArray;
    
    //Get the first (0) subgraph from the study the user has selected.
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapValueIndex, vwapValueStudyArray) > 0 && vwapValueStudyArray.GetArraySize() > 0) {
        float lastValue = vwapValueStudyArray[sc.UpdateStartIndex];
        SCString log_message;
        log_message.Format("VWAP: %f", lastValue);
        sc.AddMessageToLog(log_message, 1);
    }

    SCFloatArray vwapTopStdDevStudyArray;
    //Get the first (0) subgraph from the study the user has selected.
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapTopStdDevIndex, vwapTopStdDevStudyArray) > 0 && vwapTopStdDevStudyArray.GetArraySize() > 0) {
        float lastValue = vwapTopStdDevStudyArray[sc.UpdateStartIndex];
        SCString log_message;
        log_message.Format("TOP STD DEV: %f", lastValue);
        sc.AddMessageToLog(log_message, 1);
    }

    SCFloatArray vwapBottomStdDevStudyArray;
    //Get the first (0) subgraph from the study the user has selected.
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapBottomStdDevIndex, vwapBottomStdDevStudyArray) > 0 && vwapBottomStdDevStudyArray.GetArraySize() > 0) {
        float lastValue = vwapBottomStdDevStudyArray[sc.UpdateStartIndex];
        SCString log_message;
        log_message.Format("BOTTOM STD DEV: %f", lastValue);
        sc.AddMessageToLog(log_message, 1);
    }
}