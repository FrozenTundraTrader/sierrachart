#include "sierrachart.h"
#include <vector>
SCDLLName("Frozen Tundra - Pace of Tape")

/*
    Written by Frozen Tundra
*/


// struct to store ticks per second
struct RecordsPerUnit {
    // integer representation of the time, in seconds
    int TimeInSeconds;

    // number of records tallied for this second in time
    int NumRecords;

    int MaxRecords;
};

struct LineNumber {
    int StudyId;
    int TrailIndex;
    int SquareIndex;
    int LineNumber;
    int TimeInSeconds;
};

struct GlobalLineNumbers {
    // global to keep track of all drawings of all rectangles across all instances
    // of this study on the same chart
    int NextLineNumber = 20221205;

    std::vector<LineNumber> LineNumbers;

    int AddLineNumber(int StudyId, int TimeInSeconds, int SquareIndex) {

        // search for existing line number
        for (int i=0; i<LineNumbers.size(); i++) {
            LineNumber tmp = LineNumbers[i];
            if (tmp.StudyId == StudyId && tmp.TimeInSeconds == TimeInSeconds && tmp.SquareIndex == SquareIndex) {
                return(tmp.LineNumber);
            }
        }

        // no existing found, add a new one and keep track of it
        LineNumber tmp;
        tmp.StudyId = StudyId;
        //tmp.TrailIndex = TrailIndex;
        tmp.TimeInSeconds = TimeInSeconds;
        tmp.SquareIndex = SquareIndex;
        tmp.LineNumber = NextLineNumber;
        LineNumbers.push_back(tmp);
        NextLineNumber++;
        return(tmp.LineNumber);
    };

};

GlobalLineNumbers g_LineNumbers;

