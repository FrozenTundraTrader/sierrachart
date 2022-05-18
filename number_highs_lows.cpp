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

    int VerticalOffset = sc.Input[0].GetInt();
    int HorizontalOffset = sc.Input[1].GetInt();
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
    const COLORREF blue = COLOR_BLUE;
    const COLORREF red = COLOR_RED;
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-settextcolor
    ::SetTextColor(DeviceContext, wht);
    ::SetBkColor(DeviceContext, blk);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    SetBkMode(DeviceContext, OPAQUE);
    SelectObject(DeviceContext, hFont);

    // grab the memory address of f0
    float &PrevHod = sc.GetPersistentFloat(0);
    int &NumHighs = sc.GetPersistentInt(0);
    float &PrevLod = sc.GetPersistentFloat(1);
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
        if (Hour < 9 || Day != PrevBarDay || (Hour == 9 && Minute < 30)) {
            // reset
            PrevHod = 0;
            NumHighs = 0;
            PrevLod = 0;
            NumLows = 0;
            NoonIdx = 0;
            continue;
        }

        // check if curr bar's high is > prev hod
        if (sc.High[i] > PrevHod) {
            // set a new hod
            PrevHod = sc.High[i];
            // increment num of highs we've had today
            NumHighs++;
        }
        // check if curr bar's low is < prev lod
        if (sc.Low[i] < PrevLod || PrevLod == 0) {
            // set new lod
            PrevLod = sc.Low[i];
            // increment num of low of days we have today
            NumLows++;
        }

        // plot Num highs onto subgraph
        s_NumHighs[i] = NumHighs;
        // plot Num lows onto subgraph
        s_NumLows[i] = NumLows;

        // NoonIdx the bar index of noon, I used this as a way to make the numbers appear in a centered place, consistently
        if (Hour < 12) {
            NoonIdx = sc.IndexOfLastVisibleBar;
        }
        else if (Hour == 12 && Minute == 0) {
            NoonIdx = i;
        }
        if ((Hour == 16 && Minute == 0) || i == sc.ArraySize - 1) {
            topX = sc.BarIndexToXPixelCoordinate(NoonIdx) + HorizontalOffset;
            // main chart graph
            topY = sc.RegionValueToYPixelCoordinate(PrevHod, 0);
            bottomX = topX;
            bottomY = sc.RegionValueToYPixelCoordinate(PrevLod, 0);

            msg.Format("%d", NumHighs);
            ::SetTextColor(DeviceContext, blue);
            ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
            // had to do some fudging of the offsets here to make things look right to human eye
            ::TextOut(DeviceContext, topX, topY - (2*VerticalOffset), msg, msg.GetLength());

            msg.Format("%d", NumLows);
            ::SetTextColor(DeviceContext, red);
            ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
            // had to do some fudging of the offsets here to make things look right to human eye
            ::TextOut(DeviceContext, topX, bottomY - (VerticalOffset/2), msg, msg.GetLength());
        }
    }

    // delete font
    DeleteObject(hFont);

    return;
}
