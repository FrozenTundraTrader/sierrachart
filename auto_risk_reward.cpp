#include "sierrachart.h"
#include <string>
SCDLLName("Frozen Tundra - Auto Risk Reward Tool")

/*
    Written by Frozen Tundra
*/


SCSFExport scsf_AutoRiskReward(SCStudyInterfaceRef sc)
{
    // logging object
    SCString msg;

    // helper class
    //Helper help(sc);

    SCInputRef i_FontSize = sc.Input[0];
    SCInputRef i_LineSize = sc.Input[1];
    SCInputRef i_MarkerSize = sc.Input[2];
    SCInputRef i_UseBold = sc.Input[3];
    SCInputRef i_TransparentBg = sc.Input[4];
    SCInputRef i_RemoveLatestDrawing = sc.Input[5];

    // Set configuration variables
    if (sc.SetDefaults)
    {
        sc.GraphName = "Auto Risk Reward Tool";
        sc.StudyDescription = "";
        sc.GraphRegion = 0;
        sc.GraphShortName = "AutoRR";
        sc.GraphRegion = 0;

        i_FontSize.Name = "Font Size";
        i_FontSize.SetInt(14);

        i_LineSize.Name = "Line Size";
        i_LineSize.SetInt(2);

        i_MarkerSize.Name = "Marker Size";
        i_MarkerSize.SetInt(8);

        i_UseBold.Name = "Bold Text?";
        i_UseBold.SetYesNo(1);

        i_TransparentBg.Name = "Transparent Background?";
        i_TransparentBg.SetYesNo(0);

        i_RemoveLatestDrawing.Name = "Remove Drawing After Position Closed?";
        i_RemoveLatestDrawing.SetYesNo(0);
        return;
    }

    // if Remove Latest Drawing setting is on, remove the latest drawing if position closed
    int &LineNumber = sc.GetPersistentInt(1);
    // grab position data for this chart
    s_SCPositionData PositionData;
    sc.GetTradePosition(PositionData);
    // grab curr position size
    int CurrentPositionSize = PositionData.PositionQuantity;
    // if size is 0 (closed position) and we want to discard drawings, delete drawing
    if (i_RemoveLatestDrawing.GetInt() == 1 && CurrentPositionSize == 0) {
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, LineNumber);
    }


    // persistent vars for when Sim orders disappear from Trade Orders window
    // (Guitarmadillo: this doesn't happen with live orders )
    int EntryIndex = sc.GetPersistentInt(2);
    double EntryPrice = sc.GetPersistentFloat(3);
    double StopPrice = sc.GetPersistentFloat(4);
    double TargetPrice = sc.GetPersistentFloat(5);

    // locate most recent fill
    s_SCOrderFillData LastFill;
    int i = 1;
    // keep track of parent order id
    int ParentOrderId = -1;
    s_SCTradeOrder LastOrder;
    // while we havent found the parent order
    while (ParentOrderId != 0) {
        // grab the next most recent fill
        sc.GetOrderFillEntry(sc.GetOrderFillArraySize()-i, LastFill);
        // grab the order id for this fill
        int LastOrderId = LastFill.InternalOrderID;
        // grab the order's details
        sc.GetOrderByOrderID(LastOrderId, LastOrder);
        // grab the parent id
        ParentOrderId = LastOrder.ParentInternalOrderID;
        //SCString OrderType = LastOrder.OrderType;
        //int OrderStatusCode = LastOrder.OrderStatusCode;
        i++;
    }

    // grab the internal order id of the latest entry
    int EntryOrderId = LastOrder.InternalOrderID;

    // fetch entry order details
    s_SCTradeOrder EntryOrder;
    sc.GetOrderByOrderID(EntryOrderId, EntryOrder);

    // fetch the target and stop order ids from the entry
    // these are the children
    int StopOrderId = EntryOrder.StopChildInternalOrderID;
    int TargetOrderId = EntryOrder.TargetChildInternalOrderID;

    // bomb out if we have had a manual fill thats not OCO
    if (EntryOrderId <= 0 || EntryOrderId <= 0 || EntryOrderId <= 0) {
        return;
    }

    msg.Format("EntryOrderId=%d, StopOrderId=%d, TgtOrderId=%d", EntryOrderId, StopOrderId, TargetOrderId);
    //help.dump(msg);

    // fetch Entry order details
    if (EntryPrice <= 0) EntryPrice = EntryOrder.Price1;
    SCDateTime EntryDateTime = EntryOrder.EntryDateTime;
    // grab the bar index for this date
    if (EntryIndex <= 0) EntryIndex = sc.GetContainingIndexForSCDateTime(sc.ChartNumber, EntryDateTime);

    // fetch stop and target orders
    s_SCTradeOrder StopOrder;
    sc.GetOrderByOrderID(StopOrderId, StopOrder);
    s_SCTradeOrder TargetOrder;
    sc.GetOrderByOrderID(TargetOrderId, TargetOrder);

    // fetch prices
    if (TargetPrice <= 0) TargetPrice = TargetOrder.Price1;
    if (StopPrice <= 0) StopPrice = StopOrder.Price1;

    // if we had to go back further than 1 to find the parent entry
    // then we are in a closed state, we do not have an active order
    // THUS, do not re-draw the tool

    if (EntryPrice <= 0 || StopPrice <= 0 || TargetPrice <= 0) return;

    msg.Format("Entry=%f, Stop=%f, Target=%f", EntryPrice, StopPrice, TargetPrice);
    //help.dump(msg);


    // draw the Risk Reward Tool onto the chart
    s_UseTool Tool;

    Tool.ChartNumber = sc.ChartNumber;
    if(LineNumber != 0) Tool.LineNumber = LineNumber;

    Tool.Region = 0;
    Tool.AddMethod = UTAM_ADD_OR_ADJUST;

    Tool.DrawingType = DRAWING_REWARD_RISK;

    Tool.BeginIndex = EntryIndex;
    Tool.BeginValue = StopPrice;
    Tool.EndIndex = EntryIndex;
    Tool.EndValue = EntryPrice;
    Tool.ThirdIndex = sc.ArraySize-1;
    Tool.ThirdValue = TargetPrice;

    Tool.Color = COLOR_YELLOW;                // text color
    Tool.TransparentLabelBackground = i_TransparentBg.GetInt();    // opaque labels
    Tool.TextAlignment = DT_RIGHT;            // text alignment (target marker is left/right of text)
    Tool.ShowTickDifference = 1;            // text options
    Tool.ShowPriceDifference = 1;
    Tool.ShowCurrencyValue = 1;
    Tool.FontFace = sc.GetChartTextFontFaceName();    // override chart drawing text font
    Tool.FontSize = i_FontSize.GetInt();
    Tool.FontBold = i_UseBold.GetInt();

    // remaining options are controlled via the use tool levels
    Tool.LevelColor[0] = COLOR_DARKRED;            // stop to entry line
    Tool.LevelStyle[0] = LINESTYLE_SOLID;
    Tool.LevelWidth[0] = i_LineSize.GetInt();

    Tool.LevelColor[1] = COLOR_DARKGREEN;        // entry to target line
    Tool.LevelStyle[1] = LINESTYLE_SOLID;
    Tool.LevelWidth[1] = i_LineSize.GetInt();

    Tool.LevelColor[2] = COLOR_WHITE;            // entry marker
    Tool.LevelStyle[2] = MARKER_POINT;            // marker type
    Tool.RetracementLevels[2] = i_MarkerSize.GetInt();                // marker size
    Tool.LevelWidth[2] = i_LineSize.GetInt();                        // marker width

    Tool.LevelColor[3] = COLOR_RED;                // stop marker
    Tool.LevelStyle[3] = MARKER_DASH;            // marker type
    Tool.RetracementLevels[3] = i_MarkerSize.GetInt();                // marker size
    Tool.LevelWidth[3] = i_LineSize.GetInt();                        // marker width

    Tool.LevelColor[4] = COLOR_ORANGE;            // target marker
    Tool.LevelStyle[4] = MARKER_STAR;            // marker type
    Tool.RetracementLevels[4] = i_MarkerSize.GetInt();                // marker size
    Tool.LevelWidth[4] = i_LineSize.GetInt();                        // marker width

    sc.UseTool(Tool);  // here we make the function call to add the reward risk tool
    LineNumber = Tool.LineNumber; // remember line number which has been automatically set
}   
