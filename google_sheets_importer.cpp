#include "sierrachart.h"
#include <string>
#include <sstream>
#include <vector>
SCDLLName("Frozen Tundra - Google Sheets Levels Importer")

/*
    Written by Frozen Tundra

    This study allows you to enter price values into a google docs spreadsheet
    and automatically draw those levels onto your Sierra chart window.

    The format of the Google Spreadsheet is as follows:
        (float)  Price
        (float)  Price 2 (For Rectangles only)
        (string) Note
        (string) Color
        (int)    Line Type
        (int)    Line Width
        (int)    Text Alignment
*/

// simple struct to hold bare minimum data from google sheets
// we will populate a vector of these and pass to the WinGDI function for drawing
struct PriceLabel {
    float Price;
    SCString Label;
    COLORREF LabelColor;
};

// WinGDI draw function definition
void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc);

SCSFExport scsf_GoogleSheetsLevelsImporter(SCStudyInterfaceRef sc)
{
    // logging object
    SCString msg;

    // General settings
    SCInputRef i_FilePath = sc.Input[0];

    // Chart related settings
    SCInputRef i_Transparency = sc.Input[1];
    SCInputRef i_DrawLinesOnChart = sc.Input[3];
    SCInputRef i_ShowPriceOnChart = sc.Input[4];

    // DOM related settings
    SCInputRef i_ShowLabelOnDom = sc.Input[5];
    SCInputRef i_DomFontSize = sc.Input[6];
    SCInputRef i_xOffset = sc.Input[7];
    SCInputRef i_yOffset = sc.Input[8];
    SCInputRef i_LabelBgColor = sc.Input[9];

    // recalc interval input
    // 0 = off
    // 1 = recalc every 1 min
    // 5 = recalc every 5 min...
    SCInputRef i_RecalcInterval = sc.Input[10];

    // Set configuration variables
    if (sc.SetDefaults)
    {
        sc.GraphName = "Google Sheets Importer";
        sc.GraphRegion = 0;

        i_FilePath.Name = "Google Sheets URL";
        // example template:
        // https://docs.google.com/spreadsheets/d/1fx_Ym6QodEJIqQO90HR1dWYFw5RoK2w5ywtHeMlMeZY/gviz/tq?tqx=out:csv
        // Google Docs Spreadsheet
        // REQUIRED:
        //    - you must set SHARING privs to "Anyone with link"
        //    - you must strip off anything after the unique key, for example "/edit#usp=sharing" **needs** to be removed
        i_FilePath.SetString("https://docs.google.com/spreadsheets/d/1fx_Ym6QodEJIqQO90HR1dWYFw5RoK2w5ywtHeMlMeZY");

        i_Transparency.Name = "Transparency Level";
        i_Transparency.SetInt(70);

        // Chart specific settings
        i_DrawLinesOnChart.Name = "Draw Lines On Chart?";
        i_DrawLinesOnChart.SetYesNo(1);

        i_ShowPriceOnChart.Name = "> Show Price By Chart Label?";
        i_ShowPriceOnChart.SetYesNo(0);


        // DOM specific settings
        i_ShowLabelOnDom.Name = "Show Label On DOM?";
        i_ShowLabelOnDom.SetYesNo(0);

        i_DomFontSize.Name = " > DOM Font Size";
        i_DomFontSize.SetInt(18);

        i_xOffset.Name = " > DOM Label X-coord Offset";
        i_xOffset.SetInt(-70);

        i_yOffset.Name = " > DOM Label Y-coord Offset";
        i_yOffset.SetInt(-10);

        i_LabelBgColor.Name = " > DOM Label Background Color";
        i_LabelBgColor.SetColor(255,255,255);

        i_RecalcInterval.Name = "Recalculation Interval (0=off, 5=5min)";
        i_RecalcInterval.SetInt(0);

        return;
    }

    // TODO: only perform recalc automatically if interval set to > 0

    // calculate the last time we ran this study
    int RecalcIntervalMin = i_RecalcInterval.GetInt();
    int RecalcIntervalSec = RecalcIntervalMin * 60;

    int &LastUpdated = sc.GetPersistentInt(9);

    // grab actual current time
    SCDateTime Now = sc.CurrentSystemDateTime;
    // convert time into flat number, into seconds (minutes/hours/whatever)
    int TimeInSec = Now.GetTimeInSeconds();
    // perform our check if enough time has elapsed to do a recalc
    if (LastUpdated > 0 && LastUpdated + RecalcIntervalSec > TimeInSec && sc.Index != 0) {
        // we have not elapsed enough time, do not recalc
        return;
    }

    // update our marker for when we last updated
    LastUpdated = TimeInSec;


    // hold our HTTP Response
    SCString &HttpResponseContent = sc.GetPersistentSCString(4);

    SCString Url = i_FilePath.GetString();
    // add specific code to output as CSV attachment from Google Sheets
    Url.Format("%s/gviz/tq?tqx=out:csv", Url.GetChars());

    // IMPORTANT
    // create pointer to struct array
    // we need this vector to persist so we can pass it to the WinGDI function
    std::vector<PriceLabel>* p_PriceLabels = reinterpret_cast<std::vector<PriceLabel>*>(sc.GetPersistentPointer(0));
    if (p_PriceLabels == NULL) {
        // array of structs to hold our CSV labels for each price
        p_PriceLabels = new std::vector<PriceLabel>;
        sc.SetPersistentPointer(0, p_PriceLabels);
    }
    else {
        // clear old levels so we can recalc
        p_PriceLabels->clear();
    }

    // HTTP request start
    // status codes
    enum {REQUEST_NOT_SENT = 0,  REQUEST_SENT, REQUEST_RECEIVED};
    // latest request status
    int& RequestState = sc.GetPersistentInt(1);

    // Only run on full recalc OR if recalc interval is set
    if (sc.Index == 0 || RecalcIntervalMin > 0) {
        if (RequestState == REQUEST_NOT_SENT)
        {
            // Make a request for a text file on the server. When the request is complete and all of the data
            //has been downloaded, this study function will be called with the file placed into the sc.HTTPResponse character string array.

            if (!sc.MakeHTTPRequest(Url))
            {
                sc.AddMessageToLog("Error making HTTP request.", 1);

                // Indicate that the request was not sent. 
                // Therefore, it can be made again when sc.Index equals 0.
                RequestState = REQUEST_NOT_SENT;
            }
            else
                RequestState = REQUEST_SENT;
        }
    }

    if (RequestState == REQUEST_SENT && sc.HTTPResponse != "")
    {
        RequestState = REQUEST_RECEIVED;
        // Display the response from the Web server in the Message Log
        //sc.AddMessageToLog(sc.HTTPResponse, 1);
        HttpResponseContent = sc.HTTPResponse;
    }
    else if (RequestState == REQUEST_SENT && sc.HTTPResponse == "")
    {
        //The request has not completed, therefore there is nothing to do so we will return
        return;
    }

    // reset state for next run
    RequestState = REQUEST_NOT_SENT;

    // we'll split each CSV row up into tokens
    std::vector<char*> tokens;
    // open an input stream and read from our Google Sheet
    std::istringstream input(HttpResponseContent.GetChars());
    int LineNumber = 1;
    for (std::string line; getline(input,line);) {
        msg.Format("%s", line.c_str());
        //sc.AddMessageToLog(msg,1);

        SCString scline = line.c_str();
        // skip the opening quotes and end quotes
        scline = scline.GetSubString(scline.GetLength() - 2, 1);
        // anything between quotes and commas
        scline.Tokenize("\",\"", tokens);

        s_UseTool Tool;
        Tool.LineStyle = LINESTYLE_SOLID;
        Tool.LineWidth = 1;
        Tool.TextAlignment = DT_RIGHT;
        int idx = 1;
        // used for lines and rectangles
        float price;
        // only used for rectangles
        float price2 = 0;
        SCString note;
        SCString color;
        int linewidth = 1;
        int textalignment = 1;
        for (char* i : tokens) {
            //msg.Format("%s", i);
            //sc.AddMessageToLog(msg,1);

            // price
            if (idx == 1) {
                price = atof(i);
            }
            // price 2 (only used for rectangles)
            else if (idx == 2) {
                price2 = atof(i);
                if (price2 == 0) {
                    Tool.DrawingType = DRAWING_HORIZONTALLINE;
                    Tool.BeginValue = price;
                    Tool.EndValue = price;
                }
                else {
                    Tool.DrawingType = DRAWING_RECTANGLE_EXT_HIGHLIGHT;
                    Tool.BeginValue = price;
                    Tool.EndValue = price2;
                }
            }
            // note label
            else if (idx == 3) {
                note = i;
            }
            // color
            else if (idx == 4) {
                color = i;

                // could use a switch here, but most will find this easier to read:
                if (color == "red") Tool.Color = COLOR_RED;
                else if (color == "green") Tool.Color = COLOR_GREEN;
                else if (color == "blue") Tool.Color = COLOR_BLUE;
                else if (color == "white") Tool.Color = COLOR_WHITE;
                else if (color == "black") Tool.Color = COLOR_BLACK;
                else if (color == "purple") Tool.Color = COLOR_PURPLE;
                else if (color == "pink") Tool.Color = COLOR_PINK;
                else if (color == "yellow") Tool.Color = COLOR_YELLOW;
                else if (color == "gold") Tool.Color = COLOR_GOLD;
                else if (color == "brown") Tool.Color = COLOR_BROWN;
                else if (color == "cyan") Tool.Color = COLOR_CYAN;
                else if (color == "gray") Tool.Color = COLOR_GRAY;
                else Tool.Color = COLOR_WHITE;

                // if drawing a rectangle, make the fill color same as border
                if (price2 > 0) Tool.SecondaryColor = Tool.Color;
            }
            // line type
            else if (idx == 5) {
                int LineType = atoi(i);
                if (LineType == 0) Tool.LineStyle = LINESTYLE_SOLID;
                else if (LineType == 1) Tool.LineStyle = LINESTYLE_DASH;
                else if (LineType == 2) Tool.LineStyle = LINESTYLE_DOT;
                else if (LineType == 3) Tool.LineStyle = LINESTYLE_DASHDOT;
                else if (LineType == 4) Tool.LineStyle = LINESTYLE_DASHDOTDOT;
            }
            // line width
            else if (idx == 6) {
                linewidth = atoi(i);
                if (linewidth > 0) Tool.LineWidth = linewidth;
            }
            // text alignment
            else if (idx == 7) {
                textalignment = atoi(i);
                if (textalignment > 0) Tool.TextAlignment = textalignment;
            }

            // draw line
            if (i_DrawLinesOnChart.GetInt() == 1) {
                Tool.ChartNumber = sc.ChartNumber;
                Tool.BeginDateTime = sc.BaseDateTimeIn[0];
                Tool.EndDateTime = sc.BaseDateTimeIn[sc.ArraySize-1];
                Tool.AddMethod = UTAM_ADD_OR_ADJUST;
                Tool.ShowPrice = i_ShowPriceOnChart.GetInt();
                Tool.TransparencyLevel = i_Transparency.GetInt();
                //Tool.Text = note;
                //Tool.Text.Format("%s\nnewline test", note.GetChars());

                // look for newline characters in the CSV file, replace them with actual new lines
                // store note in this string obj
                std::string NoteStr;
                NoteStr = note.GetChars();

                // store an escaped newline string literal here, for comparison
                std::string NewLine = "\\n";

                // position where the newline exists in the haystack
                std::size_t IdxOfNewLine;

                // pointer to where newline exists in haystack
                char * p_substr = strstr(NoteStr.c_str(), NewLine.c_str());
                if (p_substr != NULL) {
                    // find index of newline
                    IdxOfNewLine = NoteStr.find(NewLine);

                    // copy in an actual newline where string literal newline was
                    strncpy(p_substr, "\n", 1);

                    // actual newline is only 1 char long, the string literal version is 2 chars
                    // we need to erase the `n` from the string after where the `\` was
                    NoteStr.erase(IdxOfNewLine + 1, 1);
                }
                Tool.Text.Format("%s", NoteStr.c_str());
                Tool.LineNumber = LineNumber;
                sc.UseTool(Tool);
            }

            // increment field counter
            idx++;
        }

        // increment row counter
        LineNumber++;

        // if we want to show on DOM, we need to add to our list
        if (i_ShowLabelOnDom.GetInt() == 1 && price > 0 && note != "") {
            PriceLabel TmpPriceLabel = { price, note, Tool.Color };
            if (price2 > 0) {
                // RECTANGLE only
                TmpPriceLabel.Label.Format("1.%s", note.GetChars());
            }
            //PriceLabels.insert(PriceLabels.end(), TmpPriceLabel);
            p_PriceLabels->insert(p_PriceLabels->end(), TmpPriceLabel);
//msg.Format("Adding %f => %s", price, note.GetChars());
//sc.AddMessageToLog(msg, 1);

            if (price2 > 0) {
                // RECTANGLE only
                TmpPriceLabel = { price2, note, Tool.Color };
                TmpPriceLabel.Label.Format("2.%s", note.GetChars());
                p_PriceLabels->insert(p_PriceLabels->end(), TmpPriceLabel);
            }
        }
    }

    sc.p_GDIFunction = DrawToChart;
}

