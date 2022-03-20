#include "sierrachart.h"
#include <string>
#include <sstream>
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

SCSFExport scsf_GoogleSheetsLevelsImporter(SCStudyInterfaceRef sc)
{
    // logging object
    SCString msg;

    SCInputRef i_FilePath = sc.Input[0];
    SCInputRef i_Transparency = sc.Input[1];
    SCInputRef i_ShowPrice = sc.Input[2];

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

        i_ShowPrice.Name = "Show Price";
        i_ShowPrice.SetYesNo(0);

        return;
    }

    SCString Url = i_FilePath.GetString();
    // add specific code to output as CSV attachment from Google Sheets
    Url.Format("%s/gviz/tq?tqx=out:csv", Url.GetChars());

    // HTTP request start
    // status codes
    enum {REQUEST_NOT_SENT = 0,  REQUEST_SENT, REQUEST_RECEIVED};
    // latest request status
    int& RequestState = sc.GetPersistentInt(1);
    // Only run on full recalc
    if (sc.Index == 0)
    {
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
    std::istringstream input(sc.HTTPResponse.GetChars());
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
            Tool.ChartNumber = sc.ChartNumber;
            Tool.BeginDateTime = sc.BaseDateTimeIn[0];
            Tool.EndDateTime = sc.BaseDateTimeIn[sc.ArraySize-1];
            Tool.AddMethod = UTAM_ADD_OR_ADJUST;
            Tool.ShowPrice = i_ShowPrice.GetInt();
            Tool.TransparencyLevel = i_Transparency.GetInt();
            Tool.Text = note;
            Tool.LineNumber = LineNumber;
            sc.UseTool(Tool);

            // increment field counter
            idx++;
        }

        // increment row counter
        LineNumber++;
    }
}
