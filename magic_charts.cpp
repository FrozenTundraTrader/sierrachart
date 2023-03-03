#include "sierrachart.h"
SCDLLName("Frozen Tundra - Magic Executions")
using std::unordered_map;
using std::vector;

/*
    Written by Frozen Tundra
*/

// function prototype declaration
void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc);

SCSFExport scsf_Magic(SCStudyInterfaceRef sc)
{
    // logging object
    SCString msg;

    int InputIdx = -1;
    SCInputRef i_UpdateIntervalMs = sc.Input[++InputIdx];
    SCInputRef i_MaxTicksPerBar = sc.Input[++InputIdx];
    SCInputRef i_SizeDownThreshold = sc.Input[++InputIdx];
    SCInputRef i_SkipRate = sc.Input[++InputIdx];
    SCInputRef i_FontSize = sc.Input[++InputIdx];
    SCInputRef i_yOffset = sc.Input[++InputIdx];
    SCInputRef i_MagicWidthPerc = sc.Input[++InputIdx];
    SCInputRef i_LargeExecutionThreshold = sc.Input[++InputIdx];
    SCInputRef i_RegularExecutionStr = sc.Input[++InputIdx];
    SCInputRef i_LargeExecutionStr = sc.Input[++InputIdx];
    SCInputRef i_BarsBeforeTurningOff = sc.Input[++InputIdx];
    SCInputRef i_BidColor = sc.Input[++InputIdx];
    SCInputRef i_AskColor = sc.Input[++InputIdx];
    SCInputRef i_EnablePositionDebug = sc.Input[++InputIdx];

    // Set configuration variables
    if (sc.SetDefaults)
    {
        sc.GraphName = "Magic";
        sc.GraphShortName = "Magic";
        sc.GraphRegion = 0;

        i_UpdateIntervalMs.Name = "Update Interval in Milliseconds";
        i_UpdateIntervalMs.SetInt(250);

        i_MaxTicksPerBar.Name = "Max ticks drawn per bar (0=off)";
        i_MaxTicksPerBar.SetInt(3000);

        i_SizeDownThreshold.Name = "Start Skipping Records After This #";
        i_SizeDownThreshold.SetInt(10);

        i_SkipRate.Name = "% Skip Rate";
        i_SkipRate.SetFloat(0.01);

        i_FontSize.Name = "Font Size";
        i_FontSize.SetInt(22);

        i_yOffset.Name = "Y-axis offset";
        i_yOffset.SetInt(-10);

        i_MagicWidthPerc.Name = "Magic Width % (0.90 tight, 1.50 fills gap)";
        i_MagicWidthPerc.SetFloat(1.50);

        i_LargeExecutionThreshold.Name = "Large Execution Threshold";
        i_LargeExecutionThreshold.SetInt(5000);

        i_RegularExecutionStr.Name = "Character to print for each tick, i.e. '.'";
        i_RegularExecutionStr.SetString(".");

        i_LargeExecutionStr.Name = "Character to print for large executions, i.e. 'O'";
        i_LargeExecutionStr.SetString("O");

        i_BarsBeforeTurningOff.Name = "Stop displaying executions after this many bars are on screen.";
        i_BarsBeforeTurningOff.SetInt(11);

        i_BidColor.Name = "Bid Color";
        i_BidColor.SetColor(COLOR_RED);

        i_AskColor.Name = "Ask Color";
        i_AskColor.SetColor(COLOR_BLUE);

        i_EnablePositionDebug.Name = "Enable Position Debug on Bars";
        i_EnablePositionDebug.SetYesNo(0);

        //this must be set to 1 in order to use the sc.ReadIntradayFileRecordForBarIndexAndSubIndex function. 
        // https://dtcprotocol.org/SupportBoard.php?PostID=130661#P130661
        sc.MaintainAdditionalChartDataArrays = 1;
        return;
    }

    // delay interval logic
    int &LastUpdated = sc.GetPersistentInt(0);
    int Interval = i_UpdateIntervalMs.GetInt();
    SCDateTime Now = sc.CurrentSystemDateTime;
    int NowInMilliseconds = Now.GetTimeInMilliseconds();
    bool UpdateTicks = false;
    // if there's been a 6+ hour time difference, like start of next day, reset LastUpdated
    int ResetTime = 1000 * 6 * 60;
    if (&LastUpdated == NULL)
    {
        LastUpdated = NowInMilliseconds;
    }
    if (NowInMilliseconds > LastUpdated + Interval)
    {
        UpdateTicks = true;
        LastUpdated = NowInMilliseconds;
    }
    else if (NowInMilliseconds < LastUpdated - ResetTime)
    {
        LastUpdated = NowInMilliseconds;
    }
    else
    {
        return;
    }

    // persistent structure holding tick data for each bar
    unordered_map<int, vector<s_IntradayRecord>> *p_TicksPerBar = (unordered_map<int, vector<s_IntradayRecord>>*)sc.GetPersistentPointer(0);
    if (p_TicksPerBar == NULL)
    {
        p_TicksPerBar = new unordered_map<int, vector<s_IntradayRecord>>;
        sc.SetPersistentPointer(0, p_TicksPerBar);
    }

    if (sc.Index == 0 || UpdateTicks)
    {

        //msg.Format("Update=%d, MaxTicks=%d, FontSize=%d", Interval, i_MaxTicksPerBar.GetInt(), i_FontSize.GetInt());
        //sc.AddMessageToLog(msg, 0);

        // reset
        p_TicksPerBar->clear();

        const int MAX_SIZE = i_MaxTicksPerBar.GetInt();
        const int SIZE_DOWN_SAMPLING_THRESHOLD = i_SizeDownThreshold.GetInt();
        const float SKIP_RATE = i_SkipRate.GetFloat();

        // visible bars only
        for (int CurrIdx=sc.IndexOfFirstVisibleBar; CurrIdx<=sc.IndexOfLastVisibleBar; CurrIdx++)
        {

            // Intraday Record File reading
            int ReadSuccess = true;
            bool FirstIteration = true;
            s_IntradayRecord IntradayRecord;
            int SubIndex = 0;//Start at first record within bar

            //Read records until sc.ReadIntradayFileRecordForBarIndexAndSubIndex returns 0
            while (ReadSuccess)
            {

                // default status for our file lock is to do nothing
                IntradayFileLockActionEnum  IntradayFileLockAction = IFLA_NO_CHANGE;
                if (FirstIteration)
                {
                    // on first iteration, place lock on intraday file
                    IntradayFileLockAction = IFLA_LOCK_READ_HOLD;
                    // flip first iteration semaphore
                    FirstIteration = false;
                }

                // read intraday records at these indicies
                ReadSuccess = sc.ReadIntradayFileRecordForBarIndexAndSubIndex(CurrIdx, SubIndex, IntradayRecord, IntradayFileLockAction);

                if (ReadSuccess)
                {
                    if (IntradayRecord.IsSingleTradeWithBidAsk())
                    {
                        // fetch properties of this trade
                        int Volume = IntradayRecord.TotalVolume;
                        float Price = IntradayRecord.GetClose();
                        // if its within the bounds of the bar's prices (no late prints)
                        if (Price <= sc.High[CurrIdx] && Price >= sc.Low[CurrIdx])
                        {
                            int Year = IntradayRecord.DateTime.GetYear();
                            int Month = IntradayRecord.DateTime.GetMonth();
                            int Day = IntradayRecord.DateTime.GetDay();
                            int Hour = IntradayRecord.DateTime.GetHour();
                            int Minute = IntradayRecord.DateTime.GetMinute();
                            int Second = IntradayRecord.DateTime.GetSecond();
                            // store into persisting struct
                            (*p_TicksPerBar)[CurrIdx].push_back(IntradayRecord);
                            //msg.Format("Storing @ Index %d: [%d/%d][%d:%d:%d] %d @ %f", CurrIdx, SubIndex, p_TicksPerBar[CurrIdx].size(), Hour, Minute, Second, Volume, Price);
                            //sc.AddMessageToLog(msg, 0);
                        }
                    }
                }
                ++SubIndex;

                // down sampling logic for performance optimization
                // start skipping records if we hit our threshold
                if ((*p_TicksPerBar)[CurrIdx].size() > SIZE_DOWN_SAMPLING_THRESHOLD)
                {
                    SubIndex += max(SKIP_RATE * (*p_TicksPerBar)[CurrIdx].size(), 1);
                }

                // stop storing records after this point
                if (MAX_SIZE > 0 && (*p_TicksPerBar)[CurrIdx].size() > MAX_SIZE)
                {
                    //msg.Format("Bailing out due to max size hit");
                    //sc.AddMessageToLog(msg, 0);
                    break;
                }
            } // end of intraday file reading loop

            // done reading, release lock
            sc.ReadIntradayFileRecordForBarIndexAndSubIndex(-1, -1, IntradayRecord, IFLA_RELEASE_AFTER_READ);

            // reverse sort this index - needed for drawing (TODO)
            std::reverse((*p_TicksPerBar)[CurrIdx].begin(), (*p_TicksPerBar)[CurrIdx].end());
        }
    }

    // draw
    sc.p_GDIFunction = DrawToChart;
}

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{
    SCString msg, log;

    int FirstIdx = sc.IndexOfFirstVisibleBar;
    int LastIdx = sc.IndexOfLastVisibleBar;

    // find our struct we built from reading intraday file
    unordered_map<int, vector<s_IntradayRecord>> *p_TicksPerBar = (unordered_map<int, vector<s_IntradayRecord>>*)sc.GetPersistentPointer(0);
    if (p_TicksPerBar == NULL)
    {
        p_TicksPerBar = new unordered_map<int, vector<s_IntradayRecord>>;
        sc.SetPersistentPointer(0, p_TicksPerBar);
        return;
    }

    // GRAB INPUTS
    int FontSize                 = sc.Input[4].GetInt();
    int yOffset                  = sc.Input[5].GetInt();
    float MagicWidthPerc         = sc.Input[6].GetFloat();
    int LargeExecThreshold       = sc.Input[7].GetInt();
    SCString RegularExecutionStr = sc.Input[8].GetString();
    SCString LargeExecutionStr   = sc.Input[9].GetString();
    int BarsBeforeTurningOff     = sc.Input[10].GetColor();
    COLORREF BidColor       = sc.Input[11].GetColor();
    COLORREF AskColor       = sc.Input[12].GetColor();
    bool EnablePositioningDebug  = sc.Input[13].GetYesNo();

    // safeguard, dont try to render if there's a lot of bars visible
    if (sc.IndexOfLastVisibleBar - sc.IndexOfFirstVisibleBar > BarsBeforeTurningOff) {
        //msg.Format("Exiting, too many bars");
        //sc.AddMessageToLog(msg, 0);
        return;
    }

    // grab the name of the font used in this chartbook
    SCString chartFont = sc.ChartTextFont();

    float vHigh, vLow, vDiff;
    sc.GetMainGraphVisibleHighAndLow(vHigh, vLow);
    vDiff = (vHigh - vLow);
    int NumberOfLevels = vDiff / sc.TickSize;

    // num bars displayed currently
    int NumBarsDisplayed = sc.IndexOfLastVisibleBar - sc.IndexOfFirstVisibleBar;
    int xFirstVisibleBar = sc.BarIndexToXPixelCoordinate(sc.IndexOfFirstVisibleBar);
    int xLastVisibleBar = sc.BarIndexToXPixelCoordinate(sc.IndexOfLastVisibleBar);
    int xScreenWidth = xLastVisibleBar - xFirstVisibleBar;
    int y;
    y = sc.RegionValueToYPixelCoordinate(vLow + (vDiff/2), sc.GraphRegion);

    // Windows GDI font creation
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta
    HFONT hFont;
    hFont = CreateFont(FontSize,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, DEFAULT_PITCH,TEXT(chartFont));

    // https://docs.microsoft.com/en-us/windows/win32/gdi/colorref
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-settextcolor
    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    SetBkMode(DeviceContext, TRANSPARENT);
    SelectObject(DeviceContext, hFont);
    ::SetTextAlign(DeviceContext, TA_NOUPDATECP);

    for (int CurrIdx=FirstIdx; CurrIdx<=LastIdx; CurrIdx++)
    {
        // dont try to paint anything if no trades
        if ((*p_TicksPerBar)[CurrIdx].size() > 0)
        {

            // start and end datetimes for this bar
            SCDateTime BarStart = (*p_TicksPerBar)[CurrIdx].at(0).DateTime;
            SCDateTime BarEnd = (*p_TicksPerBar)[CurrIdx].at((*p_TicksPerBar)[CurrIdx].size()-1).DateTime;

            // x coord position
            int xBarStart = sc.BarIndexToXPixelCoordinate(CurrIdx) - 2;

            // store a copy, we'll need it later
            int xOrigStart = xBarStart;

            // next bar's start
            int xNextBarStart = sc.BarIndexToXPixelCoordinate(CurrIdx+1)-1;

            // adjustable bar width
            //float MagicBarWidth = 0.90; // very defined
            //float MagicBarWidth = 1.50;  // together seamlessly
            int xBarWidth = (xNextBarStart - xBarStart) * MagicWidthPerc;

            // adjust bar start based on dynamic width
            xBarStart -= xBarWidth/3;
            int xBarEnd = xBarStart + xBarWidth - (xBarWidth/3);

            // find the diff
            int xDiff = xBarEnd - xBarStart;

            // loop through ticks that executed during this bar
            int TicksForBarIdx = (*p_TicksPerBar)[CurrIdx].size();
            for (int i=0; i<TicksForBarIdx; i++)
            {
                // grab next tick
                s_IntradayRecord IntradayRecord = (*p_TicksPerBar)[CurrIdx].at(i);

                // extract relevant data from tick record
                int Volume = IntradayRecord.TotalVolume;
                int BidVolume = IntradayRecord.BidVolume;
                int AskVolume = IntradayRecord.AskVolume;

                // set colors as needed
                log.Format("%s", RegularExecutionStr.GetChars());
                if (BidVolume > AskVolume)
                {
                    ::SetTextColor(DeviceContext, BidColor);
                }
                else if (AskVolume > BidVolume)
                {
                    ::SetTextColor(DeviceContext, AskColor);
                }

                // large size special case
                if (Volume >= LargeExecThreshold)
                {
                    log.Format("%s", LargeExecutionStr.GetChars());
                }

                // extra large size special case
                if (Volume >= 3 * LargeExecThreshold)
                {
                    // TODO this is equities specific right now
                    log.Format("%s %d", LargeExecutionStr.GetChars(), Volume/1000);
                }

                // draw the tick
                if (IntradayRecord.GetClose() <= sc.High[CurrIdx] && IntradayRecord.GetClose() >= sc.Low[CurrIdx])
                {
                    float OffsetPercInMs = ((float)i / (float)TicksForBarIdx);
                    int xTarget = xBarStart - (xDiff * OffsetPercInMs) + xDiff;
                    y = sc.RegionValueToYPixelCoordinate(IntradayRecord.GetClose(), sc.GraphRegion);
                    y += yOffset;
                    ::TextOut(DeviceContext, xTarget, y, log, log.GetLength());
                }
            }

            // enable positioning debug
            if (EnablePositioningDebug)
            {
                // render center pt of candle
                log.Format("c%d", CurrIdx);
                ::TextOut(DeviceContext, xOrigStart, y, log, log.GetLength());
                // render start pt of candle
                log.Format("s%d", CurrIdx);
                ::TextOut(DeviceContext, xBarStart, y, log, log.GetLength());
                // render end pt of candle
                log.Format("e%d", CurrIdx);
                ::TextOut(DeviceContext, xBarEnd, y, log, log.GetLength());
            }
        }

        // stats
        if (sc.Index == 0)
        {
            int TmpCurrSize = (*p_TicksPerBar)[CurrIdx].size();
            msg.Format("[%d] Size=%d", CurrIdx, TmpCurrSize);
            sc.AddMessageToLog(msg, 0);
        }
    }
    // delete font
    DeleteObject(hFont);
}
