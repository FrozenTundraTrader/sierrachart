#include "sierrachart.h"

SCDLLName("Frozen Tundra Discord Room Studies")

/*
	Written by Malykubo and Frozen Tundra in FatCat's Discord Room
*/

SCSFExport scsf_ChangeVolAtPriceMult(SCStudyInterfaceRef sc)
{
	
	SCInputRef DebugOn = sc.Input[0];
	SCInputRef LookbackInterval = sc.Input[2];
	SCInputRef MagicNumber = sc.Input[3];
	SCInputRef UseBarAverageInstead = sc.Input[4];
	SCInputRef MinimalVapPercentageChangeThreshold = sc.Input[5];
	SCString log_message;
	
    // Configuration
    if (sc.SetDefaults)
    {
		sc.GraphRegion = 0;
		DebugOn.Name = "Debug Enabled?";
		DebugOn.SetYesNo(0);
		LookbackInterval.Name = "Lookback Interval # Bars";
		LookbackInterval.SetInt(sc.IndexOfLastVisibleBar - sc.IndexOfFirstVisibleBar);
		MagicNumber.Name = "Magic Multiplier (ticks)";
		MagicNumber.SetFloat(0.3);
		UseBarAverageInstead.SetYesNo(0);
		UseBarAverageInstead.Name = "Use bar averages instead of max visible bar for diff calculation";
		MinimalVapPercentageChangeThreshold.Name = "Percentage that VAP must change to trigger re-calc";
		MinimalVapPercentageChangeThreshold.SetFloat(0.30);
        return;
    }

	
	int currentIndex = sc.UpdateStartIndex;  // used to be sc.Index
	int lastIndex = sc.IndexOfLastVisibleBar;	
	
	log_message.Format("currentIndex=%d, lastIndex=%d, sc.Index=%d, sc.GetPersistentInt=%d", currentIndex, lastIndex, sc.Index, sc.GetPersistentInt(0));
	if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
	
	// calc bar ranges 
	float tmp_bar_diff = 0;
	float max_bar_diff = 0;
	float bar_diff_sums = 0;
	int lookback_interval = LookbackInterval.GetInt();
	
	log_message.Format("Heading into for loop, lastIndex=%d, lookback_interval=%d", lastIndex,  lookback_interval);
	if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
	
	for (int i=lastIndex - lookback_interval; i<=lastIndex; i++) {
		tmp_bar_diff = sc.BaseData[SC_HIGH][i] - sc.BaseData[SC_LOW][i];
		log_message.Format("lastIndex=%d, i=%d, tmp_bar_diff=%f", lastIndex, i, tmp_bar_diff);
		if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
		max_bar_diff = max(max_bar_diff, tmp_bar_diff);
		bar_diff_sums += tmp_bar_diff;
	}
		
	float magic_number = MagicNumber.GetFloat();
	int vap = sc.Round(max_bar_diff / magic_number);
	if (UseBarAverageInstead.GetInt() == 1) {
		vap = sc.Round((bar_diff_sums/lookback_interval) / magic_number);
	}

	
	// futures with tick sizes more than one penny will require us to adjust
	// the formula we use to calculate visually appealing VAP:
	if (sc.TickSize > 0.01) {
		vap = sc.Round(max_bar_diff * magic_number * sc.TickSize);
	}
	
	// safety check for setting VAP
	if (vap == 0) {
		vap = 1;
	}
	
	// prevent excessive VAP updates when change is minimal
	// Malykubo suggests 30% or more change to trigger a VAP change
	int vap_diff = sc.VolumeAtPriceMultiplier - vap;
	float vap_change_perc = (float)abs(vap_diff) / (float)sc.VolumeAtPriceMultiplier;
	log_message.Format("vap_diff=%d, perc change=%f", vap_diff, vap_change_perc);
	if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
	if (vap_change_perc < MinimalVapPercentageChangeThreshold.GetFloat()) {
		// don't update, vap change too small 
		return;  
	}
		
	// Change VAP only on change
	if (sc.VolumeAtPriceMultiplier != vap) {
		log_message.Format("curr vap=%d, new vap=%d", sc.VolumeAtPriceMultiplier, vap);
		if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
		
		sc.VolumeAtPriceMultiplier = vap;
		log_message.Format("VAP has been updated to %d", sc.VolumeAtPriceMultiplier);
		if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
	}
	
	log_message.Format("Idx to calc from: %d, Bar High: %f, Bar Low: %f, Max Bar Diff: %f, VAP: %d", lastIndex, sc.High[lastIndex], sc.Low[lastIndex], max_bar_diff, vap);
	if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
}

