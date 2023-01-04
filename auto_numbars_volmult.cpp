#include "sierrachart.h"
#include <string>
#include <vector>
SCDLLName("Frozen Tundra - Auto NumBars Volume Multiplier")

/*
    Written by Frozen Tundra
*/

SCSFExport scsf_AutoNumBarsVolMult(SCStudyInterfaceRef sc)
{
    SCString msg;
    SCInputRef i_TargetChart = sc.Input[1];
    SCInputRef i_NumBarCalc1 = sc.Input[2];
    SCInputRef i_NumBarCalc2 = sc.Input[3];

    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;
        sc.GraphShortName = "Auto NumBars VolMult";

        i_TargetChart.Name = "Select Number Bars Study";
        i_TargetChart.SetChartStudyValues(sc.ChartNumber,8);

        i_NumBarCalc1.Name = "Select NumBarCalc Delta Study";
        i_NumBarCalc1.SetChartStudyValues(sc.ChartNumber,3);

        i_NumBarCalc2.Name = "Select NumBarCalc Total Vol Study";
        i_NumBarCalc2.SetChartStudyValues(sc.ChartNumber,4);

        return;
    }

    // https://www.sierrachart.com/index.php?page=doc/ACSIL_Members_Functions.html#scSecurityType

    int BoldSize, Multiplier, ImbalanceMinimum;
    if (sc.Index == 0) {
        if (sc.SecurityType() == n_ACSIL::SECURITY_TYPE_STOCK) {
            // set numbars and numbars calc to .001 vol mult
            // NumberBars: input id#9 so zero based idx of 8
            // NumberBarsCalc: input id#31 so zero based idx of 30
            // value idx of 3 (0=1, 1=0.1, 2=0.01, 3=0.001)
            sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 8, 3);
            sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc1.GetStudyID(), 30, 3);
            sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc2.GetStudyID(), 30, 3);

            BoldSize = 100000;
            Multiplier = 1000;
            ImbalanceMinimum = 10000;
            sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 80, BoldSize);

            // set the Bid/Ask Min Volume Compare Threshold amount
            sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 94, ImbalanceMinimum);
        }
        else {

            // tenths
            if (
                    false
                    //sc.Symbol.Left(2) == "NQ"
                    ) {
                sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 8, 1);
                sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc1.GetStudyID(), 30, 1);
                sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc2.GetStudyID(), 30, 1);
                Multiplier = 10;
            }
            // hundredths
            else if (
                    //sc.Symbol.Left(2) == "ES"
                    //|| 
                    sc.Symbol.Left(2) == "ZB"
                    ) {
                sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 8, 2);
                sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc1.GetStudyID(), 30, 2);
                sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc2.GetStudyID(), 30, 2);
                Multiplier = 100;
            }
            // thousandths
            else if (
                    sc.Symbol.Left(2) == "ZN"
                    ) {
                sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 8, 3);
                sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc1.GetStudyID(), 30, 3);
                sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc2.GetStudyID(), 30, 3);
                Multiplier = 1000;
            }
            // unchanged
            else {
                sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 8, 0);
                sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc1.GetStudyID(), 30, 0);
                sc.SetChartStudyInputInt(sc.ChartNumber, i_NumBarCalc2.GetStudyID(), 30, 0);
                Multiplier = 1;
            }

            // 81 = bold, so 80 zero idx
            if (sc.Symbol.Left(2) == "ES") BoldSize = 1000;
            else if (sc.Symbol.Left(2) == "NQ") BoldSize = 500;
            else if (sc.Symbol.Left(2) == "CL") BoldSize = 15;
            else if (sc.Symbol.Left(2) == "GC") BoldSize = 30;
            else if (sc.Symbol.Left(2) == "ZB") BoldSize = 1000; // 30yr
            else if (sc.Symbol.Left(2) == "ZN") BoldSize = 5000; // 10yr
            else if (sc.Symbol.Left(2) == "ZF") BoldSize = 1000; // 5yr
            else if (sc.Symbol.Left(2) == "ZT") BoldSize = 900;  // 2yr

            sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 80, BoldSize);

            // set the Bid/Ask Min Volume Compare Threshold amount
            sc.SetChartStudyInputInt(sc.ChartNumber, i_TargetChart.GetStudyID(), 94, BoldSize);
        }

        // draw text explaining the settings in top left corner
        if (sc.SecurityType() != n_ACSIL::SECURITY_TYPE_STOCK) {
            s_UseTool Tool;
            Tool.ChartNumber = sc.ChartNumber;
            Tool.LineNumber = 4122022;
            Tool.DrawingType = DRAWING_STATIONARY_TEXT;
            Tool.UseRelativeVerticalValues = 1;
            Tool.BeginValue = 98;
            Tool.BeginDateTime = 2;
            Tool.AddMethod = UTAM_ADD_OR_ADJUST;
            Tool.LineWidth = 1;
            Tool.TransparencyLevel = 70;
            Tool.Region = sc.GraphRegion;
            Tool.FontSize = 10;
            Tool.FontBold = true;
            Tool.Text.Format("1/%d\n%d", Multiplier, BoldSize);
            Tool.Color = COLOR_YELLOW;
            sc.UseTool(Tool);
        }
    }
}
