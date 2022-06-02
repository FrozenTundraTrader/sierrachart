#include "sierrachart.h"

SCDLLName("Frozen Tundra - Zoom Toggle")

/*
    Written by Frozen Tundra
*/

SCSFExport scsf_ZoomToggle(SCStudyInterfaceRef sc)
{

    // inputs
    SCInputRef i_Enabled = sc.Input[0];
    SCInputRef i_NumBarsWhenZoomed = sc.Input[1];
    SCInputRef i_KeyCodeForZoom = sc.Input[2];

    // logging obj
    SCString msg;

    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphShortName = "Zoom Toggle";
        sc.GraphRegion = 0;

        i_Enabled.Name = "Enabled?";
        i_Enabled.SetYesNo(1);

        i_NumBarsWhenZoomed.Name = "Number of bars to see when zoomed?";
        i_NumBarsWhenZoomed.SetInt(3);

        i_KeyCodeForZoom.Name = "ASCII Key Code for zoom? (ex: [SPACE] is 32, [.] is 46, [TAB] is 9)";
        i_KeyCodeForZoom.SetInt(32);
        i_KeyCodeForZoom.SetIntLimits(0, 127);

        // allow study to receive input events from our keyboard
        sc.ReceiveCharacterEvents = 1;

        return;
    }

    // dont do anything if disabled
    if (i_Enabled.GetInt() == 0) return;

    // fetch initial settings from inputs
    int NumBarsWhenZoomed = i_NumBarsWhenZoomed.GetInt();
    int KeyCodeForZoom = i_KeyCodeForZoom.GetInt();

    // persist these values between ticks
    int &PrevChartBarSpacing = sc.GetPersistentInt(0);
    int &PrevNumBarsWhenZoomed = sc.GetPersistentInt(1);
    int &NumFillSpaceBars = sc.GetPersistentInt(2);

    // grab the chart's current bar spacing
    int ChartBarSpacing = sc.ChartBarSpacing;

    // grab the chart's current fill space number of bars
    if (!NumFillSpaceBars) {
        NumFillSpaceBars = sc.NumFillSpaceBars;
    }

    // calculate the number of visible bars on the chart at the moment
    int NumBarsVisible = sc.IndexOfLastVisibleBar - sc.IndexOfFirstVisibleBar + 1;

    // init if not yet set for initial state
    if (!PrevChartBarSpacing) {
        PrevChartBarSpacing = (NumBarsVisible * ChartBarSpacing) / NumBarsWhenZoomed;
    }

    // listen for key presses in the chart
    // https://www.w3schools.com/charsets/ref_html_ascii.asp
    // example: space bar is ascii code 32;
    int CharCode = sc.CharacterEventCode;

    // key press matches what study is configured for
    if (CharCode == KeyCodeForZoom) {

        // if setting for num bars to display when zoomed in was updated, update our calculations
        if (NumBarsWhenZoomed != PrevNumBarsWhenZoomed) {
            PrevChartBarSpacing = (NumBarsVisible * ChartBarSpacing) / NumBarsWhenZoomed;
            PrevNumBarsWhenZoomed = NumBarsWhenZoomed;
        }

        // toggle the zoom
        sc.ChartBarSpacing = PrevChartBarSpacing;

        // update our prev bar spacing for next time
        PrevChartBarSpacing = ChartBarSpacing;

        // toggle fill space
        if (sc.PreserveFillSpace == 1 && sc.NumFillSpaceBars > 0) {
            NumFillSpaceBars = sc.NumFillSpaceBars;
            sc.PreserveFillSpace = 0;
            sc.NumFillSpaceBars = 0;
        }
        else if (sc.PreserveFillSpace == 0 && NumFillSpaceBars > 0) {
            sc.NumFillSpaceBars = NumFillSpaceBars;
            sc.PreserveFillSpace = 1;
        }
    }
}

