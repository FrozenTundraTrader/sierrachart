#include "sierrachart.h"

SCDLLName("Frozen Tundra Discord Room Studies")

/*
	Written by Malykubo and Frozen Tundra in FatCat's Discord Room
*/

SCSFExport scsf_ChangeVolAtPriceMult(SCStudyInterfaceRef sc)
{
	
	SCInputRef DebugOn = sc.Input[0];
	SCInputRef CalculateWhileScrolling = sc.Input[1];
	SCInputRef LookbackInterval = sc.Input[2];
	SCInputRef MagicNumber = sc.Input[3];
	SCString log_message;

	// keys for persistent dictionaries
	int lastIndexKey = 0;
	int calculateWhileScrollingKey = 1;
	int lookbackIntervalKey = 2;
	int magicNumberKey = 3;
	
    // Configuration
    if (sc.SetDefaults)
    {
		sc.GraphRegion = 0;
		DebugOn.Name = "Debug Enabled?";
		DebugOn.SetYesNo(0);

		CalculateWhileScrolling.Name = "Calculate while scrolling?";
		CalculateWhileScrolling.SetYesNo(1);
		sc.SetPersistentInt(calculateWhileScrollingKey, 1);

		LookbackInterval.Name = "Lookback Interval # Bars";
		LookbackInterval.SetInt(6);
		sc.SetPersistentInt(lookbackIntervalKey, 6);

		MagicNumber.Name = "Magic Multiplier (ticks)";
		MagicNumber.SetFloat(0.3);
		sc.SetPersistentFloat(magicNumberKey, 0.3);

        return;
    }

	int lastIndex;
	int &lastIndexProcessed = sc.GetPersistentInt(lastIndexKey);

	// Don't process on startups nor recalculations. 
	if (sc.UpdateStartIndex == 0) { 
		return; 
	}

	// set lastIndex according to last or visible bar
	if (CalculateWhileScrolling.GetInt() == 1) {
		sc.UpdateAlways = 1;
		lastIndex = sc.IndexOfLastVisibleBar;
	} else {
		sc.UpdateAlways = 0;
		// UpdateStartIndex has to be used when autoloop is set to 0 (default value)
		lastIndex = sc.UpdateStartIndex;
	}

	}
	
	log_message.Format("currentIndex=%d, lastIndex=%d, sc.Index=%d, sc.GetPersistentInt=%d", currentIndex, lastIndex, sc.Index, sc.GetPersistentInt(0));
	if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
	
	// calc bar ranges 
	float bar_diff_sums = 0;
	float tmp_bar_diff = 0;
	int lookback_interval = LookbackInterval.GetInt();
	
	log_message.Format("Heading into for loop, lastIndex=%d, lookback_interval=%d", lastIndex,  lookback_interval);
	if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
	
	for (int i=lastIndex - lookback_interval; i<lastIndex; i++) {
		tmp_bar_diff = sc.BaseData[SC_HIGH][i] - sc.BaseData[SC_LOW][i];
		log_message.Format("lastIndex=%d, i=%d, bar_diff=%f", lastIndex, i, tmp_bar_diff);
		if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
		bar_diff_sums += tmp_bar_diff;
	}
		
	float bar_diff = bar_diff_sums / lookback_interval;
	float magic_number = MagicNumber.GetFloat();
	int vap = sc.Round(bar_diff / magic_number);
	
	// futures with tick sizes more than one penny will require us to adjust
	// the formula we use to calculate visually appealing VAP:
	if (sc.TickSize > 0.01) {
		vap = sc.Round(bar_diff * magic_number * sc.TickSize);
	}
	
	// safety check for setting VAP
	if (vap == 0) {
		vap = 1;
	}
		
	// Change VAP only on change
	if (sc.VolumeAtPriceMultiplier != vap) {
		log_message.Format("curr vap=%d, new vap=%d", sc.VolumeAtPriceMultiplier, vap);
		if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
		
		sc.VolumeAtPriceMultiplier = vap;
		log_message.Format("VAP has been updated to %d", sc.VolumeAtPriceMultiplier);
		if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
	}
	
	log_message.Format("Idx to calc from: %d, Bar High: %f, Bar Low: %f, Bar Diff: %f, VAP: %d", lastIndex, sc.High[lastIndex], sc.Low[lastIndex], bar_diff, vap);
	if (DebugOn.GetInt() == 1) sc.AddMessageToLog(log_message, 1);
}

