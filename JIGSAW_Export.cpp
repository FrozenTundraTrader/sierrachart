#include "helpers.h"
#include "sierrachart.h"

SCDLLName("JIGSAW Export")

/*
    Written by Malykubo and Frozen Tundra in FatCat's Discord Room
*/


/*
 * Pardon's values to export:
 *
 * ovnH - DarkSeaGreen
 * ovnL - IndianRed
 * pdVWAP - ForestGreen
 * pdStdD+ - SeaGreen
 * pdStdD- - SeaGreen
 * dVWAP - Cyan
 * DV+ - MediumTurquoise
 * DV- - MediumTurquoise
 * psVWAP - HotPink
 * psDV+ - LightPink
 * psDV- LightPink
 * dEQ - FloralWhite
 * wEQ - Cornsilk
 * mEQ - LemonChiffon
 *
 */

// Log To File
/* target CSV format
 *  174'16,uPL,DarkGray
 *  154'21,mP,LightSalmon
 */
// pass in sc obj and string to log to file
void logf(SCStudyInterfaceRef sc, SCString log_message, bool restartFile) {
    unsigned int bytesWritten = 0;
    int fileHandle;
    int msgLength = 0;

    // add 2 bytes to the end for the newline and carriage return characters
    msgLength = log_message.GetLength() + 2;
    log_message = log_message + "\r\n";

    // file handle writing enums (scconstants.h)
    // FILE_MODE_OPEN_TO_REWRITE_FROM_START
    // FILE_MODE_OPEN_TO_APPEND
    SCString filePath;
    filePath.Format("C:\\SierraChart\\jigsaw_%s.txt", sc.Symbol.GetChars());
    if (restartFile) {
        sc.OpenFile(filePath, n_ACSIL::FILE_MODE_OPEN_TO_REWRITE_FROM_START, fileHandle);
    }
    else {
        // continue appending to file
        sc.OpenFile(filePath, n_ACSIL::FILE_MODE_OPEN_TO_APPEND, fileHandle);
    }
    sc.WriteFile(fileHandle, log_message, msgLength, &bytesWritten);
    sc.CloseFile(fileHandle);
}

