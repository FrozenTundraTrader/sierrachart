#include "sierrachart.h"
#include <vector>
#include <unordered_map>
#include <cstring>
using std::string;
SCDLLName("Frozen Tundra - Tape On Chart")
std::string REVISION = "2023-12-30b";

// NOTE: This study uses Windows GDI functions for drawing.
//       If you have OpenGL enabled in Global Settings,
//       then this study WILL NOT work for you.

// Windows GDI hook
void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc);

// struct to hold repeating print data to calculate icebergs
struct RepeatRecord
{
    float Price;
    int NumConsecPrints;
    int TotalVolume;
    SCDateTime DateTime;
    int MaxDepthObserved;
    int TradeType;
    int Index;
    int OccurrenceWithinIndex;
};

// struct to hold a symbol's various metadata being collected and calculated
struct SymbolData
{
    // used to keep track of which time and sales records already were processed
    int LatestSequence = 0;

    // used to draw different sized orbs for relative iceberg sizing
    int LargestSizeSeen = 0;

    // raw tape stored here to be drawn in drawing function
    // NOTE: TimeAndSales cannot be trusted/depended on from the GDI hook function per SC feedback,
    //       which is why we need to read & store it from the main study function
    //       and then fetch it from the GDI hook function.
    std::vector<s_TimeAndSales> Tape;

    // large executions stored here
    std::vector<s_TimeAndSales> LargeRecords;

    // repeating records stored here - potential icebergs
    std::vector<RepeatRecord> RepeatRecords;

    // list of bar indicies with count of repeating records for each bar index
    std::unordered_map<int, int> RepeatRecordsByIndex;

};

// primary struct to hold various info
// TOC = Tape On Chart
struct TOC
{
    SCStudyInterfaceRef sc;

    // needed to detect change in chart symbol
    SCString PrevSymbol;

    // needed to detect change in chart bar period
    int PrevBarPeriod;

    // struct to hold tape, records of large or repeating prints, icebergs
    std::unordered_map<std::string, SymbolData> SymData;

    // constructor - set SC obj
    TOC(SCStudyInterfaceRef &SCIntRef) : sc(SCIntRef)
    {
    }

