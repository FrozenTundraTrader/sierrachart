#include "sierrachart.h"
SCDLLName("Frozen Tundra - Number of Highs/Lows")

/*
    Written by Frozen Tundra
*/

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc);

SCSFExport scsf_NumHighsLows(SCStudyInterfaceRef sc)
{
    // logging object
    SCString msg;

    // subgraphs
    SCSubgraphRef s_NumHighs = sc.Subgraph[0];
    SCSubgraphRef s_NumLows = sc.Subgraph[1];

    // inputs
    SCInputRef i_VerticalOffset = sc.Input[0];
    SCInputRef i_HorizontalOffset = sc.Input[1];
    SCInputRef i_FontSize = sc.Input[2];
    SCInputRef i_SessionStartTime = sc.Input[3];
    SCInputRef i_SessionEndTime = sc.Input[4];
    SCInputRef i_NoonIdxOffset = sc.Input[5];

    // Set configuration variables
    if (sc.SetDefaults)
    {
        sc.GraphName = "Number of Highs/Lows";
        sc.GraphRegion = 1;

        s_NumHighs.Name = "Number of Highs";
        s_NumHighs.PrimaryColor = COLOR_YELLOW;
        s_NumHighs.LineStyle = LINESTYLE_SOLID;

        s_NumLows.Name = "Number of Lows";
        s_NumLows.PrimaryColor = COLOR_RED;
        s_NumLows.LineStyle = LINESTYLE_SOLID;

        i_VerticalOffset.Name = "Vertical Offset in px";
        i_VerticalOffset.SetInt(20);

        i_HorizontalOffset.Name = "Horizontal Offset in px";
        i_HorizontalOffset.SetInt(0);

        i_FontSize.Name = "Font Size";
        i_FontSize.SetInt(35);

        i_SessionStartTime.Name = "Custom Start Time";
        i_SessionStartTime.SetTimeAsSCDateTime(sc.StartTime1);

        i_SessionEndTime.Name = "Custom End Time";
        i_SessionEndTime.SetTimeAsSCDateTime(sc.EndTime1);

        // revisit this, and the units?
        i_NoonIdxOffset.Name = "Time as hour for text display (ex: 12 == display at noon)";
        i_NoonIdxOffset.SetTime(HMS_TIME(12,0,0));

        return;
    }

    // draw
    sc.p_GDIFunction = DrawToChart;
}


