#include "sierrachart.h"
#include <utility>

class Helper
{

    protected:
        template <typename T>
        void logit(T t) {
            SCString log_message;
            SCString typenm = typeid(t).name();
            const char* strtype = typenm.GetChars();
            //log_message.Format("%s", strtype);
            //sc.AddMessageToLog(log_message,1);
            if (typenm == "PKc") {
                // pointer to character array - string
                //log_message.Format("String(%s)='%s'", strtype, t);
                log_message.Format("'%s'", t);
            }
            else if (typenm == "i") {
                // int
                //log_message.Format("Int(%s)='%d'", strtype, t);
                log_message.Format("'%d'", t);
            }
            else if (typenm == "d" || typenm == "f") {
                // double/float
                //log_message.Format("Float(%s)='%f'", strtype, t);
                log_message.Format("'%f'",t);
            }
            else if (typenm == "8SCString") {
                // SCString obj
                SCString * sptr = (SCString*)&t;
                SCString ksptr = *sptr;
                //log_message.Format("SCString(%s)='%s'", strtype, ksptr.GetChars());
                log_message.Format("'%s'", ksptr.GetChars());
            }
            else {
                log_message.Format("Unknown type");
            }
            sc.AddMessageToLog(log_message,1);
        }

    private:
        SCStudyInterfaceRef sc;

    public:

        // constructor with inline initialization
        Helper(SCStudyInterfaceRef r) : sc(r) {}

        // dump functions
        template <typename... Args>
        void dump(Args&&... args) {
            int tmp[] = {0, ((void) logit(std::forward<Args>(args)), 0)... };
        }

        void log(SCString log_message) {
            unsigned int bytesWritten = 0;
            int fileHandle;
            int msgLength = 0;

            SCString nowStr = sc.DateTimeToString(sc.CurrentSystemDateTime, FLAG_DT_COMPLETE_DATETIME_MS);
            log_message = nowStr + " | " + log_message;

            msgLength = log_message.GetLength() + 2;

            SCString filePath = "C:\\SierraChart\\sc_log.txt";
            sc.OpenFile(filePath, n_ACSIL::FILE_MODE_OPEN_TO_APPEND, fileHandle);
            log_message = log_message + "\r\n";
            sc.WriteFile(fileHandle, log_message, msgLength, &bytesWritten);
            sc.CloseFile(fileHandle);
        }
};