    // Re-sort icebergs by (new) bar index when symbol/bar period changed
    void ReIndexRepeatRecords(const std::string &Symbol)
    {
        SCString msg;
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // clear current indexes we have stored
            FoundRecord->second.RepeatRecordsByIndex.clear();

            int NumRecords = FoundRecord->second.RepeatRecords.size();

            // Key => Value map of
            // BAR INDEX => # OCCURENCES WITHIN INDEX
            std::unordered_map<int, int> Occurrences;

            // Iterate through everything and re-index icebergs
            for (int i=0; i<NumRecords; i++)
            {
                RepeatRecord tmp = FoundRecord->second.RepeatRecords[i];
                SCDateTime dt = tmp.DateTime;
                int NewIndex = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, dt);
msg.Format("ReIndexing %s, %d/%d moved to idx=%d, %d:%d:%d", Symbol.c_str(), i, NumRecords, NewIndex, dt.GetHour(), dt.GetMinute(), dt.GetSecond());
sc.AddMessageToLog(msg,0);
                if (NewIndex > sc.ArraySize-1)
                {
msg.Format("ReIndexing %s, NewIdx=%d is past ArraySize, resetting to the end=%d", Symbol.c_str(), NewIndex, sc.ArraySize-1);
sc.AddMessageToLog(msg,0);
                    NewIndex = sc.ArraySize-1;
                }
                if (NewIndex < 0) continue;
                Occurrences[NewIndex]++;
                FoundRecord->second.RepeatRecords[i].Index = NewIndex;
                FoundRecord->second.RepeatRecords[i].OccurrenceWithinIndex = Occurrences[NewIndex];
                IncrementNumRepeatRecordsForIndex(Symbol.c_str(), NewIndex);
            }
        }
    }

    // Returns number of repeating records for a provided bar index,
    // used to calculate spacing for drawing icebergs
    int GetNumRepeatRecordsForIndex(const std::string &Symbol, int Index)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // Return number of repeat records for this index
            return(FoundRecord->second.RepeatRecordsByIndex[Index]);
        }
        return 0;
    }

    // helper setter fn
    void IncrementNumRepeatRecordsForIndex(const std::string &Symbol, int Index)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            auto FoundIndexRecord = FoundRecord->second.RepeatRecordsByIndex.find(Index);
            if (FoundIndexRecord != FoundRecord->second.RepeatRecordsByIndex.end())
            {
                FoundRecord->second.RepeatRecordsByIndex[Index]++;
            }
            else
            {
                FoundRecord->second.RepeatRecordsByIndex[Index] = 1;
            }
        }
    }

    // helper getter fn
    // Returns largest size seen, used mainky for calculating iceberg sizes
    int GetLargeRecordsSize(const std::string &Symbol)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // Return largest Size seen
            return static_cast<int>(FoundRecord->second.LargeRecords.size());
        }
        return 0;
    }

    // helper getter fn
    // Returns a T&S record that had quantity greater than threshold
    bool GetLargeRecord(const std::string &Symbol, int Index, s_TimeAndSales &r)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // Find it
            int NumRecordsForSymbol = GetLargeRecordsSize(Symbol);
            if (Index <= NumRecordsForSymbol-1)
            {
                r = FoundRecord->second.LargeRecords[Index];
                return true;
            }
        }
        // doesn't exist
        return false;
    }

    // Adds T&S record that is above threshold size
    void AddLargeRecord(const std::string &Symbol, s_TimeAndSales r)
    {
        SCString msg;
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // add record to existing
            int NumRecordsForSymbol = static_cast<int>(GetLargeRecordsSize(Symbol));
            FoundRecord->second.LargeRecords.push_back(r);  // throws cpu exception, never gets to next dump statement
        }
        else
        {
            SymbolData sd;

            // add the record
            sd.LargeRecords.push_back(r);

            // doesn't exist yet in the map, add it
            SymData.emplace(Symbol, sd);
        }
    }

    // Trims arrays to keep memory footprint lower
    void TrimTimeAndSales(const std::string &Symbol, int NUM_PRINTS_TO_DISPLAY)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            while (GetTimeAndSalesSize(Symbol) > NUM_PRINTS_TO_DISPLAY)
            {
                FoundRecord->second.Tape.erase(FoundRecord->second.Tape.begin());

                if (FoundRecord->second.LargeRecords.size() > NUM_PRINTS_TO_DISPLAY && FoundRecord->second.LargeRecords.size() > 0)
                {
                    FoundRecord->second.LargeRecords.erase(FoundRecord->second.LargeRecords.begin());
                }
            }
        }
    }

    // Returns number of records of T&S for provided symbol
    int GetTimeAndSalesSize(const std::string &Symbol)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // Return largest Size seen
            return FoundRecord->second.Tape.size();
        }
        return 0;
    }

    // Returns a specific time and sales record for a provided Symbol
    // int Index - element index (NOT chart bar index) in the Tape array for the specific T&S record to fetch
    bool GetTimeAndSalesRecord(const std::string &Symbol, int Index, s_TimeAndSales &r)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // Find it
            int NumRecordsForSymbol = SymData[Symbol].Tape.size();
            if (Index < NumRecordsForSymbol && Index >= 0)
            {
                r = FoundRecord->second.Tape[NumRecordsForSymbol - 1 - Index];
                return true;
            }
        }
        // not found or doesn't exist
        return false;
    }

    // Adds T&S to internal arrays for drawing later
    void AddTimeAndSalesRecord(const std::string &Symbol, s_TimeAndSales r)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // add record to existing
            FoundRecord->second.Tape.push_back(r);
        }
        else
        {
            SymbolData sd;

            // add the record
            sd.Tape.push_back(r);

            // doesn't exist yet in the map, add it
            SymData.emplace(Symbol, sd);
        }
    }

    // Set largest quantity seen executed in a single trade for a given symbol
    void SetLargestSizeSeen(const std::string &Symbol, int Size)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // set it
            FoundRecord->second.LargestSizeSeen = Size;
        }
    }

    // get largest quantity seen executed in a single trade for a given symbol
    int GetLargestSizeSeen(const std::string &Symbol)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // Return largest Size seen
            return static_cast<int>(FoundRecord->second.LargestSizeSeen);
        }
        return 0;
    }

    // Set most recent sequence number (an ID for an execution essentially) for a given symbol
    void SetLatestSequenceForSymbol(const std::string &Symbol, int Sequence)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // set it
            FoundRecord->second.LatestSequence = Sequence;
        }
    }

    // get most recent sequence number (an ID for an execution essentially) for a given symbol
    int GetLatestSequenceForSymbol(const std::string &Symbol)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // Return latest sequence number
            return FoundRecord->second.LatestSequence;
        }
        return 0;
    }

    // track repeating prints on a given symbol
    void AddRepeatRecord(const std::string &Symbol, RepeatRecord &r, int Index)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // add record to existing
            FoundRecord->second.RepeatRecords.push_back(r);
        }
        else
        {
            SymbolData sd;

            // add the record
            sd.RepeatRecords.push_back(r);

            // doesn't exist yet in the map, add it
            SymData.emplace(Symbol, sd);
        }

        // Tracks num consec repeating prints for iceberg calculation
        IncrementNumRepeatRecordsForIndex(Symbol.c_str(), Index);
    }

    // fetch repeating print records for a given symbol - iceberg related analysis
    void GetAllRepeatRecordsForSymbol(const std::string &Symbol, std::vector<RepeatRecord> &records)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            records = FoundRecord->second.RepeatRecords;
        }
        else
        {
            // doesn't exist, no records
            return;
        }
    }

    // get a specific repeating record for a given symbol
    bool GetRepeatRecord(const std::string &Symbol, int Index, RepeatRecord &r)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            // Find it
            int NumRecordsForSymbol = FoundRecord->second.RepeatRecords.size();
            if (Index <= NumRecordsForSymbol-1)
            {
                r = FoundRecord->second.RepeatRecords[Index];
                return true;
            }
        }
        // doesn't exist
        return false;
    }

    // resets/clears cached data for a given symbol
    void ClearAll(const std::string &Symbol)
    {
        SCString msg;
        std::vector<std::string> SymToDelete;
        for (auto& FoundRecord: SymData)
        {
            msg.Format("ClearAll(%s): Tape=%d, LargeRecords=%d, RepeatRecords=%d", FoundRecord.first.c_str(), FoundRecord.second.Tape.size(), FoundRecord.second.LargeRecords.size(), FoundRecord.second.RepeatRecords.size());
            sc.AddMessageToLog(msg, 0);

            FoundRecord.second.Tape.clear();
            FoundRecord.second.LargeRecords.clear();
            FoundRecord.second.LatestSequence = 0;

            // if no symbol passed, delete everything
            if (Symbol.c_str() == "")
            {
                SymToDelete.push_back(FoundRecord.first.c_str());
            }
            else
            {
                if (FoundRecord.second.RepeatRecords.size() == 0)
                {
                    SymToDelete.push_back(FoundRecord.first.c_str());
                }
            }

            //FoundRecord.second.LargestSizeSeen = 0;
            //FoundRecord.second.RepeatRecords.clear();
            //FoundRecord.second.RepeatRecordsByIndex.clear();
        }

        for (int i=0; i<SymToDelete.size(); i++)
        {
            SymData.erase(SymData.find(SymToDelete[i]));
        }
    }

    // used for iceberg drawing
    // returns number of repeat prints for a given symbol
    int GetNumRepeatRecordsForSymbol(const std::string &Symbol)
    {
        auto FoundRecord = SymData.find(Symbol.c_str());
        if (FoundRecord != SymData.end())
        {
            return(FoundRecord->second.RepeatRecords.size());
        }
        // doesn't exist, no records
        return(0);
    }

};

