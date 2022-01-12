#include "sierrachart.h"

SCDLLName("Frozen Tundra - Auto Volume by Price")

/*
    Written by Malykubo and Frozen Tundra in FatCat's Discord Room
*/

SCSFExport scsf_AutoVbP(SCStudyInterfaceRef sc)
{

    // which study to reference
    SCInputRef i_StudyRef = sc.Input[0];

    // magic number multiplier that will determine the granularity of VP bars we'll see as we switch symbols
    // the lower the number, the thicker and fewer bars there will be
    // the higher the magic number, the thinner and more VP bars there will be
    SCInputRef i_TickMultiplier = sc.Input[1];

    // option for tick calculations based only on vap
    SCInputRef useVap = sc.Input[2];

    // logging object
    SCString log_message;

    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;
        i_StudyRef.Name = "Select Target Study";
        i_StudyRef.SetStudyID(0);
        i_TickMultiplier.Name = "Magic Number Multiplier";
        i_TickMultiplier.SetFloat(1.3);
        useVap.Name = "Use VAP instead of magic";
        useVap.SetYesNo(0);

        return;
    }

    int studyId = i_StudyRef.GetStudyID();

    // VbP Ticks Per Volume Bar is input 32, ID 31
    int inputIdx = 31;

    if (useVap.GetInt() == 1) {
        sc.SetChartStudyInputInt(sc.ChartNumber, studyId, inputIdx, sc.VolumeAtPriceMultiplier);
        return;
    }

    float vHigh, vLow, vDiff;
    float vMultiplier = i_TickMultiplier.GetFloat();

    // fetch the graph's price scale's high and low value so we can automate the Ticks setting on VbP
    sc.GetMainGraphVisibleHighAndLow(vHigh, vLow);

    // calc the range of visible prices
    vDiff = (vHigh - vLow);

    // divide range by magic number to get the desired VbP Ticks Per Bar value
    int targetTicksPerBar = max(sc.Round(vDiff / vMultiplier), 1);

    //log_message.Format("H=%f, L=%f, Diff=%f, target=%d", vHigh, vLow, vDiff, targetTicksPerBar);
    //sc.AddMessageToLog(log_message, 1);

    int prevValue;
    sc.GetChartStudyInputInt(sc.ChartNumber, studyId, inputIdx, prevValue);

    if (abs(prevValue - targetTicksPerBar) > (targetTicksPerBar * 0.20)) {
        sc.SetChartStudyInputInt(sc.ChartNumber, studyId, inputIdx, targetTicksPerBar);
        sc.RecalculateChart(sc.ChartNumber);
    }
}