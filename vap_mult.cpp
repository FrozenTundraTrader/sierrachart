#include "sierrachart.h"

SCDLLName("Frozen Tundra Discord Room Studies")

SCSFExport scsf_ChangeVolAtPriceMult(SCStudyInterfaceRef sc)
{
	
	SCInputRef DebugOn = sc.Input[0];
	SCInputRef CalculateWhileScrolling = sc.Input[1];
	SCInputRef LookbackInterval = sc.Input[2];
	SCInputRef MagicNumber = sc.Input[3];

    // Configuration
    if (sc.SetDefaults)
    {
		sc.GraphRegion = 0;
		DebugOn.Name = "Debug Enabled?";
		DebugOn.SetYesNo(0);
		CalculateWhileScrolling.Name = "Calculate while scrolling?";
		CalculateWhileScrolling.SetYesNo(1);
		LookbackInterval.Name = "Lookback Interval # Bars";
		LookbackInterval.SetInt(5);
		MagicNumber.Name = "Magic Multiplier (ticks)";
		MagicNumber.SetFloat(0.25);
        return;
    }
	
	int lastIndex;
	// set lastIndex according to last or visible bar
	if (CalculateWhileScrolling.GetInt() == 1) {
		lastIndex = sc.IndexOfLastVisibleBar;
	} else {
		lastIndex = sc.Index;
	}
	int &lastIndexProcessed = sc.GetPersistentInt(0); 

	// Don't process on startups nor recalculations. 
	if (lastIndex == 0) { 
		lastIndexProcessed = -1; 
		return; 
	} 
	
	// Process only when lastIndex changed
	if (lastIndex == lastIndexProcessed) { 
		return;
	}
	
	// calc bar ranges 
	float bar_diff_sums = 0;
	float tmp_bar_diff = 0;
	int lookback_interval = LookbackInterval.GetInt();
	for (int i=lastIndex; i>lastIndex - lookback_interval; i--) {
		tmp_bar_diff = sc.BaseData[SC_HIGH][i] - sc.BaseData[SC_LOW][i];
		bar_diff_sums += tmp_bar_diff;
	}
		
	float bar_diff = bar_diff_sums / lookback_interval;
	float magic_number = MagicNumber.GetFloat();
	int vap = sc.Round(bar_diff / magic_number);
	
	// safety check for setting VAP
	if (vap == 0) {
		vap = 1;
	}
	
	// Change VAP only on change
	if (sc.VolumeAtPriceMultiplier != vap) {
		sc.VolumeAtPriceMultiplier = vap;
	}
	
	// update last proc'd idx
	lastIndexProcessed = lastIndex; 

	// debug
	if (DebugOn.GetInt() == 1) {
		SCString log_message;
		//log_message.Format("Target Volume At Price Multipler: %d", magic_number);
		//sc.AddMessageToLog(log_message, 1);
		
		log_message.Format("Idx to calc from: %d, Bar High: %f, Bar Low: %f, Bar Diff: %f, VAP: %d", idx_to_calc_from, sc.High[idx_to_calc_from], sc.Low[idx_to_calc_from], bar_diff, vap);
		sc.AddMessageToLog(log_message, 1);
	}
}
