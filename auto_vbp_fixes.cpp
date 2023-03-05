#include "sierrachart.h"

SCDLLName("Frozen Tundra - Auto Volume by Price")

/*
    Written by Malykubo and Frozen Tundra in FatCat's Discord Room
*/

SCSFExport scsf_AutoVbP(SCStudyInterfaceRef sc)
{
	SCInputRef useVap = sc.Input[0];
    // multiplier that will determine the granularity of VP bars we'll see as we switch symbols
    // the lower the number, the thicker and fewer bars there will be
    // the higher the magic number, the thinner and more VP bars there will be
    SCInputRef i_DetailLevel = sc.Input[1];
	SCInputRef i_Step = sc.Input[2];
	
	//number of inputs to set target VBP studies
	const int MAX_VBP_STUDIES = 20;
	
    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;
		sc.GraphName = "Auto set VbPs ticks per bar";
		sc.AutoLoop = 0; 
		//update always so we don't have to wait for incoming ticks (market closed)
		sc.UpdateAlways = 1;
		
		//INPUTS CONFIG
		useVap.Name = "Use VAP setting in Chart Settings";
        useVap.SetYesNo(0);
        i_DetailLevel.Name = "VbPs' detail level";
        i_DetailLevel.SetInt(100);
		i_Step.Name = "Tick-size step to increase/decrease detail";
		i_Step.SetInt(4);
		
		for(int x = 0; x < MAX_VBP_STUDIES; x++)
		{
			sc.Input[10 + x].Name.Format("%i.Target VbP study", x + 1);
			sc.Input[10 + x].SetStudyID(0);
		}
		
        return;
    }
	
	if(sc.IsFullRecalculation)
	{
		//set step to the nearest multiplier of sc.VolumeAtPriceMultiplier to avoid problems
		int step = i_Step.GetInt();
		i_Step.SetInt(step - step % sc.VolumeAtPriceMultiplier);
	}
	
	// VbP Ticks Per Volume Bar is input 32, ID 31
    int inputIdx = 31;

	//if setting VbP's Ticks-per-bar according to Volume At Price Multiplier in ChartSettings
    if (useVap.GetInt() == 1) {
		
		for(int x = 0; x < MAX_VBP_STUDIES; x++)
		{
			int studyId = sc.Input[10 + x].GetStudyID();
			if(studyId != 0)
			{
				int prevValue;
				sc.GetChartStudyInputInt(sc.ChartNumber, studyId, inputIdx, prevValue); 			
				if(prevValue != sc.VolumeAtPriceMultiplier)
					sc.SetChartStudyInputInt(sc.ChartNumber, studyId, inputIdx, sc.VolumeAtPriceMultiplier);
			}
		}
		
        return;
    }
	
	//if using detail level specified in inputs
    float vHigh, vLow, vDiff;
    int detail = i_DetailLevel.GetInt();

    // fetch the graph's price scale's high and low value so we can automate the Ticks setting on VbP
    sc.GetMainGraphVisibleHighAndLow(vHigh, vLow);

    // calc the range of visible prices
	//DIVIDE BY TICK-SIZE TO GET NUMBER OF VISIBLE TICKS
    vDiff = (vHigh - vLow) / sc.TickSize;

    // divide range by detail level to get the desired VbP Ticks Per Bar value and don't allow it to be less than the VAP multiplier
    int targetTicksPerBar = max(sc.Round(vDiff / detail), sc.VolumeAtPriceMultiplier);
	
	// adapt targetTicksPerBar to the specified step
	if(targetTicksPerBar >= i_Step.GetInt() )
		targetTicksPerBar -= targetTicksPerBar % i_Step.GetInt();
	//if targetTicksPerBar is less than the step but greater than VAP multiplier, then modify it as a VAP multiplier multiple
	else if(targetTicksPerBar > sc.VolumeAtPriceMultiplier)
		targetTicksPerBar -= targetTicksPerBar % sc.VolumeAtPriceMultiplier;
	
	//flag to redraw chart in case we change any target-VbP
	bool redraw = false;
	
	for(int x = 0; x < MAX_VBP_STUDIES; x++)
	{
		int studyId = sc.Input[10 + x].GetStudyID();
		if(studyId != 0)
		{
			int prevValue;
			sc.GetChartStudyInputInt(sc.ChartNumber, studyId, inputIdx, prevValue);

			if(targetTicksPerBar != prevValue) 
			{
				sc.SetChartStudyInputInt(sc.ChartNumber, studyId, inputIdx, targetTicksPerBar);
				redraw = true;
			}
		}
	}
	
	if(redraw) sc.RecalculateChart(sc.ChartNumber);
}
