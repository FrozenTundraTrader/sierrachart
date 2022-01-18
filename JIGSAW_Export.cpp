#include "helpers.h"
#include "sierrachart.h"

SCDLLName("JIGSAW Export")

/*
    Written by Malykubo and Frozen Tundra in FatCat's Discord Room
*/

const char *colors[147] = { "AliceBlue", "AntiqueWhite", "Aqua", "Aquamarine", "Azure", "Beige", "Bisque", "Black", "BlanchedAlmond", "Blue", "BlueViolet", "Brown", "BurlyWood", "CadetBlue", "Chartreuse", "Chocolate", "Coral", "CornflowerBlue", "Cornsilk", "Crimson", "Cyan", "DarkBlue", "DarkCyan", "DarkGoldenRod", "DarkGray", "DarkGrey", "DarkGreen", "DarkKhaki", "DarkMagenta", "DarkOliveGreen", "Darkorange", "DarkOrchid", "DarkRed", "DarkSalmon", "DarkSeaGreen", "DarkSlateBlue", "DarkSlateGray", "DarkSlateGrey", "DarkTurquoise", "DarkViolet", "DeepPink", "DeepSkyBlue", "DimGray", "DimGrey", "DodgerBlue", "FireBrick", "FloralWhite", "ForestGreen", "Fuchsia", "Gainsboro", "GhostWhite", "Gold", "GoldenRod", "Gray", "Grey", "Green", "GreenYellow", "HoneyDew", "HotPink", "IndianRed", "Indigo", "Ivory", "Khaki", "Lavender", "LavenderBlush", "LawnGreen", "LemonChiffon", "LightBlue", "LightCoral", "LightCyan", "LightGoldenRodYellow", "LightGray", "LightGrey", "LightGreen", "LightPink", "LightSalmon", "LightSeaGreen", "LightSkyBlue", "LightSlateGray", "LightSlateGrey", "LightSteelBlue", "LightYellow", "Lime", "LimeGreen", "Linen", "Magenta", "Maroon", "MediumAquaMarine", "MediumBlue", "MediumOrchid", "MediumPurple", "MediumSeaGreen", "MediumSlateBlue", "MediumSpringGreen", "MediumTurquoise", "MediumVioletRed", "MidnightBlue", "MintCream", "MistyRose", "Moccasin", "NavajoWhite", "Navy", "OldLace", "Olive", "OliveDrab", "Orange", "OrangeRed", "Orchid", "PaleGoldenRod", "PaleGreen", "PaleTurquoise", "PaleVioletRed", "PapayaWhip", "PeachPuff", "Peru", "Pink", "Plum", "PowderBlue", "Purple", "Red", "RosyBrown", "RoyalBlue", "SaddleBrown", "Salmon", "SandyBrown", "SeaGreen", "SeaShell", "Sienna", "Silver", "SkyBlue", "SlateBlue", "SlateGray", "SlateGrey", "Snow", "SpringGreen", "SteelBlue", "Tan", "Teal", "Thistle", "Tomato", "Turquoise", "Violet", "Wheat", "White", "WhiteSmoke", "Yellow", "YellowGreen" };

/*
 *
 * values to export:
 *
 * ovnH
 * ovnL
 * pdVWAP
 * pdStdD+
 * pdStdD-
 * dVWAP
 * dV+
 * dV-
 *
 */

// Log To File
/* target CSV format
 *  174'16,uPL,DarkGray
 *  154'21,mP,LightSalmon
 */
void logf(SCStudyInterfaceRef sc, SCString log_message) {
    unsigned int bytesWritten = 0;
    int fileHandle;
    int msgLength = 0;

    msgLength = log_message.GetLength() + 2;
    log_message = log_message + "\r\n";

    SCString filePath = "C:\\SierraChart\\sc_log.txt";
    sc.OpenFile(filePath, n_ACSIL::FILE_MODE_OPEN_TO_APPEND, fileHandle);
    sc.WriteFile(fileHandle, log_message, msgLength, &bytesWritten);
    sc.CloseFile(fileHandle);
}

SCSFExport scsf_JigsawExport(SCStudyInterfaceRef sc)
{

    SCString log_message;
    SCInputRef vwapStudyRef = sc.Input[0];
    SCInputRef timeInterval = sc.Input[1];


    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;

        vwapStudyRef.Name = "Select VWAP Study";
        vwapStudyRef.SetStudyID(0);

        timeInterval.Name = "Interval for recalculation in sec";
        timeInterval.SetInt(60);

        return;
    }

    Helper help(sc);

    // Recalculate every timeInterval sec
    SCDateTime& lastDateTime = sc.GetPersistentSCDateTime(0);
    if (sc.LatestDateTimeForLastBar.GetTimeInSeconds() < lastDateTime.GetTimeInSeconds() + timeInterval.GetInt()) {
        //return;
    }
    sc.SetPersistentSCDateTime(0, sc.LatestDateTimeForLastBar);

    int vwapValueIndex = 0;
    int vwapTopStdDevIndex = 1;
    int vwapBottomStdDevIndex = 2;

    SCFloatArray vwapValueStudyArray;
    //Get vwap value subgraph from the vwapStudyRef
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapValueIndex, vwapValueStudyArray) > 0 && vwapValueStudyArray.GetArraySize() > 0) {
        double lastValue = vwapValueStudyArray[sc.UpdateStartIndex];
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        log_message.Format("%s,dV,DarkGray", fmtValue.GetChars());
        help.dump(log_message);
        logf(sc, log_message);
    }

    SCFloatArray vwapTopStdDevStudyArray;
    //Get top std dev subgraph from the vwapStudyRef
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapTopStdDevIndex, vwapTopStdDevStudyArray) > 0 && vwapTopStdDevStudyArray.GetArraySize() > 0) {
        double lastValue = vwapTopStdDevStudyArray[sc.UpdateStartIndex];
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        //log_message.Format("TOP STD DEV: %f", lastValue);
        log_message.Format("%s,dV+,LightSalmon", fmtValue.GetChars());
        help.dump(log_message);
        logf(sc, log_message);
    }

    SCFloatArray vwapBottomStdDevStudyArray;
    //Get bottom std dev subgraph from the vwapStudyRef
    if (sc.GetStudyArrayUsingID(vwapStudyRef.GetStudyID(), vwapBottomStdDevIndex, vwapBottomStdDevStudyArray) > 0 && vwapBottomStdDevStudyArray.GetArraySize() > 0) {
        float lastValue = vwapBottomStdDevStudyArray[sc.UpdateStartIndex];
        SCString fmtValue = sc.FormatGraphValue(lastValue, sc.BaseGraphValueFormat);
        //log_message.Format("BOTTOM STD DEV: %f", lastValue);
        log_message.Format("%s,dV-,DeepPink", fmtValue.GetChars());
        help.dump(log_message);
        logf(sc, log_message);
    }

}
