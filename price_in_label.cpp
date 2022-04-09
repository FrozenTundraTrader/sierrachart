#include "sierrachart.h"
SCDLLName("Frozen Tundra - DOM Price In Label Column")

/*
    Written by Frozen Tundra & Guitarmadillo

*/

// WinGDI draw function definition
void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc);

SCSFExport scsf_PriceInLabel(SCStudyInterfaceRef sc)
{
    // logging object
    SCString msg;

    // Inputs
    SCInputRef i_DomFontSize = sc.Input[0];
    SCInputRef i_BoldFont = sc.Input[1];
    SCInputRef i_LabelFgColor = sc.Input[2];
    SCInputRef i_LabelBgColor = sc.Input[3];
    SCInputRef i_xOffset = sc.Input[4];
    SCInputRef i_yOffset = sc.Input[5];
    SCInputRef i_NumDigitsToDisplay = sc.Input[6];
    SCInputRef i_LastTradeFg = sc.Input[7];
    SCInputRef i_LastTradeBg = sc.Input[8];
    SCInputRef i_TargetColumn = sc.Input[9];

    // Set configuration variables
    if (sc.SetDefaults)
    {
        sc.GraphName = "DOM Price In Label Column";
        sc.GraphRegion = 0;

        i_DomFontSize.Name = "DOM Font Size";
        i_DomFontSize.SetInt(18);

        i_LabelFgColor.Name = "Text Color";
        i_LabelFgColor.SetColor(0,0,0);

        i_LabelBgColor.Name = "Background Color";
        i_LabelBgColor.SetColor(255,255,255);

        i_xOffset.Name = "DOM Label X-coord Offset";
        i_xOffset.SetInt(10);

        i_yOffset.Name = "DOM Label Y-coord Offset";
        i_yOffset.SetInt(-10);

        i_NumDigitsToDisplay.Name = "Number of digits to display (from right hand side)";
        i_NumDigitsToDisplay.SetInt(3);

        i_BoldFont.Name = "Use Bold Font?";
        i_BoldFont.SetYesNo(0);

        i_LastTradeFg.Name = "Last Trade Foreground Color";
        i_LastTradeFg.SetColor(0,0,0);

        i_LastTradeBg.Name = "Last Trade Background Color";
        i_LastTradeBg.SetColor(0,255,0);

        i_TargetColumn.Name = "Which DOM Column To Use For Price?";
        i_TargetColumn.SetCustomInputStrings("Label;General Purpose 1;General Purpose 2;");
        i_TargetColumn.SetCustomInputIndex(0);

        return;
    }

    // print out the prices
    sc.p_GDIFunction = DrawToChart;
}

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{

    SCString log;

    // grab input values
    int fontSize = sc.Input[0].GetInt();
    int UseBoldFont = sc.Input[1].GetInt();
    COLORREF fg = sc.Input[2].GetColor();
    COLORREF bg = sc.Input[3].GetColor();
    int xOffset = sc.Input[4].GetInt();
    int yOffset = sc.Input[5].GetInt();
    int NumDigitsToDisplay = sc.Input[6].GetInt();
    COLORREF LastTradeFg = sc.Input[7].GetColor();
    COLORREF LastTradeBg = sc.Input[8].GetColor();
    int SelectedTargetColumn = sc.Input[9].GetIndex();

    // default to label col, otherwise select chosen target col for price
    int x = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_SUBGRAPH_LABELS);
    if (SelectedTargetColumn == 0) x = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_SUBGRAPH_LABELS);
    else if (SelectedTargetColumn == 1) x = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_1);
    else if (SelectedTargetColumn == 2) x = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_2);

    // grab x pixel coords
    x += xOffset;
    int y;
    SCString msg;

    float plotPrice = 0;

    // grab the name of the font used in this chartbook
    SCString chartFont = sc.ChartTextFont();

    // Windows GDI font creation
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta
    HFONT hFont;
    int FontBoldType = FW_NORMAL;
    if (UseBoldFont == 1) FontBoldType = FW_BOLD;
    hFont = CreateFont(fontSize,0,0,0,FontBoldType,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, DEFAULT_PITCH,TEXT(chartFont));

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    //SetBkMode(DeviceContext, OPAQUE);
    SelectObject(DeviceContext, hFont);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    //SetBkMode(DeviceContext, TRANSPARENT);

    // fetch the graph's price scale's high and low value
    float vHigh, vLow, vDiff;
    sc.GetMainGraphVisibleHighAndLow(vHigh, vLow);

    // calc how many price levels are currently displayed so we only need to print them and not more
    vDiff = (vHigh - vLow);
    int NumLevels = vDiff / sc.TickSize;

    // make the starting price formatted correctly
    vLow = sc.RoundToTickSize(vLow, sc.TickSize);

    for (int i=0; i<NumLevels; i++) {

        // calc next price we need to print
        plotPrice = vLow + (i * sc.TickSize);
        // format price to look pretty
        msg = sc.FormatGraphValue(plotPrice, sc.BaseGraphValueFormat);

        // * * * * * * *
        // SPECIAL CASES
        // The following code blocks are for special case symbols like 
        // bonds that use fractional formats and have edge cases
        // in their formatting.
        // More info here:
        // https://www.sierrachart.com/index.php?page=doc/ACSIL_Members_Variables_And_Arrays.html#scValueFormat
        // * * * * * * *

        // ZF
        // ZN
        // Quarter Thirty-seconds. Example output: .25/32
        if (sc.BaseGraphValueFormat == 136
                || sc.BaseGraphValueFormat == 134
        ) {
            if (msg.IndexOf('.') < 0) {
                // we have a whole number and need to add two zeros
                msg.Append(".00");
            }
            else if (msg.IndexOf('.') == msg.GetLength() - 2) {
                // we have a .5 situation and need to add one zero
                msg.Append("0");
            }
        }
        // ZT
        // Eighth Thirty-seconds. Example output: .125/32
        else if (sc.BaseGraphValueFormat == 140) {
            if (msg.IndexOf('.') < 0) {
                // we have a whole number and need to add two zeros
                msg.Append(".00");
            }
            else if (msg.IndexOf('.') == msg.GetLength() - 2) {
                // we have a .5 situation and need to add one zero
                msg.Append("0");
            }
            else if (msg.IndexOf('.') == msg.GetLength() - 4) {
                // we have a .5 situation and need to add one zero
                msg = msg.GetSubString(msg.GetLength() - 1, 0);
            }
        }

        // trim to appropriate length as desired by input settings
        msg = msg.Right(NumDigitsToDisplay + 1);

        // calc y pixel position
        y = sc.RegionValueToYPixelCoordinate(plotPrice, sc.GraphRegion);
        y += yOffset;

        // last trade colors
        if (plotPrice == sc.LastTradePrice) {
            ::SetTextColor(DeviceContext, LastTradeFg);
            ::SetBkColor(DeviceContext, LastTradeBg);
        }
        else {
            // https://docs.microsoft.com/en-us/windows/win32/gdi/colorref
            // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-settextcolor
            ::SetTextColor(DeviceContext, fg);
            ::SetBkColor(DeviceContext, bg);
        }

        // print it
        ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
        ::TextOut(DeviceContext, x, y, msg, msg.GetLength());

        // delete font
        DeleteObject(hFont);
    }

    return;
}
