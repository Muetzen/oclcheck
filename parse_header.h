#ifndef PARSEHEADER_H
#define PARSEHEADER_H

#include <fstream>
#include <string>
#include <vector>

// A simple class to parse a cpp header file.
class   ParseHeader
{
    public:
        ParseHeader (void);

        /*
           Parse a header file.
           Returns an error message, or an empty string if successfull.
        */
        std::string parse (const std::string & filename);

        std::string printErrorStringMethod (void);
        std::string printClTypeMethods (void);
        std::string printMethods (void);

    private:
        std::ifstream   mFile;
        int             mLine;  // Current line number in filename.

        std::string parseFile (void);   

        enum handleNl
        {
            SKIP_NL,
            RETURN_NL
        };
        std::string readToken (std::string & token, enum handleNl = SKIP_NL);
        std::string readIncludeFilename (std::string & token);  
        std::string readDefineValue (std::string & token);  
        std::string skipLine (void); 
        std::string skipMultiLineComment (void); 

        // skip until next ';'
        std::string skipStatement (void);

        std::vector <std::string>   mClErrorCodes;

        struct openclTypeInfo
        {
            std::string mType;
            std::string mDefineName;
        };
        std::vector <struct openclTypeInfo>     mTypeInfo;

        struct parameter
        {
            std::string mTypePrefix;
            std::string mName;
            std::string mTypeSuffix;
            bool        mIsCallback = false;
            std::vector <struct parameter> mParameterList;
        };

        struct method
        {
            std::string mReturnType;
            std::string mName;
            std::vector <struct parameter> mParameters;
        };
        std::vector <struct method> mMethods;

        std::string parseMethodParameter (std::vector <struct parameter> & parameterList);

        std::string printMethodParameter (
                    const std::vector <struct parameter> & parameterList,
                    const std::string & indent = "\t");

        bool        has_errcode_ret (const std::vector <struct parameter> & parameterList);

        std::string mLastComment;   // Used to determine the meaning of a #define in cl.h.
};

#endif
