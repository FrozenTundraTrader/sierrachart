#include "sierrachart.h"
#include <vector>

SCDLLName("Frozen Tundra - Auto Bar Period")

/*
    Written by Frozen Tundra in FatCat's Discord Room
*/

// Global BarPeriod struct to share between source and target
n_ACSIL::s_BarPeriod g_SourceBarPeriod;

// semaphore to flip when target needs updating
bool g_NeedToUpdateTarget = false;

// keep track of chart ids
std::vector<int> g_TargetChartIds;

SCSFExport scsf_AutoBarPeriod(SCStudyInterfaceRef sc)
{

    SCString msg;
    int InputIdx = 0;
    SCInputRef i_SourceOrTarget = sc.Input[++InputIdx];
    SCInputRef i_BarPeriodMultiple = sc.Input[++InputIdx];
    SCInputRef i_BarPeriodDivisor = sc.Input[++InputIdx];

    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;
        sc.GraphShortName = "Auto Bar Period";

        i_SourceOrTarget.Name = "Source Or Target?";
        i_SourceOrTarget.SetCustomInputStrings("Source;Target");
        i_SourceOrTarget.SetCustomInputIndex(0);

        i_BarPeriodMultiple.Name = "TARGET ONLY: Custom Bar Period Multiple";
        i_BarPeriodMultiple.SetInt(1);

        i_BarPeriodDivisor.Name = "TARGET ONLY: Custom Bar Period Divisor";
        i_BarPeriodDivisor.SetInt(6);
        return;
    }

    // 0 = source
    // 1 = target
    int SourceOrTarget = i_SourceOrTarget.GetInt();

    // custom denominator value from input
    int CustomMultiple = i_BarPeriodMultiple.GetInt();
    int CustomDivisor = i_BarPeriodDivisor.GetInt();

    if (sc.Index == 0 && SourceOrTarget == 1) {
        // make sure this chart id exists in the array of targets
        bool AlreadyAdded = false;
        for (int i=0; i<g_TargetChartIds.size(); i++) {
            if (g_TargetChartIds[i] == sc.ChartNumber) {
                AlreadyAdded = true;
            }
        }
        if (!AlreadyAdded) {
            g_TargetChartIds.push_back(sc.ChartNumber);
        }
    }

    // the source chart
    if (SourceOrTarget == 0 && g_NeedToUpdateTarget == false) {

        // load source chart bar period parameters so that target may fetch them later
        int PrevCurrentBarPeriodInSeconds = g_SourceBarPeriod.IntradayChartBarPeriodParameter1;

        // get current chart bar period parameters
        sc.GetBarPeriodParameters(g_SourceBarPeriod);

        // get current val in seconds
        int CurrentBarPeriodInSeconds = g_SourceBarPeriod.IntradayChartBarPeriodParameter1;

        // force update if we saw a change in bar period
        if (PrevCurrentBarPeriodInSeconds != CurrentBarPeriodInSeconds) {
            // we need to update, flip the global semaphore
            g_NeedToUpdateTarget = true;
        }
    }
    // the target chart
    else if (SourceOrTarget == 1 && g_NeedToUpdateTarget == true) {

        // grab the source chart's BarPeriod struct from the global
        int CurrentBarPeriodInSeconds = g_SourceBarPeriod.IntradayChartBarPeriodParameter1;

        // make a copy of it b/c we want to update the local target chart here
        n_ACSIL::s_BarPeriod TargetBarPeriod = g_SourceBarPeriod;

        // divide the source chart's bar period by this custom number
        // credit to AlohaDave: "divide by 6"
        if (CustomMultiple != 1) {
            TargetBarPeriod.IntradayChartBarPeriodParameter1 *= CustomMultiple;
        }
        if (CustomDivisor != 1) {
            TargetBarPeriod.IntradayChartBarPeriodParameter1 /= CustomDivisor;
        }

        // set it
        sc.SetBarPeriodParameters(TargetBarPeriod);

        // remove this chart from this list
        for (int i=0; i<g_TargetChartIds.size(); i++) {
            if (g_TargetChartIds[i] == sc.ChartNumber) {
                g_TargetChartIds.erase(g_TargetChartIds.begin() + i);
            }
        }

        if (g_TargetChartIds.size() == 0) {
            // flip our semaphore back
            g_NeedToUpdateTarget = false;
        }
    }
}

