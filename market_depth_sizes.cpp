#include "sierrachart.h"

SCDLLName("Frozen Tundra - Market Depth Sizes")

/*
    Written by Frozen Tundra
*/
void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc); 

SCSFExport scsf_MarketDepthSizes(SCStudyInterfaceRef sc)
{
    // number of depth levels to calculate avg lots for
    SCInputRef NumberOfLevels = sc.Input[0];

    // minimum size of bid/offers to render, ex: 5K shares would be "5000"
    SCInputRef MinimumSize = sc.Input[1];

    // font size to render avg lots 
    SCInputRef FontSize = sc.Input[2];

    // spacing padding to align numbers to DOM prices
    SCInputRef VerticalOffset = sc.Input[3];

    // spacing padding to align numbers x axis
    SCInputRef HorizontalOffset = sc.Input[4];

    // logging object
    SCString log_message;

    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;
        sc.GraphShortName = "MarketDepthSizes";
        sc.UsesMarketDepthData = 1;
        NumberOfLevels.Name = "Number of Market Depth Levels";
        NumberOfLevels.SetInt(100);
        MinimumSize.Name = "Minimum Size Filter in Actual Shares";
        MinimumSize.SetInt(5000);
        FontSize.Name = "Font Size";
        FontSize.SetInt(30);
        VerticalOffset.Name = "Vertical Offset in Pixels";
        VerticalOffset.SetInt(20);
        HorizontalOffset.Name = "Horizontal Offset in Pixels";
        HorizontalOffset.SetInt(20);
        return;
    }

    // we need these data to persist to our windows GDI call
    int num_levels = NumberOfLevels.GetInt();
    sc.SetPersistentInt(0, num_levels);

    // draw
    sc.p_GDIFunction = DrawToChart;

}

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{
    int MinimumSize = sc.Input[1].GetInt();
    int VerticalOffset = sc.Input[3].GetInt();
    int HorizontalOffset = sc.Input[4].GetInt();
    int num_levels = sc.GetPersistentInt(0);
    int bidX = sc.BarIndexToXPixelCoordinate(sc.Index) + HorizontalOffset;
    int bidY;
    int askX = bidX;
    int askY;
    SCString msg;

    float plotPrice = 0;
    //msg.Format("tickSize=%f, lastPrice=%f", tickSize, lastPrice);
    //sc.AddMessageToLog(msg, 1);

    // grab the name of the font used in this chartbook
    int fontSize = sc.Input[2].GetInt();
    SCString chartFont = sc.ChartTextFont();

    // Windows GDI font creation
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta
    HFONT hFont;
    hFont = CreateFont(fontSize,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, DEFAULT_PITCH,TEXT(chartFont));

    // https://docs.microsoft.com/en-us/windows/win32/gdi/colorref
    const COLORREF wht = 0x00FFFFFF;
    const COLORREF blk = 0x00000000;
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-settextcolor
    ::SetTextColor(DeviceContext, wht);
    ::SetBkColor(DeviceContext, blk);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    SetBkMode(DeviceContext, OPAQUE);

    SelectObject(DeviceContext, hFont);
    s_MarketDepthEntry bid_mde;
    s_MarketDepthEntry ask_mde;        // used for calculating avg lot sizes
    for (int i=0; i<num_levels; i++) {

        sc.GetBidMarketDepthEntryAtLevel(bid_mde, i);
        sc.GetAskMarketDepthEntryAtLevel(ask_mde, i);

        // quantity, price, numorders
        //log_message.Format("i=%d, BxA=%f x %f, Count BxA=%d x %d", i, (float)bid_mde.Quantity, (float)ask_mde.Quantity, bid_mde.NumOrders, ask_mde.NumOrders);
        //sc.AddMessageToLog(log_message, 1);

        // calculate coords
        bidY = sc.RegionValueToYPixelCoordinate(bid_mde.Price, sc.GraphRegion);
        askY = sc.RegionValueToYPixelCoordinate(ask_mde.Price, sc.GraphRegion);

        // print bid side text on DOM
        msg.Format("%.0f", bid_mde.Quantity/1000);
        if (bid_mde.Quantity >= MinimumSize) {
            ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
            ::TextOut(DeviceContext, bidX, bidY - VerticalOffset, msg, msg.GetLength());
        }

        // print ask side text to DOM
        msg.Format("%.0f", ask_mde.Quantity/1000);
        if (ask_mde.Quantity >= MinimumSize) {
            ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
            ::TextOut(DeviceContext, askX, askY - VerticalOffset, msg, msg.GetLength());
        }

        // delete font
        DeleteObject(hFont);
    }

    return;
}