SCSFExport scsf_TapeOnChart(SCStudyInterfaceRef sc)
{
    // Subgraphs
    int SubgraphIdx = -1;
    //SCSubgraphRef s_LargePrints = sc.Subgraph[++SubgraphIdx];
    //SCSubgraphRef s_RepeatPrints = sc.Subgraph[++SubgraphIdx];

    // Inputs
    int InputIdx = -1;
    // numeric
    SCInputRef i_MinVolumeFilter         = sc.Input[++InputIdx];
    SCInputRef i_MaxVolumeFilter         = sc.Input[++InputIdx];
    SCInputRef i_TradeTypeFilter         = sc.Input[++InputIdx];
    SCInputRef i_LargeExecutionThreshold = sc.Input[++InputIdx];
    SCInputRef i_HugeExecutionThreshold  = sc.Input[++InputIdx];
    SCInputRef i_NumPrints               = sc.Input[++InputIdx];
    SCInputRef i_NumPinnedPrints         = sc.Input[++InputIdx];
    SCInputRef i_NumSecondsBeforeFade    = sc.Input[++InputIdx];
    SCInputRef i_MinNumPrintsForIceberg  = sc.Input[++InputIdx];
    SCInputRef i_FontSize                = sc.Input[++InputIdx];
    SCInputRef i_xOffset                 = sc.Input[++InputIdx];
    SCInputRef i_yOffset                 = sc.Input[++InputIdx];
    SCInputRef i_TopPadding              = sc.Input[++InputIdx];
    SCInputRef i_BottomPadding           = sc.Input[++InputIdx];

    // colors
    SCInputRef i_DefaultTextColor        = sc.Input[++InputIdx];
    SCInputRef i_BidColor                = sc.Input[++InputIdx];
    SCInputRef i_AskColor                = sc.Input[++InputIdx];
    SCInputRef i_LargeBidColor           = sc.Input[++InputIdx];
    SCInputRef i_LargeBidBgColor         = sc.Input[++InputIdx];
    SCInputRef i_LargeAskColor           = sc.Input[++InputIdx];
    SCInputRef i_LargeAskBgColor         = sc.Input[++InputIdx];
    SCInputRef i_HugeBidColor            = sc.Input[++InputIdx];
    SCInputRef i_HugeBidBgColor          = sc.Input[++InputIdx];
    SCInputRef i_HugeAskColor            = sc.Input[++InputIdx];
    SCInputRef i_HugeAskBgColor          = sc.Input[++InputIdx];
    SCInputRef i_PinnedBgColor           = sc.Input[++InputIdx];

    if (sc.SetDefaults)
    {
        // sc defaults
        sc.GraphName.Format("Tape On Chart Rev %s", REVISION.c_str());
        sc.GraphShortName.Format("TOC Rev %s", REVISION.c_str());
        sc.GraphRegion = 0;

        sc.ReceiveCharacterEvents = 1;

        // subgraphs
        //s_LargePrints.Name = "Large Prints";
        //s_LargePrints.DrawStyle = DRAWSTYLE_POINT;
        //s_RepeatPrints.Name = "Repeat Prints";
        //s_RepeatPrints.DrawStyle = DRAWSTYLE_POINT;

        // numeric inputs
        i_MinVolumeFilter.Name = "Minimum Volume Filter (0=off)";
        i_MinVolumeFilter.SetInt(0);

        i_MaxVolumeFilter.Name = "Maximum Volume Filter (0=off)";
        i_MaxVolumeFilter.SetInt(0);

        i_TradeTypeFilter.Name = "Filter trades by type";
        i_TradeTypeFilter.SetCustomInputStrings("Show All Executions;Bid Only;Ask Only;");
        i_TradeTypeFilter.SetCustomInputIndex(0);

        i_LargeExecutionThreshold.Name = "Minimum Volume for 'Large' Prints & Icebergs";
        i_LargeExecutionThreshold.SetInt(4000);

        i_HugeExecutionThreshold.Name = "Minimum Volume for 'Huge' Prints";
        i_HugeExecutionThreshold.SetInt(10000);

        i_NumPrints.Name = "Number of prints to display on tape";
        i_NumPrints.SetInt(20);

        i_NumPinnedPrints.Name = "Maximum number of pinned large prints to display below tape";
        i_NumPinnedPrints.SetInt(5);

        i_NumSecondsBeforeFade.Name = "Number of seconds to wait before removing pinned prints";
        i_NumSecondsBeforeFade.SetInt(10);

        i_MinNumPrintsForIceberg.Name = "Minimum number of consecutive prints needed for iceberg";
        i_MinNumPrintsForIceberg.SetInt(3);

        i_FontSize.Name = "Font Size";
        i_FontSize.SetInt(20);

        i_xOffset.Name = "X-coordinate Offset (px)";
        i_xOffset.SetInt(50);

        i_yOffset.Name = "Y-coordinate Offset (px)";
        i_yOffset.SetInt(0);

        i_TopPadding.Name = "Top Padding (px)";
        i_TopPadding.SetInt(0);

        i_BottomPadding.Name = "Bottom Padding (px)";
        i_BottomPadding.SetInt(0);

        // color inputs

        i_DefaultTextColor.Name = "Default Text Color";
        i_DefaultTextColor.SetColor(COLOR_GRAY);

        i_BidColor.Name = "Bid Text Color";
        i_BidColor.SetColor(COLOR_RED);

        i_AskColor.Name = "Ask Text Color";
        i_AskColor.SetColor(COLOR_YELLOW);

        i_LargeBidColor.Name = "Large Bid Text Color";
        i_LargeBidColor.SetColor(COLOR_WHITE);

        i_LargeBidBgColor.Name = "Large Bid Background Color";
        i_LargeBidBgColor.SetColor(COLOR_RED);

        i_LargeAskColor.Name = "Large Ask Text Color";
        i_LargeAskColor.SetColor(COLOR_BLACK);

        i_LargeAskBgColor.Name = "Large Ask Background Color";
        i_LargeAskBgColor.SetColor(COLOR_YELLOW);

        i_HugeBidColor.Name = "Huge Bid Text Color";
        i_HugeBidColor.SetColor(COLOR_WHITE);

        i_HugeBidBgColor.Name = "Huge Bid Background Color";
        i_HugeBidBgColor.SetColor(COLOR_RED);

        i_HugeAskColor.Name = "Huge Ask Text Color";
        i_HugeAskColor.SetColor(COLOR_BLACK);

        i_HugeAskBgColor.Name = "Huge Ask Background Color";
        i_HugeAskBgColor.SetColor(COLOR_YELLOW);

        i_PinnedBgColor.Name = "Pinned Prints Background Color";
        i_PinnedBgColor.SetColor(COLOR_BLACK);

        return;
    }

    // set GDI hook
    sc.p_GDIFunction = DrawToChart;

    SCString msg;

    // bar period for current chart
    n_ACSIL::s_BarPeriod bp;
    sc.GetBarPeriodParameters(bp);

    // attempt to fetch mother struct
    TOC *p_toc = (TOC*)sc.GetPersistentPointer(0);
    if (p_toc == NULL)
    {
        // couldn't find it, create a new one
        msg.Format("Creating new TOC...");
        sc.AddMessageToLog(msg,0);
        p_toc = new TOC(sc);
        sc.SetPersistentPointer(0, p_toc);
        p_toc->PrevSymbol = sc.Symbol;
        p_toc->PrevBarPeriod = bp.IntradayChartBarPeriodParameter1;
    }

    // fetch input values
    int NUM_PRINTS_TO_DISPLAY       = i_NumPrints.GetInt();
    int NUM_LARGE_PRINTS_TO_DISPLAY = i_NumPinnedPrints.GetInt();
    int NUM_SECONDS_BEFORE_FADE     = i_NumSecondsBeforeFade.GetInt();
    int NUM_PRINTS_FOR_ICEBERG      = i_MinNumPrintsForIceberg.GetInt();
    int HIGH_VOLUME_THRESHOLD       = i_LargeExecutionThreshold.GetInt();
    int HUGE_VOLUME_THRESHOLD       = i_HugeExecutionThreshold.GetInt();

    // grab raw time and sales
    c_SCTimeAndSalesArray TaS;
    if (sc.Index == sc.ArraySize-1)
    {
        // only get T&S on live bar
        sc.GetTimeAndSales(TaS);
    }

    // num records in received time and sales
    int NumRecords = TaS.Size();
//msg.Format("Number of T&S Records Found = %d", NumRecords);
//sc.AddMessageToLog(msg,0);

    // safety check
    if (NumRecords == 0 && TaS.Size() == 0)
    {
        // no ts found
        //sc.AddMessageToLog("No Time And Sales Records found", 0);
        //p_toc->ClearAll(sc.Symbol.GetChars());
        return;
    }

    // - - -
    // fetch various persistent variables needed to keep track of events
    // - - -

    // Previous Price - used to detect repeat consecutive prints which might be icebergs
    float &PrevPrice = sc.GetPersistentFloat(0);

    // NumConsecPrintsSamePrice - counter for number of repeated prints
    int &NumConsecPrintsSamePrice = sc.GetPersistentInt(1);

    // TotalVolumeSamePrice - temporary counter for total volume transacted at price in the most recent tape
    int &TotalVolumeSamePrice = sc.GetPersistentInt(2);

    // LargestAdvertisedSize - track the largest thus-far advertised size while seeing repeated prints to determine if iceberg or not
    int &LargestAdvertisedSize = sc.GetPersistentInt(3);

    // HOTKEY for QUICK MANUAL RESET OF STUDY
    // reset and recalc
    // CLEAR and RESET EVERYTHING
    int ClearInternalArraysKeyCode = 32; // space bar = 32
    int CharCode = sc.CharacterEventCode;
    if (CharCode == ClearInternalArraysKeyCode
            || sc.Symbol != p_toc->PrevSymbol
            || bp.IntradayChartBarPeriodParameter1 != p_toc->PrevBarPeriod
            || sc.Index == 0
        )
    {
        //msg.Format("1.1 RESET index 0 reset");
        //sc.AddMessageToLog(msg,0);

        p_toc->ClearAll(sc.Symbol.GetChars());

        // dont clear on symbol change
        //if (sc.Symbol == p_toc->PrevSymbol) p_toc->RepeatRecords->clear();
        if (sc.Symbol == p_toc->PrevSymbol && CharCode == ClearInternalArraysKeyCode)
        {
//msg.Format("Same symbol but different bar period. Resetting RepeatRecords arrays");
//sc.AddMessageToLog(msg,0);
            p_toc->ClearAll("");
            p_toc->SymData[sc.Symbol.GetChars()].RepeatRecords.clear();
            p_toc->SymData[sc.Symbol.GetChars()].RepeatRecordsByIndex.clear();
        }

        if (bp.IntradayChartBarPeriodParameter1 != p_toc->PrevBarPeriod)
        {
msg.Format("Same symbol but different bar period. ReIndexing RepeatRecords arrays");
sc.AddMessageToLog(msg,0);
            // re-index repeat records - we have changed bar period interval
            p_toc->ReIndexRepeatRecords(sc.Symbol.GetChars());
            p_toc->SetLatestSequenceForSymbol(sc.Symbol.GetChars(), 0);
        }

        p_toc->PrevSymbol = sc.Symbol;
        p_toc->PrevBarPeriod = bp.IntradayChartBarPeriodParameter1;

        PrevPrice = 0;
        NumConsecPrintsSamePrice = 0;
        TotalVolumeSamePrice = 0;
        LargestAdvertisedSize = 0;
    }

    SCString Output;
    int Counter = 0;
    // print most recent executions from time and sales
    for (int i=NumRecords-1; i>=NumRecords-NUM_PRINTS_TO_DISPLAY && i>=0; --i)
    {
        SCDateTime DateTime = TaS[i].DateTime;
        DateTime += sc.TimeScaleAdjustment;
        float Price = sc.RoundToTickSize(TaS[i].Price, sc.TickSize);
        int Volume = TaS[i].Volume;
        float Bid = sc.RoundToTickSize(TaS[i].Bid, sc.TickSize);
        float Ask = sc.RoundToTickSize(TaS[i].Ask, sc.TickSize);
        int BidSize = TaS[i].BidSize;
        int AskSize = TaS[i].AskSize;
        int Type = TaS[i].Type;
        int Sequence = TaS[i].Sequence;
        int TotalBidDepth = TaS[i].BidSize;
        int TotalAskDepth = TaS[i].AskSize;
        int LatestSequence = p_toc->GetLatestSequenceForSymbol(sc.Symbol.GetChars());

        // skip already processed records
        if (Sequence <= LatestSequence && LatestSequence > 0) continue;

        // skip Level 2 updates and other types, we only want actual executions
        if (Type != SC_TS_BID && Type != SC_TS_ASK) continue;

        // store this execution, we'll want to draw it
        p_toc->AddTimeAndSalesRecord(sc.Symbol.GetChars(), TaS[i]);

        // large executions get saved to a separate list
        if (Volume >= HIGH_VOLUME_THRESHOLD
            && (Sequence > LatestSequence || LatestSequence == 0)
           )
        {
            // high volume execution detected, store it
            p_toc->AddLargeRecord(sc.Symbol.GetChars(), TaS[i]);
        }


        // TODO - find more efficient way to implement this
        // housekeeping, manage memory, downsize arrays, get rid of old records we dont need to display anymore
        if (p_toc->GetTimeAndSalesSize(sc.Symbol.GetChars()) > NUM_PRINTS_TO_DISPLAY)
        {
            p_toc->TrimTimeAndSales(sc.Symbol.GetChars(), NUM_PRINTS_TO_DISPLAY);
        }
        //if (p_toc->p_LargeRecords->size() > NUM_LARGE_PRINTS_TO_DISPLAY)
        //{
        //    while (p_toc->p_LargeRecords->size() > NUM_LARGE_PRINTS_TO_DISPLAY)
        //    {
        //        p_toc->p_LargeRecords->erase(p_toc->p_LargeRecords->begin());
        //    }
        //}
        // TODO
        //if (p_toc->p_RepeatRecords->size() > NUM_LARGE_PRINTS_TO_DISPLAY)
        //{
        //    while (p_toc->p_RepeatRecords->size() > NUM_LARGE_PRINTS_TO_DISPLAY)
        //    {
        //        p_toc->p_RepeatRecords->erase(p_toc->p_RepeatRecords->begin());
        //    }
        //}

        // detect a repeating print
        if (sc.RoundToTickSize(Price, sc.TickSize) == sc.RoundToTickSize(PrevPrice, sc.TickSize))
        {
            //msg.Format("REPEAT+++++ %d for %d at %.2f", NumConsecPrintsSamePrice, TotalVolumeSamePrice, PrevPrice);
            //sc.AddMessageToLog(msg,0);
            NumConsecPrintsSamePrice++;
            TotalVolumeSamePrice += Volume;
            if (Type == SC_TS_BID && LargestAdvertisedSize < BidSize)
            {
                // update the largest size bid we've observed
                LargestAdvertisedSize = BidSize;
            }
            else if (Type == SC_TS_ASK && LargestAdvertisedSize < AskSize)
            {
                // update the largest size bid we've observed
                LargestAdvertisedSize = AskSize;
            }
        }
        else
        {
            // detect repeated prints overcoming vol threshold
            if (NumConsecPrintsSamePrice >= NUM_PRINTS_FOR_ICEBERG && TotalVolumeSamePrice >= HIGH_VOLUME_THRESHOLD)
            {
                //msg.Format("Consecutive Prints >>> %d for %d at %.2f", NumConsecPrintsSamePrice, TotalVolumeSamePrice, PrevPrice);
                //sc.AddMessageToLog(msg,0);

                // create struct and save repeating print record for rendering
                RepeatRecord tmp;
                tmp.Price = PrevPrice;
                tmp.NumConsecPrints = NumConsecPrintsSamePrice;
                tmp.TotalVolume = TotalVolumeSamePrice;
                tmp.DateTime = DateTime;
                tmp.MaxDepthObserved = LargestAdvertisedSize;
                tmp.TradeType = Type;
                tmp.Index = sc.Index;
                int NumRepeatRecordsForIndex = p_toc->GetNumRepeatRecordsForIndex(sc.Symbol.GetChars(), sc.Index);
                tmp.OccurrenceWithinIndex = NumRepeatRecordsForIndex + 1;
                p_toc->AddRepeatRecord(sc.Symbol.GetChars(), tmp, sc.Index);
                //p_toc->IncrementNumRepeatRecordsForIndex(sc.Symbol.GetChars(), sc.Index);
                //s_RepeatPrints[sc.Index] = PrevPrice;

                // store largest size seen
                int LargestSizeSeen = p_toc->GetLargestSizeSeen(sc.Symbol.GetChars());
                if (TotalVolumeSamePrice > LargestSizeSeen)
                {
                    //p_toc->LargestSizeSeen = TotalVolumeSamePrice;
                    p_toc->SetLargestSizeSeen(sc.Symbol.GetChars(), TotalVolumeSamePrice);
                }
            }

            // reset
            NumConsecPrintsSamePrice = 0;
            TotalVolumeSamePrice = 0;
            LargestAdvertisedSize = 0;
        }

        // END OF LOOP UPDATES:

        // update latest sequence number
        //p_toc->LatestSequence = Sequence;
        p_toc->SetLatestSequenceForSymbol(sc.Symbol.GetChars(), Sequence);

        // update prev price
        PrevPrice = Price;

    } // end of raw Time and Sales loop

    // cleanup, cleanup, everybody do your share
    if (sc.LastCallToFunction)
    {
        TOC *p_toc = (TOC*)sc.GetPersistentPointer(0);
        if (p_toc != NULL)
        {
            delete(p_toc);
            sc.AddMessageToLog("Cleanup complete",0);
        }
    }
}

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{
    SCDateTime Now = sc.GetCurrentDateTime();

    int InputIdx = -1;
    // numeric
    SCInputRef i_MinVolumeFilter         = sc.Input[++InputIdx];
    SCInputRef i_MaxVolumeFilter         = sc.Input[++InputIdx];
    SCInputRef i_TradeTypeFilter         = sc.Input[++InputIdx];
    SCInputRef i_LargeExecutionThreshold = sc.Input[++InputIdx];
    SCInputRef i_HugeExecutionThreshold  = sc.Input[++InputIdx];
    SCInputRef i_NumPrints               = sc.Input[++InputIdx];
    SCInputRef i_NumPinnedPrints         = sc.Input[++InputIdx];
    SCInputRef i_NumSecondsBeforeFade    = sc.Input[++InputIdx];
    SCInputRef i_MinNumPrintsForIceberg  = sc.Input[++InputIdx];
    SCInputRef i_FontSize                = sc.Input[++InputIdx];
    SCInputRef i_xOffset                 = sc.Input[++InputIdx];
    SCInputRef i_yOffset                 = sc.Input[++InputIdx];
    SCInputRef i_TopPadding              = sc.Input[++InputIdx];
    SCInputRef i_BottomPadding           = sc.Input[++InputIdx];

    // colors
    SCInputRef i_DefaultTextColor        = sc.Input[++InputIdx];
    SCInputRef i_BidColor                = sc.Input[++InputIdx];
    SCInputRef i_AskColor                = sc.Input[++InputIdx];
    SCInputRef i_LargeBidColor           = sc.Input[++InputIdx];
    SCInputRef i_LargeBidBgColor         = sc.Input[++InputIdx];
    SCInputRef i_LargeAskColor           = sc.Input[++InputIdx];
    SCInputRef i_LargeAskBgColor         = sc.Input[++InputIdx];
    SCInputRef i_HugeBidColor            = sc.Input[++InputIdx];
    SCInputRef i_HugeBidBgColor          = sc.Input[++InputIdx];
    SCInputRef i_HugeAskColor            = sc.Input[++InputIdx];
    SCInputRef i_HugeAskBgColor          = sc.Input[++InputIdx];
    SCInputRef i_PinnedBgColor           = sc.Input[++InputIdx];

    SCString msg;

    int FontSize = i_FontSize.GetInt();
    int NUM_PRINTS_TO_DISPLAY = i_NumPrints.GetInt();
    int NUM_LARGE_PRINTS_TO_DISPLAY = i_NumPinnedPrints.GetInt();
    int NUM_SECONDS_BEFORE_FADE = i_NumSecondsBeforeFade.GetInt();
    int NUM_PRINTS_FOR_ICEBERG = i_MinNumPrintsForIceberg.GetInt();
    int HIGH_VOLUME_THRESHOLD = i_LargeExecutionThreshold.GetInt();
    int HUGE_VOLUME_THRESHOLD = i_HugeExecutionThreshold.GetInt();
    int MIN_VOLUME_FILTER = i_MinVolumeFilter.GetInt();
    int MAX_VOLUME_FILTER = i_MaxVolumeFilter.GetInt();

    //msg.Format("2.0 Fetching TOC pointer");
    //sc.AddMessageToLog(msg,0);

    TOC *p_toc = (TOC*)sc.GetPersistentPointer(0);
    //if (p_toc == NULL)
    //{
    //    msg.Format("2.1 no TOC ptr found, aborting");
    //    sc.AddMessageToLog(msg,0);
    //    return;
    //}


    int NumRecords = p_toc->GetTimeAndSalesSize(sc.Symbol.GetChars());
    if (NumRecords == 0 || p_toc == NULL)
    {
        // no ts found
        //sc.AddMessageToLog("B4 Drawing: No Time And Sales Records found", 0);
        //return;
    }
    else
    {
        // print time and sales on screen
        int Counter = 0;
        //int NumConsecPrintsSamePrice = 0;
        //int TotalVolumeSamePrice = 0;
        SCString Output;

        SCString chartFont = sc.ChartTextFont();
        HFONT hFont;
        try
        {
            hFont = CreateFont(FontSize,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                    CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, DEFAULT_PITCH,TEXT(chartFont));
            SetBkMode(DeviceContext, TRANSPARENT);
            SelectObject(DeviceContext, hFont);
            ::SetTextAlign(DeviceContext, TA_NOUPDATECP | TA_RIGHT);

            // get range of scale to use for UX
            float vHigh, vLow, vDiff, vLowerLimit, vUpperLimit;
            sc.GetMainGraphVisibleHighAndLow(vHigh, vLow);
            vDiff = (vHigh - vLow);
            int NumberOfLevels = vDiff / sc.TickSize;
            vLowerLimit = vLow + (vDiff / 4.0);
            vUpperLimit = vHigh - (vDiff / 4.0);

            // get x-coord based on last bar index's physical location
            int x = sc.BarIndexToXPixelCoordinate(sc.ArraySize);
            x += i_xOffset.GetInt();
            for (int i=0; i<NUM_PRINTS_TO_DISPLAY && i<NumRecords; i++)
            {
                SelectObject(DeviceContext, hFont);
                s_TimeAndSales r;
                //msg.Format("i=%d / NumRecords=%d", i, NumRecords);
                //sc.AddMessageToLog(msg,0);
                bool Success = p_toc->GetTimeAndSalesRecord(sc.Symbol.GetChars(), i, r);
                if (!Success)
                {
                    msg.Format("FAILED TAPE LOOP: i=%d / NumRecords=%d", i, NumRecords);
                    sc.AddMessageToLog(msg,0);
                    continue;
                }
                SCDateTime DateTime = r.DateTime;
                float Price = sc.RoundToTickSize(r.Price, sc.TickSize);
                int Volume = r.Volume;
                float Bid = sc.RoundToTickSize(r.Bid, sc.TickSize);
                float Ask = sc.RoundToTickSize(r.Ask, sc.TickSize);
                int BidSize = r.BidSize;
                int AskSize = r.AskSize;
                int Type = r.Type;
                int Sequence = r.Sequence;

                // safety check
                if (Price <= 0 || Sequence <= 0 || Volume <= 0) continue;

                // filter check
                if (Volume < MIN_VOLUME_FILTER || (Volume > MAX_VOLUME_FILTER && MAX_VOLUME_FILTER > 0)) continue;

                // DRAWING

                // default
                ::SetTextColor(DeviceContext, i_DefaultTextColor.GetColor());
                SetBkColor(DeviceContext, i_PinnedBgColor.GetColor());
                SetBkMode(DeviceContext, TRANSPARENT);

                // exec on bid
                if (Price <= Bid)
                {
                    ::SetTextColor(DeviceContext, i_BidColor.GetColor());
                }
                else if (Price >= Ask)
                {
                    ::SetTextColor(DeviceContext, i_AskColor.GetColor());
                }

                // LARGE/HIGH volume
                if (Volume >= HIGH_VOLUME_THRESHOLD && Volume < HUGE_VOLUME_THRESHOLD)
                {
                    // default
                    SetBkMode(DeviceContext, OPAQUE);
                    SetBkColor(DeviceContext, COLOR_WHITE);
                    if (Price <= Bid)
                    {
                        SetBkColor(DeviceContext, i_LargeBidBgColor.GetColor());
                        ::SetTextColor(DeviceContext, i_LargeBidColor.GetColor());
                    }
                    else if (Price >= Ask)
                    {
                        SetBkColor(DeviceContext, i_LargeAskBgColor.GetColor());
                        ::SetTextColor(DeviceContext, i_LargeAskColor.GetColor());
                    }
                }

                // HUGE/GIGANTIC volume
                if (Volume >= HUGE_VOLUME_THRESHOLD)
                {
                    SetBkMode(DeviceContext, OPAQUE);
                    if (Price <= Bid)
                    {
                        SetBkColor(DeviceContext, i_HugeBidBgColor.GetColor());
                        ::SetTextColor(DeviceContext, i_HugeBidColor.GetColor());
                    }
                    else if (Price >= Ask)
                    {
                        SetBkColor(DeviceContext, i_HugeAskBgColor.GetColor());
                        ::SetTextColor(DeviceContext, i_HugeAskColor.GetColor());
                    }
                }

                int y = sc.RegionValueToYPixelCoordinate(sc.GetLastPriceForTrading(), sc.GraphRegion);
                int yLowerLimit = sc.RegionValueToYPixelCoordinate(vLowerLimit, sc.GraphRegion);
                int yUpperLimit = sc.RegionValueToYPixelCoordinate(vUpperLimit, sc.GraphRegion);
                // safety check
                if (y > yLowerLimit)
                {
                    // cap y out at one third of scale...
                    y = yLowerLimit;
                }
                if (y < yUpperLimit)
                {
                    // cap y out at one third of scale...
                    y = yUpperLimit;
                }
                Counter++;
                y -= Counter * FontSize;

                int DisplayVolume = Volume;
                if (sc.SecurityType() == n_ACSIL::SECURITY_TYPE_STOCK) DisplayVolume = Volume/100;
                Output.Format("%d     %.2f", DisplayVolume, Price);
                //Output.Format("%d %d     %.2f", Sequence, DisplayVolume, Price);
                //Output.Format("%d: %.2f %d", DateTime.GetSecond(), Price, Volume/100);
                //Output.Format("%d x %d", x, y);
                //sc.AddMessageToLog(Output, 0);
                ::TextOut(DeviceContext, x, y, Output, Output.GetLength());

            } // end of raw time and sales loop

            // LARGE PRINTS
            int PinnedSize = p_toc->GetLargeRecordsSize(sc.Symbol.GetChars());
            Counter = 0;
            if (PinnedSize > 0)
            {
                SetBkMode(DeviceContext, OPAQUE);
                SetBkColor(DeviceContext, i_PinnedBgColor.GetColor());
                for (int i=0; i<NUM_LARGE_PRINTS_TO_DISPLAY && i<PinnedSize; i++)
                {
                    s_TimeAndSales Record;
                    bool Success = p_toc->GetLargeRecord(sc.Symbol.GetChars(), PinnedSize-1-i, Record);
                    if (!Success)
                    {
                        continue;
                    }
                    SCDateTime DateTime = Record.DateTime;
                    DateTime += sc.TimeScaleAdjustment;
                    int NumSecondsAgo = Now.GetTimeInSeconds() - DateTime.GetTimeInSeconds();
//msg.Format("NumSecondsAgo=%d >? FADE=%d", NumSecondsAgo, NUM_SECONDS_BEFORE_FADE);
//msg.Format("Now=%d, DT=%d", Now.GetTimeInSeconds(), DateTime.GetTimeInSeconds());
//sc.AddMessageToLog(msg, 0);
                    // fade
                    if (NumSecondsAgo > NUM_SECONDS_BEFORE_FADE)
                    {
                        continue;
                    }

                    float Price = sc.RoundToTickSize(Record.Price, sc.TickSize);
                    float Bid   = sc.RoundToTickSize(Record.Bid, sc.TickSize);
                    float Ask   = sc.RoundToTickSize(Record.Ask, sc.TickSize);
                    int Volume = Record.Volume;
                    int Type = Record.Type;

                    if (Price <=0 || Volume <= 0) continue;

                    if (Price <= Bid)
                    {
                        ::SetTextColor(DeviceContext, i_BidColor.GetColor());
                    }
                    else if (Price >= Ask)
                    {
                        ::SetTextColor(DeviceContext, i_AskColor.GetColor());
                    }
                    else
                    {
                        ::SetTextColor(DeviceContext, i_DefaultTextColor.GetColor());
                    }

                    int y = sc.RegionValueToYPixelCoordinate(sc.GetLastPriceForTrading(), sc.GraphRegion);
                    int yLowerLimit = sc.RegionValueToYPixelCoordinate(vLowerLimit, sc.GraphRegion);
                    int yUpperLimit = sc.RegionValueToYPixelCoordinate(vUpperLimit, sc.GraphRegion);
                    // safety check
                    if (y > yLowerLimit)
                    {
                        // cap y out at one third of scale...
                        y = yLowerLimit;
                    }
                    if (y < yUpperLimit)
                    {
                        // cap y out at one third of scale...
                        y = yUpperLimit;
                    }

                    //int y = sc.RegionValueToYPixelCoordinate(sc.GetLastPriceForTrading(), sc.GraphRegion);
                    y += i_yOffset.GetInt();
                    y += 1 * FontSize;
                    y += i * FontSize * 1.05;

                    Output.Format("%d     %.2f", Volume/100, Price);
                    ::TextOut(DeviceContext, x, y, Output, Output.GetLength());

                    Counter++;

                } // end large prints for loop

            } // end of large prints 

            // REPEAT PRINTS - "ICEBERGS"
            int RepeatSize = p_toc->GetNumRepeatRecordsForSymbol(sc.Symbol.GetChars());
//msg.Format("RepeatSizeTotal = %d", RepeatSize);
//sc.AddMessageToLog(msg,0);

            // Fetch largest size we've seen thusfar - used for drawing circles
            int LargestSizeSeen = p_toc->GetLargestSizeSeen(sc.Symbol.GetChars());

            // reset counter used for offsetting text when drawing
            Counter = 0;

            if (RepeatSize > 0)
            {
                SetBkMode(DeviceContext, OPAQUE);
                for (int i=0; i<NUM_LARGE_PRINTS_TO_DISPLAY && i<RepeatSize; i++)
                {
                    SetBkColor(DeviceContext, i_PinnedBgColor.GetColor());
                    RepeatRecord Record;
                    bool Success = p_toc->GetRepeatRecord(sc.Symbol.GetChars(), RepeatSize-1-i, Record);
                    if (!Success)
                    {
msg.Format("RepeatSize: unable to find repeat record %d", RepeatSize-1-i);
sc.AddMessageToLog(msg,0);
                        continue;
                    }
                    SCDateTime DateTime = Record.DateTime;
                    int NumSecondsAgo = Now.GetTimeInSeconds() - DateTime.GetTimeInSeconds();

                    float Price = (float)Record.Price;
                    int NumConsecPrints = (int)Record.NumConsecPrints;
                    int TotalVolume = (int)Record.TotalVolume;
                    int MaxDepthObserved = (int)Record.MaxDepthObserved;
                    int Type = Record.TradeType;
                    int Index = Record.Index;
                    int OccurrenceWithinIndex = Record.OccurrenceWithinIndex;
                    int BarWidthPx = sc.BarIndexToXPixelCoordinate(Index) - sc.BarIndexToXPixelCoordinate(Index - 1);

                    // check for invalid records
                    if (TotalVolume <= 0 || NumConsecPrints <= 0 || Price <= 0)
                    {
msg.Format("RepeatSize: invalid record (volume, price, something else?)");
sc.AddMessageToLog(msg,0);
                        continue;
                    }

                    int y = sc.RegionValueToYPixelCoordinate(sc.GetLastPriceForTrading(), sc.GraphRegion);
                    int yLowerLimit = sc.RegionValueToYPixelCoordinate(vLowerLimit, sc.GraphRegion);
                    int yUpperLimit = sc.RegionValueToYPixelCoordinate(vUpperLimit, sc.GraphRegion);
                    // safety check
                    if (y > yLowerLimit)
                    {
                        // cap y out at one third of scale...
                        y = yLowerLimit;
                    }
                    if (y < yUpperLimit)
                    {
                        // cap y out at one third of scale...
                        y = yUpperLimit;
                    }
                    y += i_yOffset.GetInt();
                    y += Counter * FontSize;
                    // TODO - MAGIC NUMBER
                    y += i * FontSize * 1.05;

                    if (sc.SecurityType() == n_ACSIL::SECURITY_TYPE_STOCK) MaxDepthObserved = MaxDepthObserved * 100;

                    // TODO - set color for the iceberg
                    COLORREF clr = i_DefaultTextColor.GetColor();
                    ::SetTextColor(DeviceContext, clr);
                    if (Type == SC_TS_BID)
                    {
                        clr = i_BidColor.GetColor();
                    }
                    if (Type == SC_TS_ASK)
                    {
                        clr = i_AskColor.GetColor();
                    }

                    // only draw recent ones
                    if (NumSecondsAgo <= NUM_SECONDS_BEFORE_FADE)
                    {
                        // text output
                        ::SetTextColor(DeviceContext, clr);
                        Output.Format("%d x %d (%d shown) @ %.2f", NumConsecPrints, TotalVolume, MaxDepthObserved, Price);
                        ::TextOut(DeviceContext, x, y, Output, Output.GetLength());
                    }

                    // draw bubbles
                    int x1, y1, x2, y2;
                    int SizeAdjustment = (int)(FontSize * (float)((float)TotalVolume / (float)LargestSizeSeen));
                    int NumRepeatRecordsForIndex = p_toc->GetNumRepeatRecordsForIndex(sc.Symbol.GetChars(), Index);
                    //msg.Format("%d Records for Index %d", NumRepeatRecordsForIndex, Index);
                    //sc.AddMessageToLog(msg,0);

                    // safety against 0
                    int TimeAdjustment = BarWidthPx;
                    if (NumRepeatRecordsForIndex > 0)
                    {
                        TimeAdjustment = BarWidthPx / (NumRepeatRecordsForIndex + 1);
                    }
                    y1 = sc.RegionValueToYPixelCoordinate(Price, sc.GraphRegion);
                    y2 = y1 + SizeAdjustment;
                    x1 = sc.BarIndexToXPixelCoordinate(Index);
                    int BarStart = x1 - (BarWidthPx / 2);
                    x1 = BarStart + (TimeAdjustment * OccurrenceWithinIndex);
                    x2 =  x1 + SizeAdjustment;
                    HBRUSH brush = CreateSolidBrush(clr);
                    SelectObject(DeviceContext, brush);

                    // main circle
                    Ellipse(DeviceContext, x1, y1, x2, y2);

                    // CLEAN UP - delete obj
                    //DeleteObject(brush);

                    // TODO - 3D attempt
                    int HalfwayY = y1 + ((y2-y1)/2);
                    y1 = HalfwayY + 1;
                    y2 = y1 + 2;
                    //brush = CreateSolidBrush(COLOR_WHITE);
                    //SelectObject(DeviceContext, brush);
                    Ellipse(DeviceContext, x1, y1, x2, y2);
                    DeleteObject(brush);

                    Counter++;

                } // end repeat records for loop

            } // end of repeat records
        }
        catch (const std::runtime_error &ex)
        {
            // run time error
            sc.AddMessageToLog("Drawing: Runtime error",0);
        }
        catch (const std::exception &ex)
        {
            // generic exception
            sc.AddMessageToLog("Drawing: Generic exception",0);
        }
        catch (...)
        {
            // other exception
            sc.AddMessageToLog("Drawing: Other exception",0);
        }

        // !!! IMPORTANT !!!
        // must delete this font object from memory to avoid memory leaks!
        DeleteObject(hFont);

        // reset background mode so price bar isnt all messed up
        SetBkMode(DeviceContext, OPAQUE);
    }
}