SCSFExport scsf_JigsawExport(SCStudyInterfaceRef sc)
{

    SCString log_message;
    SCInputRef timeInterval = sc.Input[0];
    SCInputRef vwapStudyRef = sc.Input[1]; // dVWAP, dv+, dv-
    SCInputRef ovnHref = sc.Input[2]; // ovnH
    SCInputRef ovnLref = sc.Input[3]; // ovnL
    SCInputRef pdVwapRef = sc.Input[4]; // pdVWAP
    SCInputRef pdDevHighRef = sc.Input[5]; // pdStdD+
    SCInputRef pdDevLowRef = sc.Input[6]; // pdStdD-
    SCInputRef psVwapRef = sc.Input[7]; // psVWAP
    SCInputRef psDevHighRef = sc.Input[8]; // psDV+
    SCInputRef psDevLowRef = sc.Input[9]; // psDV-
    SCInputRef dailyEqRef = sc.Input[10]; // dEQ
    SCInputRef weeklyEqRef = sc.Input[11]; // wEQ
    SCInputRef monthlyEqRef = sc.Input[12]; // mEQ
    Helper help(sc);

    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;

        timeInterval.Name = "Interval for recalculation in sec";
        timeInterval.SetInt(60);

        vwapStudyRef.Name = "Select Daily VWAP Study";
        vwapStudyRef.SetStudyID(0);

        ovnHref.Name = "Select ovnH";
        ovnHref.SetStudySubgraphValues(0,0);

        ovnLref.Name = "Select ovnL";
        ovnLref.SetStudySubgraphValues(0,0);

        pdVwapRef.Name = "Select pdVWAP";
        pdVwapRef.SetStudySubgraphValues(0,0);

        pdDevHighRef.Name = "Select pdStdD+";
        pdDevHighRef.SetStudySubgraphValues(0,0);

        pdDevLowRef.Name = "Select pdStdD-";
        pdDevLowRef.SetStudySubgraphValues(0,0);

        psVwapRef.Name = "Select psVWAP";
        psVwapRef.SetStudySubgraphValues(0,0);

        psDevHighRef.Name = "Select psStdD+";
        psDevHighRef.SetStudySubgraphValues(0,0);

        psDevLowRef.Name = "Select psStdD-";
        psDevLowRef.SetStudySubgraphValues(0,0);

        dailyEqRef.Name = "Select Daily EQ Study";
        dailyEqRef.SetStudySubgraphValues(0,0);

        weeklyEqRef.Name = "Select Weekly EQ Study";
        weeklyEqRef.SetStudySubgraphValues(0,0);

        monthlyEqRef.Name = "Select Monthly EQ Study";
        monthlyEqRef.SetStudySubgraphValues(0,0);

        return;
    }

    // Recalculate every timeInterval sec
    SCDateTime& lastDateTime = sc.GetPersistentSCDateTime(0);
    if (sc.LatestDateTimeForLastBar.GetTimeInSeconds() < lastDateTime.GetTimeInSeconds() + timeInterval.GetInt()) {
        return;
    }
    sc.SetPersistentSCDateTime(0, sc.LatestDateTimeForLastBar);

    int vwapValueIndex = 0;
    int vwapTopStdDevIndex = 1;
    int vwapBottomStdDevIndex = 2;

    SCFloatArray vwapValueStudyArray;
    //Get vwap value subgraph from the vwapStudyRef
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapValueIndex, vwapValueStudyArray) > 0 && vwapValueStudyArray.GetArraySize() > 0) {
        double lastValue = vwapValueStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,dV,Cyan", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, true);
    }

    SCFloatArray vwapTopStdDevStudyArray;
    //Get top std dev subgraph from the vwapStudyRef
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapTopStdDevIndex, vwapTopStdDevStudyArray) > 0 && vwapTopStdDevStudyArray.GetArraySize() > 0) {
        double lastValue = vwapTopStdDevStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        //log_message.Format("TOP STD DEV: %f", lastValue);
        log_message.Format("%s,dV+,MediumTurquoise", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray vwapBottomStdDevStudyArray;
    //Get bottom std dev subgraph from the vwapStudyRef
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapBottomStdDevIndex, vwapBottomStdDevStudyArray) > 0 && vwapBottomStdDevStudyArray.GetArraySize() > 0) {
        float lastValue = vwapBottomStdDevStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        //log_message.Format("BOTTOM STD DEV: %f", lastValue);
        log_message.Format("%s,dV-,MediumTurquoise", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray ovnHStudyArray;
    //Get values subgraph from the ovnHref
    if (sc.GetStudyArrayUsingID(ovnHref.GetStudyID(), ovnHref.GetSubgraphIndex(), ovnHStudyArray) > 0 && ovnHStudyArray.GetArraySize() > 0) {
        double lastValue = ovnHStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,ovnH,DarkSeaGreen", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray ovnLStudyArray;
    //Get values subgraph from the ovnLref
    if (sc.GetStudyArrayUsingID(ovnLref.GetStudyID(), ovnLref.GetSubgraphIndex(), ovnLStudyArray) > 0 && ovnLStudyArray.GetArraySize() > 0) {
        double lastValue = ovnLStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,ovnL,IndianRed", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray pdVwapStudyArray;
    //Get values subgraph from the pdVwapRef
    if (sc.GetStudyArrayUsingID(pdVwapRef.GetStudyID(), pdVwapRef.GetSubgraphIndex(), pdVwapStudyArray) > 0 && pdVwapStudyArray.GetArraySize() > 0) {
        double lastValue = pdVwapStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,pdVWAP,ForestGreen", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray pdDevHighStudyArray;
    //Get values subgraph from the pdDevHighRef
    if (sc.GetStudyArrayUsingID(pdDevHighRef.GetStudyID(), pdDevHighRef.GetSubgraphIndex(), pdDevHighStudyArray) > 0 && pdDevHighStudyArray.GetArraySize() > 0) {
        double lastValue = pdDevHighStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,pdStdD+,SeaGreen", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray pdDevLowStudyArray;
    //Get values subgraph from the pdDevLowRef
    if (sc.GetStudyArrayUsingID(pdDevLowRef.GetStudyID(), pdDevLowRef.GetSubgraphIndex(), pdDevLowStudyArray) > 0 && pdDevLowStudyArray.GetArraySize() > 0) {
        double lastValue = pdDevLowStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,pdStdD-,SeaGreen", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray psVWAPStudyArray;
    //Get values subgraph from the psVwapRef
    if (sc.GetStudyArrayUsingID(psVwapRef.GetStudyID(), psVwapRef.GetSubgraphIndex(), psVWAPStudyArray) > 0 && psVWAPStudyArray.GetArraySize() > 0) {
        double lastValue = psVWAPStudyArray[sc.Index];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,psVWAP,HotPink", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray psDevHighStudyArray;
    //Get values subgraph from the psDevHighRef
    if (sc.GetStudyArrayUsingID(psDevHighRef.GetStudyID(), psDevHighRef.GetSubgraphIndex(), psDevHighStudyArray) > 0 && psDevHighStudyArray.GetArraySize() > 0) {
        double lastValue = psDevHighStudyArray[sc.Index];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,psStdD+,LightPink", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray psDevLowStudyArray;
    //Get values subgraph from the psDevLowRef
    if (sc.GetStudyArrayUsingID(psDevLowRef.GetStudyID(), psDevLowRef.GetSubgraphIndex(), psDevLowStudyArray) > 0 && psDevLowStudyArray.GetArraySize() > 0) {
        double lastValue = psDevLowStudyArray[sc.Index];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,psStdD-,LightPink", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray dailyEqStudyArray;
    //Get values subgraph from the dailyEqRef
    if (sc.GetStudyArrayUsingID(dailyEqRef.GetStudyID(), dailyEqRef.GetSubgraphIndex(), dailyEqStudyArray) > 0 && dailyEqStudyArray.GetArraySize() > 0) {
        double lastValue = dailyEqStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,dEQ,FloralWhite", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray weeklyEqStudyArray;
    //Get values subgraph from the weeklyEqRef
    if (sc.GetStudyArrayUsingID(weeklyEqRef.GetStudyID(), weeklyEqRef.GetSubgraphIndex(), weeklyEqStudyArray) > 0 && weeklyEqStudyArray.GetArraySize() > 0) {
        double lastValue = dailyEqStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,wEQ,Cornsilk", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

    SCFloatArray monthlyEqStudyArray;
    //Get values subgraph from the monthlyEqRef
    if (sc.GetStudyArrayUsingID(monthlyEqRef.GetStudyID(), monthlyEqRef.GetSubgraphIndex(), monthlyEqStudyArray) > 0 && monthlyEqStudyArray.GetArraySize() > 0) {
        double lastValue = monthlyEqStudyArray[sc.UpdateStartIndex];
        lastValue = sc.RoundToTickSize((float)lastValue,sc.TickSize);
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,mEQ,LemonChiffon", fmtValue.GetChars());
        //help.dump(log_message);
        logf(sc, log_message, false);
    }

}