void DrawToChart(HWND WindowHandle, HDC DeviceContext, SCStudyInterfaceRef sc)
{

    SCString log;

    int xOffset = sc.Input[7].GetInt();
    int yOffset = sc.Input[8].GetInt();
    int x = sc.GetDOMColumnLeftCoordinate(n_ACSIL::DOM_COLUMN_GENERAL_PURPOSE_1);
    x += xOffset;
    int y;
    SCString msg;
    std::vector<PriceLabel> *p_PriceLabels = (std::vector<PriceLabel>*)sc.GetPersistentPointer(0);

    float plotPrice = 0;

    // grab the name of the font used in this chartbook
    int fontSize = sc.Input[6].GetInt();
    //int fontSize = 14;
    SCString chartFont = sc.ChartTextFont();

    // Windows GDI font creation
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta
    HFONT hFont;
    hFont = CreateFont(fontSize,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, DEFAULT_PITCH,TEXT(chartFont));

    // https://docs.microsoft.com/en-us/windows/win32/gdi/colorref
    //const COLORREF bg = 0x00C0C0C0;
    const COLORREF fg = 0x00000000;
    COLORREF bg = sc.Input[9].GetColor();
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-settextcolor
    ::SetTextColor(DeviceContext, fg);
    ::SetBkColor(DeviceContext, bg);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    //SetBkMode(DeviceContext, OPAQUE);
    SelectObject(DeviceContext, hFont);

    // Windows GDI transparency 
    // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setbkmode
    //SetBkMode(DeviceContext, TRANSPARENT);
    //int NumberOfDecimals = sc.Input[1].GetInt();
    int NumPriceLabels = p_PriceLabels->size();
//log.Format("%d PriceLabels found", NumPriceLabels);
//sc.AddMessageToLog(log,1);
    for (int i=0; i<NumPriceLabels; i++) {

        // fetch the PriceLabel we want to draw
        PriceLabel TmpPriceLabel = p_PriceLabels->at(i);
        plotPrice = TmpPriceLabel.Price;
        msg = TmpPriceLabel.Label;
        COLORREF LabelColor = (COLORREF)TmpPriceLabel.LabelColor;
        ::SetTextColor(DeviceContext, LabelColor);

        // calculate coords
        y = sc.RegionValueToYPixelCoordinate(plotPrice, sc.GraphRegion);
        y += yOffset;
        ::SetTextAlign(DeviceContext, TA_NOUPDATECP);
//log.Format("[%d/%d] %d (%f) => %s", (i+1), NumPriceLabels, y, plotPrice, msg.GetChars());
//sc.AddMessageToLog(log,1);
        ::TextOut(DeviceContext, x, y, msg, msg.GetLength());

    }

    // delete font
    DeleteObject(hFont);

    return;
}