void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{
    // subgraphs
    SCSubgraphRef s_NumHighs = sc.Subgraph[0];
    SCSubgraphRef s_NumLows = sc.Subgraph[1];

    // grab inputs
    int VerticalOffset = sc.Input[0].GetInt();
    int HorizontalOffset = sc.Input[1].GetInt();
    SCDateTime SessionStart = sc.Input[3].GetDateTime();
    SCDateTime SessionEnd = sc.Input[4].GetDateTime();
    SCDateTime NoonIdxDateTime = sc.Input[5].GetDateTime();

    int SessionStartHour = SessionStart.GetHour();
    int SessionStartMinute = SessionStart.GetMinute();
    int SessionEndHour = SessionEnd.GetHour();
    int SessionEndMinute = SessionEnd.GetMinute();
    int NoonIdxHour = NoonIdxDateTime.GetHour();
    int NoonIdxMinute = NoonIdxDateTime.GetMinute();

    int topX;
    int topY;
    int bottomX;
    int bottomY;
    SCString msg, log;

    // grab the name of the font used in this chartbook
    int fontSize = sc.Input[2].GetInt();
    SCString chartFont = sc.ChartTextFont();

    // Windows GDI font creation
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta
    HFONT hFont;
    hFont = CreateFont(fontSize,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, DEFAULT_PITCH,TEXT(chartFont));

    // https://docs.microsoft.com/en-us/windows/win32/gdi/colorref
    const COLORREF wht = COLOR_WHITE;
    const COLORREF blk = COLOR_BLACK;
    const COLORREF NewHighsColor = sc.Subgraph[0].PrimaryColor;
    const COLORREF NewLowsColor = sc.Subgraph[1].PrimaryColor;
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-settextcolor
    ::SetTextColor(DeviceContext, wht);
    ::SetBkColor(DeviceContext, blk);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    SetBkMode(DeviceContext, OPAQUE);
    SelectObject(DeviceContext, hFont);

    // grab the memory address of f0
    float &PrevHod = sc.GetPersistentFloat(0);
    int &PrevHodIdx = sc.GetPersistentInt(3);
    int &NumHighs = sc.GetPersistentInt(0);
    float &PrevLod = sc.GetPersistentFloat(1);
    int &PrevLodIdx = sc.GetPersistentInt(4);
    int &NumLows = sc.GetPersistentInt(1);
    int &NoonIdx = sc.GetPersistentInt(2);

    for (int i=0; i<sc.ArraySize; i++) {
        // RESET at new day

        // grab the date and time for the bar thats being processed
        SCDateTime BarDateTime = sc.BaseDateTimeIn[i];
        int Day = BarDateTime.GetDay();
        SCDateTime PrevBarDateTime = sc.BaseDateTimeIn[i-1];
        int PrevBarDay = PrevBarDateTime.GetDay();
        int Hour = BarDateTime.GetHour();
        int Minute = BarDateTime.GetMinute();

        // if its 930am then reset all our counters
        if (Hour < SessionStartHour || Day != PrevBarDay || (Hour == SessionStartHour && Minute < SessionStartMinute)) {
            // reset
            PrevHod = 0;
            PrevHodIdx = 0;
            NumHighs = 0;
            PrevLod = 0;
            PrevLodIdx = 0;
            NumLows = 0;
            NoonIdx = 0;
            continue;
        }

        // check if curr bar's high is > prev hod
        if (sc.High[i] > PrevHod) {
            // set a new hod
            PrevHod = sc.High[i];
            PrevHodIdx = i;
            // increment num of highs we've had today
            NumHighs++;
        }
        // check if curr bar's low is < prev lod
        if (sc.Low[i] < PrevLod || PrevLod == 0) {
            // set new lod
            PrevLod = sc.Low[i];
            PrevLodIdx = i;
            // increment num of low of days we have today
            NumLows++;
        }

        // plot Num highs onto subgraph
        s_NumHighs[i] = NumHighs;
        // plot Num lows onto subgraph
        s_NumLows[i] = NumLows;

        // NoonIdx the bar index of noon, I used this as a way to make the numbers appear in a centered place, consistently
        if (Hour < NoonIdxHour) {
            NoonIdx = sc.IndexOfLastVisibleBar;
        }
        else if (Hour == NoonIdxHour && NoonIdxMinute == 0) {
            NoonIdx = i;
        }
        if ((Hour == SessionEndHour && Minute == SessionEndMinute) || i == sc.ArraySize - 1) {
            topX = sc.BarIndexToXPixelCoordinate(NoonIdx) + HorizontalOffset;
            // main chart graph
            topY = sc.RegionValueToYPixelCoordinate(PrevHod, 0);
            bottomX = topX;
            bottomY = sc.RegionValueToYPixelCoordinate(PrevLod, 0);

            msg.Format("%d", NumHighs);
            ::SetTextColor(DeviceContext, NewHighsColor);
            ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
            // had to do some fudging of the offsets here to make things look right to human eye
            ::TextOut(DeviceContext, topX, topY - (2*VerticalOffset), msg, msg.GetLength());

            msg.Format("%d", NumLows);
            ::SetTextColor(DeviceContext, NewLowsColor);
            ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
            // had to do some fudging of the offsets here to make things look right to human eye
            ::TextOut(DeviceContext, topX, bottomY - (VerticalOffset/2), msg, msg.GetLength());

            // draw line
            s_UseTool Tool;

            Tool.ChartNumber = sc.ChartNumber;
            Tool.LineNumber = 52320220;
            Tool.DrawingType = DRAWING_LINE;
            Tool.LineStyle = LINESTYLE_DOT;
            Tool.BeginValue = PrevHod;
            Tool.BeginIndex = PrevHodIdx;
            Tool.EndValue = PrevHod + (sc.TickSize * VerticalOffset);
            Tool.EndIndex = NoonIdx;
            Tool.AddMethod = UTAM_ADD_OR_ADJUST;
            Tool.LineWidth = 1;
            Tool.Region = 0;
            Tool.Color = sc.Subgraph[0].PrimaryColor;
            sc.UseTool(Tool);

            Tool.Clear();

            Tool.ChartNumber = sc.ChartNumber;
            Tool.LineNumber = 52320221;
            Tool.DrawingType = DRAWING_LINE;
            Tool.LineStyle = LINESTYLE_DOT;
            Tool.BeginValue = PrevLod;
            Tool.BeginIndex = PrevLodIdx;
            Tool.EndValue = PrevLod - (sc.TickSize * VerticalOffset);
            Tool.EndIndex = NoonIdx;
            Tool.AddMethod = UTAM_ADD_OR_ADJUST;
            Tool.LineWidth = 1;
            Tool.Region = 0;
            Tool.Color = sc.Subgraph[1].PrimaryColor;
            sc.UseTool(Tool);
        }
    }

    // delete font
    DeleteObject(hFont);

    return;
}
