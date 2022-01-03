#include "sierrachart.h"

SCDLLName("Frozen Tundra Discord Room Studies")

/*
	Written by Malykubo and Frozen Tundra in FatCat's Discord Room
	This custom study requires Market Depth data.
	Computes the average number of lots advertised per order
	and displays them in General Purpose Column 1 of the DOM.
*/
void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc); 

SCSFExport scsf_AverageLotSize(SCStudyInterfaceRef sc)
{
	SCInputRef NumberOfLevels = sc.Input[0];
	SCString log_message;
	
    // Configuration
    if (sc.SetDefaults)
    {
		sc.GraphRegion = 0;
		sc.UsesMarketDepthData = 1;
		NumberOfLevels.Name = "Number of Levels";
		NumberOfLevels.SetInt(5);
        return;
    }
	
	int num_levels = NumberOfLevels.GetInt();
	
	// SierraChart object for a market depth record
	s_MarketDepthEntry bid_mde;
	s_MarketDepthEntry ask_mde;		
	
	// used for calculating avg lot sizes
	float bid_avg_lot = 0;
	int i_bid_avg_lot = 0;
	float ask_avg_lot = 0;
	int i_ask_avg_lot = 0;
	
	// pointers to dynamically allocated arrays
	int *p_bidAvgLots;
	int *p_askAvgLots;
				
	// malloc memory for arrays
	p_bidAvgLots = (int *) sc.AllocateMemory( 1024 * sizeof(int) );
	p_askAvgLots = (int *) sc.AllocateMemory( 1024 * sizeof(int) );
	
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
	
		// round them out to ints
		i_bid_avg_lot = (int)sc.Round(bid_avg_lot);
		i_ask_avg_lot = (int)sc.Round(ask_avg_lot);
		
		// store in arrays
		p_bidAvgLots[i] = i_bid_avg_lot;
		p_askAvgLots[i] = i_ask_avg_lot;
	
		//log_message.Format("??? bidAvg=%d x askAvg=%d", i_bid_avg_lot, i_ask_avg_lot);
		//log_message.Format(">>> bidAvg=%d x askAvg=%d", bidAvgLots[i], askAvgLots[i]);
		//sc.AddMessageToLog(log_message, 1);
	}
	
	// we need these data to persist to our windows GDI call
	sc.SetPersistentInt(0, num_levels);
	sc.SetPersistentPointer(0, p_bidAvgLots);
	sc.SetPersistentPointer(1, p_askAvgLots);
	
	//log_message.Format("?P?P? bidAvg=%d x askAvg=%d", *p_bidAvgLots, *p_askAvgLots);
	//sc.AddMessageToLog(log_message, 1);
	
	sc.p_GDIFunction = DrawToChart;
}

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{
	int num_levels = sc.GetPersistentInt(0);
	int bidX = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_1);
	int bidY;
	int askX = bidX + 30; //sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_2);
	int askY;
	SCString msg;
	
	// fetch the arrays of data
	int *p_bidAvgLots = (int *)sc.GetPersistentPointer(0);
	int *p_askAvgLots = (int *)sc.GetPersistentPointer(1);
	
	for (int i=0; i<num_levels; i++) {
		
		// calculate coords
		bidY = sc.RegionValueToYPixelCoordinate(sc.Close[sc.Index], sc.GraphRegion);
		// spacing for visuals
		bidY += (i * 30);
		askY = sc.RegionValueToYPixelCoordinate(sc.Close[sc.Index], sc.GraphRegion) - 30;
		// spacing for visuals
		askY -= (i * 30);
		
		// munge bid side text together
		msg.Format("%d", *(p_bidAvgLots + i));
		// print bid side text on DOM
		::SetTextAlign(DeviceContext, TA_NOUPDATECP);
		::TextOut(DeviceContext, bidX, bidY, msg, msg.GetLength());
		
		// munge ask side text together
		msg.Format("%d", *(p_askAvgLots + i));
		// print ask side text to DOM
		::SetTextAlign(DeviceContext, TA_NOUPDATECP);
		::TextOut(DeviceContext, askX, askY, msg, msg.GetLength());

		//msg.Format("COORDS: %d x %d", bidX, bidY);
		//sc.AddMessageToLog(msg, 1);
	}

	return;
}
