#include "sierrachart.h"

SCDLLName("Frozen Tundra - Guitarmadillo")

/*
    Written by Malykubo and Frozen Tundra in FatCat's Discord Room
    This custom study requires Market Depth data.
    Computes the average number of lots advertised per order
    and displays them in General Purpose Column 1 of the DOM.
*/
void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc); 

SCSFExport scsf_AverageLotSize(SCStudyInterfaceRef sc)
{
    // number of depth levels to calculate avg lots for
    SCInputRef NumberOfLevels = sc.Input[0];

    // number of decimal points to display for the avg lots
    SCInputRef NumberOfDecimals = sc.Input[1];

    // font size to render avg lots 
    SCInputRef FontSize = sc.Input[2];

    // spacing padding to align numbers to DOM prices
    SCInputRef VerticalOffset = sc.Input[3];

    // logging object
    SCString log_message;

    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;
        sc.UsesMarketDepthData = 1;
        NumberOfLevels.Name = "Number of Levels";
        NumberOfLevels.SetInt(5);
        NumberOfDecimals.Name = "Number Of Decimals";
        NumberOfDecimals.SetInt(1);
        NumberOfDecimals.SetIntLimits(0, 2);
        FontSize.Name = "Font Size";
        FontSize.SetInt(22);
        VerticalOffset.Name = "Vertical Offset in Pixels";
        VerticalOffset.SetInt(10);
        return;
    }

    int num_levels = NumberOfLevels.GetInt();

    // SierraChart object for a market depth record
    s_MarketDepthEntry bid_mde;
    s_MarketDepthEntry ask_mde;        // used for calculating avg lot sizes
    float bid_avg_lot = 0;
    float ask_avg_lot = 0;

    // pointers to dynamically allocated arrays
    float *p_bidAvgLots;
    float *p_askAvgLots;

    // malloc memory for arrays
    p_bidAvgLots = (float *) sc.AllocateMemory( 1024 * sizeof(float) );
    p_askAvgLots = (float *) sc.AllocateMemory( 1024 * sizeof(float) );

    // grab market depth data
    for (int i=0; i<num_levels; i++) {
        sc.GetBidMarketDepthEntryAtLevel(bid_mde, i);
        sc.GetAskMarketDepthEntryAtLevel(ask_mde, i);

        // quantity, price, numorders
        //log_message.Format("i=%d, BxA=%f x %f, Count BxA=%d x %d", i, (float)bid_mde.Quantity, (float)ask_mde.Quantity, bid_mde.NumOrders, ask_mde.NumOrders);
        //sc.AddMessageToLog(log_message, 1);

        // the calculations for avg lot size
        bid_avg_lot = (float)bid_mde.Quantity / bid_mde.NumOrders;
        ask_avg_lot = (float)ask_mde.Quantity / ask_mde.NumOrders;

        // store in arrays
        p_bidAvgLots[i] = bid_avg_lot;
        p_askAvgLots[i] = ask_avg_lot;

    }

    // we need these data to persist to our windows GDI call
    sc.SetPersistentInt(0, num_levels);
    sc.SetPersistentPointer(0, p_bidAvgLots);
    sc.SetPersistentPointer(1, p_askAvgLots);
    //log_message.Format("?P?P? bidAvg=%d x askAvg=%d", *p_bidAvgLots, *p_askAvgLots);
    //sc.AddMessageToLog(log_message, 1);

    // draw
    sc.p_GDIFunction = DrawToChart;

    // free up memory
    sc.FreeMemory(p_bidAvgLots);
    sc.FreeMemory(p_askAvgLots);
}

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{
    int num_levels = sc.GetPersistentInt(0);
    int bidX = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_1);
    int bidY;
    int askX = bidX;
    int askY;
    SCString msg;

    // fetch the arrays of data
    float *p_bidAvgLots = (float *)sc.GetPersistentPointer(0);
    float *p_askAvgLots = (float *)sc.GetPersistentPointer(1);    float lastPrice = sc.Close[sc.Index];
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
    SelectObject(DeviceContext, hFont);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    SetBkMode(DeviceContext, TRANSPARENT);
    int NumberOfDecimals = sc.Input[1].GetInt();
    int padding = sc.Input[3].GetInt();
    for (int i=0; i<num_levels; i++) {

        // calculate coords
        plotPrice = sc.Bid - (i * sc.TickSize);
        bidY = sc.RegionValueToYPixelCoordinate(plotPrice, sc.GraphRegion);
        plotPrice = sc.Ask + (i * sc.TickSize);
        askY = sc.RegionValueToYPixelCoordinate(plotPrice, sc.GraphRegion);

        // formatting of number of decimal places
        switch (NumberOfDecimals) {
            case 0:
                msg.Format("%.0f", *(p_bidAvgLots + i));
                break;
            case 1:
                msg.Format("%.1f", *(p_bidAvgLots + i));
                break;
            case 2:
                msg.Format("%.2f", *(p_bidAvgLots + i));
                break;                    }

        // print bid side text on DOM
        ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
        ::TextOut(DeviceContext, bidX, bidY - padding, msg, msg.GetLength());

        // munge ask side text together
        //msg.Format("%f", *(p_askAvgLots + i));
        switch (NumberOfDecimals) {
            case 0:
                msg.Format("%.0f", *(p_askAvgLots + i));
                break;
            case 1:
                msg.Format("%.1f", *(p_askAvgLots + i));
                break;
            case 2:
                msg.Format("%.2f", *(p_askAvgLots + i));
                break;                    }

        // print ask side text to DOM
        ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
        ::TextOut(DeviceContext, askX, askY - padding, msg, msg.GetLength());

        // delete font
        DeleteObject(hFont);
    }

    return;
}