SCSFExport scsf_PaceOfTape(SCStudyInterfaceRef sc)
{
    // logging object
    SCString msg;

    // inputs
    int InputIdx = 0;
    SCInputRef i_Symbol = sc.Input[++InputIdx];
    SCInputRef i_TicksOrVolume = sc.Input[++InputIdx];
    SCInputRef i_Layout = sc.Input[++InputIdx];
    SCInputRef i_NumRecordsToExamine = sc.Input[++InputIdx];
    SCInputRef i_NumSquares = sc.Input[++InputIdx];
    SCInputRef i_SquareSize = sc.Input[++InputIdx];
    SCInputRef i_Reverse = sc.Input[++InputIdx];
    SCInputRef i_CalcMethod = sc.Input[++InputIdx];
    SCInputRef i_Shape = sc.Input[++InputIdx];
    SCInputRef i_OutlineColor = sc.Input[++InputIdx];
    SCInputRef i_StartColor = sc.Input[++InputIdx];
    SCInputRef i_EndColor = sc.Input[++InputIdx];
    SCInputRef i_FillTransparency = sc.Input[++InputIdx];
    SCInputRef i_VerticalOffset = sc.Input[++InputIdx];
    SCInputRef i_HorizontalOffset = sc.Input[++InputIdx];
    SCInputRef i_EnableText = sc.Input[++InputIdx];
    SCInputRef i_TextColor = sc.Input[++InputIdx];
    SCInputRef i_FontSize = sc.Input[++InputIdx];
    SCInputRef i_DrawTrails = sc.Input[++InputIdx];
    SCInputRef i_TrailsNumSeconds = sc.Input[++InputIdx];

    // subgraphs
    SCSubgraphRef s_Current = sc.Subgraph[0];
    SCSubgraphRef s_Max     = sc.Subgraph[1];
    SCSubgraphRef s_PoT     = sc.Subgraph[2];

    // Set configuration variables
    if (sc.SetDefaults)
    {
        sc.GraphName = "Pace of Tape";
        sc.GraphRegion = 0;
        sc.UpdateAlways = 1;

        i_Symbol.Name = "Use Different Symbol (Blank = Chart Symbol)";
        i_Symbol.SetString("");

        i_TicksOrVolume.Name = "Calculate Ticks or Volume Per Second";
        i_TicksOrVolume.SetCustomInputStrings("Ticks;Volume");
        i_TicksOrVolume.SetCustomInputIndex(0);

        i_Layout.Name = "Layout";
        i_Layout.SetCustomInputStrings("Vertical;Horizontal");
        i_Layout.SetCustomInputIndex(0);

        i_NumRecordsToExamine.Name = "Number of seconds to examine";
        i_NumRecordsToExamine.SetInt(60);

        i_NumSquares.Name = "Number of squares/circles";
        i_NumSquares.SetInt(5);

        i_SquareSize.Name = "Square/Circle Size";
        i_SquareSize.SetInt(2);

        i_Reverse.Name = "Reverse direction?";
        i_Reverse.SetYesNo(0);

        i_CalcMethod.Name = "Calculation Method";
        i_CalcMethod.SetCustomInputStrings("Original;Lagging Max");
        i_CalcMethod.SetCustomInputIndex(1);

        i_Shape.Name = "Shape";
        i_Shape.SetCustomInputStrings("Squares;Circles");
        i_Shape.SetCustomInputIndex(0);

        i_OutlineColor.Name = "Outline Color";
        i_OutlineColor.SetColor(255,255,000);

        i_StartColor.Name = "Start (slow) Color";
        i_StartColor.SetColor(255,255,000);

        i_EndColor.Name = "End (fast) Color";
        i_EndColor.SetColor(255,000,000);

        i_FillTransparency.Name = "Fill Transparency % (0 to 100)";
        i_FillTransparency.SetInt(20);

        i_VerticalOffset.Name = "Vertical Offset";
        i_VerticalOffset.SetInt(0);

        i_HorizontalOffset.Name = "Horizontal Offset";
        i_HorizontalOffset.SetInt(5);

        i_EnableText.Name = "Enable Text?";
        i_EnableText.SetYesNo(0);

        i_TextColor.Name = "> Text Color";
        i_TextColor.SetColor(255,255,000);

        i_FontSize.Name = "> Font Size";
        i_FontSize.SetInt(12);

        i_DrawTrails.Name = "Draw Trails?";
        i_DrawTrails.SetYesNo(0);

        i_TrailsNumSeconds.Name = "> # Seconds of Trails";
        i_TrailsNumSeconds.SetInt(10);


        // subgraphs
        s_Current.Name      = "Current Rate";
        s_Current.DrawStyle = DRAWSTYLE_IGNORE;

        s_Max.Name      = "Max Rate";
        s_Max.DrawStyle = DRAWSTYLE_IGNORE;

        s_PoT.Name      = "Pace of Tape";
        s_PoT.DrawStyle = DRAWSTYLE_IGNORE;
        return;
    }

    int StudyID = sc.StudyGraphInstanceID;

    // we need to count backwards in time from the last (most recent) execution that occurred
    int NumSecondsToExamine = i_NumRecordsToExamine.GetInt();

    // struct to store raw T&S data
    c_SCTimeAndSalesArray TimeSales;

    // symbol we'll be fetching T&S data on
    SCString SymbolToUse;
    // check the input for referencing a diff symbol
    SymbolToUse = i_Symbol.GetString();
    // if no symbol provided in the input, use the chart's symbol
    if (SymbolToUse.IsEmpty()) {
        SymbolToUse = sc.Symbol;
    }

    // NOTE: MAKE SURE TO UPDATE GLOBAL SETTINGS -> NUM TIME AND SALES RECORDS!
    // grab latest time and sales data to fetch the most recent tick.
    sc.GetTimeAndSalesForSymbol(SymbolToUse, TimeSales);

    // PROBLEM - no tape found, bomb out
    if (TimeSales.Size() == 0) {
        //msg.Format("ERROR: GetTimeAndSales() returned 0 records for %s", SymbolToUse.GetChars());
        //sc.AddMessageToLog(msg, 1);
        return;
    }

    // if user has study set to hidden, don't add drawing objects
    if (sc.HideStudy) return;

    // number of squares to draw
    int NumSquares = i_NumSquares.GetInt();

    // we'll store totals of trades in a vector of this structure
    std::vector<RecordsPerUnit> Records;
    Records.clear();

    // safety check
    if (NumSecondsToExamine < NumSquares) NumSecondsToExamine = NumSquares;

    // number of records returned by SC for the T&S data
    int NumRecords = TimeSales.Size();

    // fetch the DateTime obj for the most recent tick
    SCDateTime LastDT = TimeSales[NumRecords-1].DateTime;

    // grab the integer representation of time in seconds
    int LastTimeInSec = LastDT.GetTimeInSeconds();

    // count backwards to get the "start" time we'll be examining
    int StartTimeInSec = LastTimeInSec - NumSecondsToExamine;

// msg.Format("%d records found for %s, Start=%d, Last=%d, GoBack=%d", NumRecords, SymbolToUse.GetChars(), StartTimeInSec, LastTimeInSec, GoBackCounter);
// sc.AddMessageToLog(msg, 1);

    // instantiate our struct based on the number of seconds we wish to examine
    for (int i=0; i<NumSecondsToExamine; i++) {
        RecordsPerUnit tmp;
        tmp.TimeInSeconds = StartTimeInSec + i;
        tmp.NumRecords = 0;
        Records.push_back(tmp);
    }

    // grab first tick's time
    //int FirstTimeInSec = Records[0].TimeInSeconds;

    // 0 = ticks
    // 1 = volume
    int TicksOrVolume = i_TicksOrVolume.GetIndex();

    // start storing # ticks at the start time up to end time
    for (int i=0; i<NumRecords; i++) {

        // grab next tick's time
        SCDateTime ts_DateTime = TimeSales[i].DateTime;

        // grab integer representation of time in seconds
        int TimeInSec = ts_DateTime.GetTimeInSeconds();

// msg.Format("%d >=? %d ==> %d", StartTimeInSec, TimeInSec, StartTimeInSec >= TimeInSec);
// sc.AddMessageToLog(msg, 1);
//return;

        // skip anything before our intended start time
        if (TimeInSec < StartTimeInSec) continue;

        // grab the SIDE of the execution (Bid or Ask)
        int ts_Type = TimeSales[i].Type;

        // if this is an L2 update, skip it, if its an execution, process it
        if (ts_Type == SC_TS_BID || ts_Type == SC_TS_ASK) {
            // find the correct index in the Records struct
            int MatchingIdx = -1;
            for (int j=0; j<NumSecondsToExamine; j++) {
                if (Records[j].TimeInSeconds == TimeInSec) {
                    MatchingIdx = j;
                    break;
                }
            }
            if (MatchingIdx >= 0) {
                // add this tick to this idx
                if (TicksOrVolume == 0) {
                    Records[MatchingIdx].NumRecords++;
                }
                else if (TicksOrVolume == 1) {
                    Records[MatchingIdx].NumRecords += TimeSales[i].Volume;
                }
            }
        }
    }

    // calculate the max ticks/sec and overall avg
    int CalcMethod = i_CalcMethod.GetIndex();
    int SumRecords = 0;
    float AvgRecords = 0;
    int MaxRecordsPerSecond = 0;
    int MaxRecordsTimeInSec = 0;
    for (int i=0; i<NumSecondsToExamine; i++) {
        //msg.Format("%d %d = %d", i, Records[i].TimeInSeconds, Records[i].NumRecords);
        //sc.AddMessageToLog(msg, 1);
        SumRecords += Records[i].NumRecords;
        if (Records[i].NumRecords > MaxRecordsPerSecond) {

            if (CalcMethod == 1 && (Records[i].TimeInSeconds < LastTimeInSec - (NumSecondsToExamine/NumSquares))) {
                // "lagging maximum" calculation
                // only set the max when it isn't happening right now, otherwise
                // we'll never see the gauge max out during rapid pace
                MaxRecordsPerSecond = Records[i].NumRecords;
                MaxRecordsTimeInSec = Records[i].TimeInSeconds;
            }
            else {
                // original calculation
                // set max whenever a new max records is found
                MaxRecordsPerSecond = Records[i].NumRecords;
                MaxRecordsTimeInSec = Records[i].TimeInSeconds;
            }

            Records[i].MaxRecords = MaxRecordsPerSecond;
        }
    }
    AvgRecords = SumRecords / NumSecondsToExamine;

    // calculate quick avg to help with jerkyness
    // dynamically adjust quick avg's length depending on number of squares to be drawn
    int QuickAvgLength = NumSecondsToExamine / NumSquares;
    int QuickSum = 0;
    float QuickAvg = 0;
    for (int i=NumSecondsToExamine-1-QuickAvgLength; i<NumSecondsToExamine; i++) {
        //msg.Format("%d %d = %d", i, Records[i].TimeInSeconds, Records[i].NumRecords);
        //sc.AddMessageToLog(msg, 1);
        QuickSum += Records[i].NumRecords;
    }
    QuickAvg = QuickSum / QuickAvgLength;

    // int CurrNumRecords = Records[NumSecondsToExamine-1].NumRecords;
    int CurrNumRecords = QuickAvg;

    // safety check/min feel check
    if (CurrNumRecords == 0) CurrNumRecords = Records[NumSecondsToExamine-1].NumRecords;

    // modify the max because we're never hitting the max again
    //MaxRecordsPerSecond = MaxRecordsPerSecond - QuickAvg;

    // safety checks
    if (MaxRecordsPerSecond == 0) MaxRecordsPerSecond = 1;

    // calculate the pace of tape percentage
    float PaceOfTape = (float)CurrNumRecords / (float)MaxRecordsPerSecond;

    // calculate number of squares to color in based on PoT
    int NumSquaresToColor = PaceOfTape * NumSquares;

    // TODO: smoothness adjustment...revisit this
    if (PaceOfTape >= (NumSquaresToColor/NumSquares)) {
        NumSquaresToColor += 1;
    }
    else if (PaceOfTape < 1/NumSquares) {
        NumSquaresToColor = 0;
    }

    // more safety checks
    if (PaceOfTape * 100 == 0) NumSquaresToColor = 0;

//  msg.Format("%d/%d = PoT %.2f, color %d/%d", CurrNumRecords, MaxRecordsPerSecond, PaceOfTape, NumSquaresToColor, NumSquares);
//  sc.AddMessageToLog(msg, 1);

    // get desired size of square from input
    int SquareSize = i_SquareSize.GetInt();

    // get desired colors from inputs
    COLORREF OutlineColor = i_OutlineColor.GetColor();
    COLORREF StartColor = i_StartColor.GetColor();
    COLORREF EndColor = i_EndColor.GetColor();

    // grab the fill transparency
    int FillTransparency = i_FillTransparency.GetInt();

    // extract the integer values for Red, Green, and Blue
    // for both the start color and end color
    // so we can gradient it
    int StartR = GetRValue(StartColor);
    int StartG = GetGValue(StartColor);
    int StartB = GetBValue(StartColor);
    int EndR = GetRValue(EndColor);
    int EndG = GetGValue(EndColor);
    int EndB = GetBValue(EndColor);

    // calculate the numeric difference between the end and start Reds, Greens, and Blues
    int RDiff = EndR - StartR;
    int GDiff = EndG - StartG;
    int BDiff = EndB - StartB;

    // calculate the "step size" for the Reds, Greens, and Blues that we'll jump for each consecutive square
    int RInterval = RDiff / NumSquares;
    int GInterval = GDiff / NumSquares;
    int BInterval = BDiff / NumSquares;

    // SierraChart width is 150, not 100, so we need to multiply everything by 1.5 to get correct value.
    // See docs for more info on this:
    // https://www.sierrachart.com/index.php?page=doc/ACSILDrawingTools.html#s_UseTool_BeginDateTime
    float SquareMultiplier = 1.5;

// msg.Format("%d %d %d -> %d %d %d || %d %d %d", StartR, StartG, StartB, EndR, EndG, EndB, RDiff, GDiff, BDiff);
// sc.AddMessageToLog(msg, 1);

    // grab vertical and horizontal offsets from inputs
    int VerticalOffset = i_VerticalOffset.GetInt();
    int HorizontalOffset = i_HorizontalOffset.GetInt();

    // grab layout position from input
    // 0 = vertical, 1 = horizontal
    int Layout = i_Layout.GetIndex();

    // grab bool for reversing direction
    // ex: instead of left to right, make it right to left
    bool IsReverse = i_Reverse.GetYesNo();

    // grab shape
    // 0 = squares
    // 1 = circles
    int Shape = i_Shape.GetIndex();

    // trails
    bool DrawTrails = i_DrawTrails.GetYesNo();
    int TrailsNumSeconds = i_TrailsNumSeconds.GetInt();
    if (!DrawTrails) TrailsNumSeconds = 1;

    for (int j=0; j<TrailsNumSeconds; j++) {

        int j_TimeInSeconds = Records[NumSecondsToExamine-j].TimeInSeconds;

        // draw the squares
        for (int i=0; i<NumSquares; i++) {

            // declare drawing tool object
            s_UseTool Tool;

            // TODO add input for this
            Tool.ChartNumber = sc.ChartNumber;

            // draw a rectangle
            Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
            if (Shape == 1) {
                Tool.DrawingType = DRAWING_ELLIPSEHIGHLIGHT;
            }

            // line number HAS TO BE UNIQUE for each rectangle
            // even across multiple instances of the study!
            //Tool.LineNumber =  ((1+i) * sc.StudyGraphInstanceID) + ((1+j)*sc.StudyGraphInstanceID);
            Tool.LineNumber = g_LineNumbers.AddLineNumber(StudyID, j_TimeInSeconds, i);
//msg.Format("[%d] (%d, %d) = %d", StudyID, j_TimeInSeconds, i, Tool.LineNumber);
//sc.AddMessageToLog(msg,1);

            // use relative positioning
            Tool.UseRelativeVerticalValues = 1;

            // vertical layout
            if (Layout == 0) {
                // y-axis start of rectangle
                Tool.BeginValue = SquareSize + (SquareSize*i) + VerticalOffset;

                // y-axis end of rectangle 
                Tool.EndValue = (2*SquareSize) + (SquareSize*i) + VerticalOffset;

                // x-axis start of rectangle
                Tool.BeginDateTime = 150 - (2*SquareSize*SquareMultiplier) - HorizontalOffset - (j*HorizontalOffset);

                // y-axis end of rectangle
                Tool.EndDateTime = 150-(SquareSize*SquareMultiplier) - HorizontalOffset - (j*HorizontalOffset);
            }
            // horizontal layout
            else if (Layout == 1) {
                Tool.BeginValue = VerticalOffset - (2*SquareSize) - (j*SquareSize) - (j*1);
                Tool.EndValue = VerticalOffset - (SquareSize) - (j*SquareSize) - (j*1);
                Tool.BeginDateTime = 150 - HorizontalOffset - (NumSquares*SquareSize*SquareMultiplier) + (i*SquareSize);
                Tool.EndDateTime = 150 - HorizontalOffset - (NumSquares*SquareSize*SquareMultiplier) + ((i+1)*SquareSize);
            }

            // always add if new or adjust existing rectangle
            Tool.AddMethod = UTAM_ADD_OR_ADJUST;

            // width of square outline
            Tool.LineWidth = 1;

            // transparency set in input
            Tool.TransparencyLevel = FillTransparency;

            // outline color
            Tool.Color = OutlineColor;

            int cursor = i;
            if (IsReverse) {
                // modify the cursor
                cursor = NumSquares-i-1;
            }

            // at max Pace of Tape, explicitly color the last rectangle fully opaque and with the end color
            if (NumSquares == NumSquaresToColor && cursor == NumSquares-1) {
                Tool.TransparencyLevel = 0;
                Tool.SecondaryColor = EndColor;
            }
            // paint the rectangle with the calculated gradient RGB value based on where it is located
            else if (cursor < NumSquaresToColor) {
                Tool.SecondaryColor = RGB(StartR+(cursor*RInterval), StartG+(cursor*GInterval), StartB+(cursor*BInterval));
            }
            // empty rectangles should use the chart's bg color to appear empty
            else {
                Tool.SecondaryColor = sc.ChartBackgroundColor;
            }

            if (DrawTrails && j>0) {
                int MaxTrailsTransparency = 95;
                int TransparencyDiff = (MaxTrailsTransparency - Tool.TransparencyLevel);
                int TransparencyInterval = TransparencyDiff / TrailsNumSeconds;
                Tool.TransparencyLevel = MaxTrailsTransparency - (TransparencyDiff/3) + (j*TransparencyInterval);
                if (Tool.TransparencyLevel > MaxTrailsTransparency) Tool.TransparencyLevel = MaxTrailsTransparency;
                Tool.LineWidth = 1;
                Tool.Color = sc.ChartBackgroundColor;

                // calculate the pace of tape percentage
                float TrailsPaceOfTape = (float)Records[NumSecondsToExamine-j].NumRecords / (float)MaxRecordsPerSecond;

                // calculate number of squares to color in based on PoT
                int TrailsNumSquaresToColor = TrailsPaceOfTape * NumSquares;

                // TODO: smoothness adjustment...revisit this
                if (TrailsPaceOfTape >= (TrailsNumSquaresToColor/NumSquares)) {
                    TrailsNumSquaresToColor += 1;
                }
                else if (TrailsPaceOfTape < 1/NumSquares) {
                    TrailsNumSquaresToColor = 0;
                }

                // more safety checks
                if (TrailsPaceOfTape * 100 == 0) TrailsNumSquaresToColor = 0;

                if (TrailsNumSquaresToColor > cursor) {
                    Tool.SecondaryColor = RGB(StartR+(cursor*RInterval), StartG+(cursor*GInterval), StartB+(cursor*BInterval));
                }
                else {
                    //Tool.SecondaryColor = sc.ChartBackgroundColor;
                    if (Layout == 0) {
                        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, Tool.LineNumber);
                        continue;
                    }
                }
            }

            // draw the rectangle
            sc.UseTool(Tool);
        }
    }

    // populate subgraphs
    s_Current[sc.Index] = CurrNumRecords;
    s_Max[sc.Index] = MaxRecordsPerSecond;
    s_PoT[sc.Index] = PaceOfTape;

    // contracts vs shares for text
    bool IsStock = sc.SecurityType() == n_ACSIL::SECURITY_TYPE_STOCK;

    // user input set to draw the text statistics
    if (i_EnableText.GetInt() == 1) {
        // CURR NUM RECORDS/SEC TEXT
        s_UseTool CurrNumRecordsTool;
        // TODO add input for this
        CurrNumRecordsTool.ChartNumber = sc.ChartNumber;
        CurrNumRecordsTool.DrawingType = DRAWING_TEXT;
        CurrNumRecordsTool.LineNumber = (100*sc.StudyGraphInstanceID) + 20221114;
        CurrNumRecordsTool.UseRelativeVerticalValues = 1;
        if (!sc.UserDrawnChartDrawingExists(sc.ChartNumber, CurrNumRecordsTool.LineNumber)) {
            if (Layout == 0) {
                CurrNumRecordsTool.BeginValue = SquareSize + VerticalOffset;
                CurrNumRecordsTool.BeginDateTime = 150 - (2*SquareSize*SquareMultiplier) - HorizontalOffset;
            }
            else if (Layout == 1) {
                CurrNumRecordsTool.BeginValue = (2*SquareSize) + VerticalOffset;
                CurrNumRecordsTool.BeginDateTime = 150 - HorizontalOffset - (NumSquares*SquareSize*SquareMultiplier) - (2*SquareSize);
            }
        }
        CurrNumRecordsTool.AddMethod = UTAM_ADD_OR_ADJUST;
        CurrNumRecordsTool.FontSize = i_FontSize.GetInt();
        CurrNumRecordsTool.Color = i_TextColor.GetColor();
        if (IsStock && TicksOrVolume == 1) {
            CurrNumRecords = CurrNumRecords / 1000;
            CurrNumRecordsTool.Text.Format("%dK/s", CurrNumRecords);
        }
        else {
            CurrNumRecordsTool.Text.Format("%d/s", CurrNumRecords);
        }
        CurrNumRecordsTool.AddAsUserDrawnDrawing = 1;
        sc.UseTool(CurrNumRecordsTool);

        // MAX NUMBER
        s_UseTool MaxNumRecordsTool;
        // TODO add input for this
        MaxNumRecordsTool.ChartNumber = sc.ChartNumber;
        MaxNumRecordsTool.DrawingType = DRAWING_TEXT;
        MaxNumRecordsTool.LineNumber = (100*sc.StudyGraphInstanceID) + 20221113;
        MaxNumRecordsTool.UseRelativeVerticalValues = 1;
        if (!sc.UserDrawnChartDrawingExists(sc.ChartNumber, MaxNumRecordsTool.LineNumber)) {
            if (Layout == 0) {
                MaxNumRecordsTool.BeginValue = SquareSize + (SquareSize*(NumSquares+1)) + VerticalOffset;
                MaxNumRecordsTool.BeginDateTime = 150 - (2*SquareSize*SquareMultiplier) - HorizontalOffset;
            }
            else if (Layout == 1) {
                MaxNumRecordsTool.BeginValue = (2*SquareSize) + VerticalOffset;
                MaxNumRecordsTool.BeginDateTime = 150 - HorizontalOffset - (SquareMultiplier*SquareSize);
            }
        }
        MaxNumRecordsTool.AddMethod = UTAM_ADD_OR_ADJUST;
        MaxNumRecordsTool.FontSize = i_FontSize.GetInt();
        MaxNumRecordsTool.Color = i_TextColor.GetColor();
        if (IsStock && TicksOrVolume == 1) {
            MaxRecordsPerSecond = MaxRecordsPerSecond / 1000;
            MaxNumRecordsTool.Text.Format("%dK/s", MaxRecordsPerSecond);
        }
        else {
            MaxNumRecordsTool.Text.Format("%d/s", MaxRecordsPerSecond);
        }
        MaxNumRecordsTool.AddAsUserDrawnDrawing = 1;
        sc.UseTool(MaxNumRecordsTool);

        // Pace Of Tape TEXT
        s_UseTool PoTTool;
        // TODO add input for this
        PoTTool.ChartNumber = sc.ChartNumber;
        PoTTool.DrawingType = DRAWING_TEXT;
        PoTTool.LineNumber = (100*sc.StudyGraphInstanceID) + 20221112;
        PoTTool.UseRelativeVerticalValues = 1;
        if (!sc.UserDrawnChartDrawingExists(sc.ChartNumber, PoTTool.LineNumber)) {
            if (Layout == 0) {
                PoTTool.BeginValue = SquareSize + (SquareSize*(NumSquares/2+1)) + VerticalOffset;
                PoTTool.BeginDateTime = 150 - (4*SquareSize*SquareMultiplier) - HorizontalOffset;
            }
            if (Layout == 1) {
                PoTTool.BeginValue = (2*SquareSize) + VerticalOffset + SquareSize;
                PoTTool.BeginDateTime = 150 - HorizontalOffset - (SquareMultiplier*SquareSize*(NumSquares/2)) - (2*SquareSize);
            }
        }
        PoTTool.AddMethod = UTAM_ADD_OR_ADJUST;
        PoTTool.FontSize = i_FontSize.GetInt();
        PoTTool.Color = i_TextColor.GetColor();
        PoTTool.Text.Format("%.0f%%", 100*PaceOfTape);
        PoTTool.AddAsUserDrawnDrawing = 1;
        sc.UseTool(PoTTool);

    }

}
