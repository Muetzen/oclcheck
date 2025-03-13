#include "parse_header.h"
#include <iostream>
#include <sstream>
#include <string.h>

ParseHeader::ParseHeader (void)
{
}


std::string
ParseHeader::parse (const std::string & filename)
{
    mLine = 1;
    mFile.open (filename);  
    if (mFile.good () == false)
    {
        return std::string ("Could not open file '") + filename + "'.";
    }

    std::string ret = parseFile ();

    mFile.close ();

    return ret;
}


std::string
ParseHeader::printMethods (void)
{
    std::string ret;

    std::cout << "// This file is automatically generated, do not edit.\n";
    for (size_t i = 0; i < mMethods.size (); ++i)
    {
        std::cout << "\n\n";
        std::cout << mMethods [i].mReturnType << "\n";
        std::cout << mMethods [i].mName << "\n";

        ret = printMethodParameter (mMethods [i].mParameters);

        std::cout << "{\n";
        std::cout << "\tinitialize ();\n";
        // TODO: Error exit

        std::cout << "\tstd::cerr << \"OCL> " << mMethods [i].mName << " ()\\n\";";

        std::cout << "\t" << mMethods [i].mReturnType << "(* origMethod)\n";
        printMethodParameter (mMethods [i].mParameters);
        std::cout << "\t= nullptr;\n";

        // TODO: This does not work everywhere
//      std::cout << "\tdecltype (& " << mMethods [i].mName << ") origMethod = nullptr;\n";

        // TODO: make origMethod static and fetch the method only once
        std::cout << "\torigMethod = reinterpret_cast <decltype (origMethod)> (dlsym (RTLD_NEXT, \""
            << mMethods [i].mName << "\"));\n";

        // TODO: check for nullptr before calling origMethod
        if (mMethods [i].mReturnType != "void")
        {
            std::cout << "\t" << mMethods [i].mReturnType << " returnValue;\n";

            std::cout << "\treturnValue = ";
        }
        else
        {
            std::cout << "\t";
        }

        std::cout << "origMethod (\n";
        for (size_t j = 0; j < mMethods [i].mParameters.size (); ++j)
        {
            std::cout << "\t\t" << mMethods [i].mParameters [j].mName;
            if (j < mMethods [i].mParameters.size () - 1)
            {
                std::cout << ",";
            }
            std::cout << "\n";
        }
        std::cout << "\t);\n";

        if (mMethods [i].mReturnType != "void")
        {
            std::cout << "\treturn returnValue;\n";
        }

        std::cout << "}\n";
    }

    return ret;
}


std::string
ParseHeader::printMethodParameter (
        const std::vector <struct parameter> & pl,
        const std::string & indent)
{
    std::string ret;

    std::cout << indent << "(\n";
    for (size_t j = 0; j < pl.size (); ++j)
    {
        std::cout << indent << "\t" << pl [j].mTypePrefix << " ";
        std::cout << pl [j].mName << " ";

        std::cout << pl [j].mTypeSuffix;
        if (j < pl.size () - 1)
        {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << indent << ")\n";

    return ret;
}


std::string
ParseHeader::parseFile (void)
{
    std::string ret;
    while (mFile.eof () == false)
    {
        std::string token;

        ret = readToken (token);
        if ( ! ret.empty ())
        {
            break;
        }

        if (token == "#include")
        {
            token += ' ';
            ret = readIncludeFilename (token);
            if ( ! ret.empty ())
            {
                break;
            }
        }

        if (token == "#define")
        {
            std::string varname;
            ret = readToken (varname);
            if ( ! ret.empty ())
            {
                break;
            }

            token += ' ';
            token += varname;

            std::string value;
            ret = readDefineValue (value);

            if (value.empty () == false)
            {
                token += ' ';
                token += value;
            }
        }

        // #if, #endif, #pragma, ...
        if (token [0] == '#')
        {
            std::string value;
            ret = readDefineValue (value);
            continue;
        }

        if (token == "typedef")
        {
            ret = skipStatement ();
            continue;
        }

        if (token == "extern")
        {
            struct  method  m;

            ret = readToken (token);
            if ( ! ret.empty ())
            {
                break;
            }

            if (token == "\"C\"")
            {
                // skip: extern "C"
                continue;
            }

            if (token == "CL_API_ENTRY")
            {
                ret = readToken (m.mReturnType);
            }

            if (strncmp (m.mReturnType.c_str (), "CL_API_PREFIX__VERSION_", strlen ("CL_API_PREFIX__VERSION_")) == 0)
            {
                ret = readToken (m.mReturnType);
            }

            ret = readToken (token);
            while (token != "CL_API_CALL")
            {
                m.mReturnType += ' ';
                m.mReturnType += token;
                ret = readToken (token);
            }

//          std::cerr << "return type: " << m.mReturnType << "\n";

            ret = readToken (m.mName);

//          std::cerr << "method name: " << m.mName << "\n";

            ret = readToken (token);

            if (token == "(")
            {
                ret = parseMethodParameter (m.mParameters);
            }

            mMethods.push_back (m);

            // TODO: check CL_API_SUFFIX
            ret = skipStatement ();
            continue;
        }


        
//      std::cerr << token << "\n";
    }

    return ret;
}

std::string
ParseHeader::readToken (std::string & token, enum handleNl  nl)
{
    char        c;
    std::string ret;

    token.clear ();
    while (mFile.eof () == false && ret.empty ())
    {
        if (mFile.peek () == '\n' && token.empty () == false)
        {
            break;
        }

        mFile.get (c);

        if (c == '\n')
        {
            ++mLine;

            if (nl == RETURN_NL)
            {
                token += c;
                break;
            }
        }
        else if (c == '\\' && mFile.peek () == '\n')
        {
            mFile.get (c);
            continue;
        }
        else if (isspace (c))
        {
            if (token.empty () == false)
            {
                return ret;
            }
        }
        else if (c == '/')
        {
            if (mFile.peek () == '/')
            {
                ret = skipLine ();
            }
            else if (mFile.peek () == '*')
            {
                ret = skipMultiLineComment ();
            }
            else
            {
                std::stringstream   msg;
                msg << "'/" << (char)(mFile.peek ()) << "' found in line " << mLine << ". '/' does not start a comment.";
                ret = msg.str ();
            }
        }
        else if (isalpha (c) || c == '_')
        {
            token += c;

            c = mFile.peek ();
            while (isalnum (c) || c == '_')
            {
                mFile.get (c);
                token += c;
                c = mFile.peek ();
            }

            break;
        }
        else if (c == '#')
        {
            token += c;

            c = mFile.peek ();
            while (isspace (c))
            {
                mFile.get (c);
                c = mFile.peek ();
            }

            c = mFile.peek ();
            while (isalpha (c))
            {
                mFile.get (c);
                token += c;
                c = mFile.peek ();
            }
            break;
        }
        else if (isdigit (c) ||
                ((c == '-' || c == '+') && isdigit (mFile.peek ())))
        {
            token += c;

            c = mFile.peek ();
            while (isxdigit (c) || c == '.' || tolower (c) == 'x' || tolower (c) == 'o' || c == '-' || c == '+')
            {
                mFile.get (c);
                token += c;
                c = mFile.peek ();
            }

            break;
        }
        else if (c == '"')
        {
            token += c;
            mFile.get (c);
            while (! mFile.eof ())
            {
                if (c == '\n')
                {
                    ++mLine;

                    std::stringstream msg;
                    msg << "Unterminated string in line " << mLine << ".";
                    ret = msg.str ();

                    break;
                }
                else if (c == '"')
                {
                    token += c;
                    break;
                }
                else
                {
                    token += c;
                }
                mFile.get (c);
            }

            break;
        }
        else
        {
            token += c;

            char    n;
            switch (c)
            {
                case '>':
                case '<':
                case '&':
                case '|':
                    n = mFile.peek ();
                    if (n == c)
                    {
                        mFile.get (c);
                        token += c;
                        n = mFile.peek ();
                    }
                    if (n == '=')
                    {
                        mFile.get (c);
                        token += c;
                    }
                    break;

                case '=':
                case '!':
                case '~':
                    n = mFile.peek ();
                    if (n == '=')
                    {
                        mFile.get (c);
                        token += c;
                    }
                    break;
            }
            break;
        }
    }
    return ret;
}

std::string
ParseHeader::readIncludeFilename (std::string & token)
{
    char        c;
    std::string ret;

    mFile.get (c);
    while (isspace (c))
    {
        if (c == '\n')
        {
            std::stringstream msg;
            msg << "#include filename expected in line " << mLine << ".";
            ret = msg.str ();

            ++mLine;

            return ret;
        }
        mFile.get (c);
    }

    char lastChar = 0;

    if (c == '"')
    {
        lastChar = '"';
    }
    else if (c == '<')
    {
        lastChar = '>';
    }
    else
    {
        std::stringstream msg;
        msg << "#include filename expected in line " << mLine << ".";
        ret = msg.str ();

        return ret;
    }

    token += c;
        
    mFile.get (c);
    while (c != lastChar)
    {
        if (c == '\n')
        {
            std::stringstream msg;
            msg << "#include filename expected in line " << mLine << ".";
            ret = msg.str ();

            ++mLine;

            return ret;
        }

        token += c;
        mFile.get (c);
    }

    token += c;

    return ret;
}

std::string
ParseHeader::readDefineValue (std::string & token)
{
    std::string ret;

    token.clear ();
    while (true)
    {
        std::string t;

        ret = readToken (t, RETURN_NL);
        if (ret.empty () == false || t == "\n")
        {
            break;
        }

        if (token.empty () == false)
        {
            token += ' ';
        }
        token += t;
    }

    return ret;
}

std::string
ParseHeader::skipLine (void)
{
    std::string ret;
    while (mFile.eof () == false)
    {
        char    c;

        mFile.get (c);

        if (c == '\n')
        {
            ++mLine;
            return ret;
        }
    }
    return ret;
}

// Skip a comment. Note: The first '/' character is already read.
std::string
ParseHeader::skipMultiLineComment (void)
{
    char c;

    // read '*'
    mFile.get (c);

    while (mFile.eof () == false)
    {
        mFile.get (c);

        if (c == '\n')
        {
            ++mLine;
        }
        else if (c == '*' && mFile.peek () == '/')
        {
            // read '/'
            mFile.get (c);
            return std::string ();
        }
    }

    return "Expected end of multi line comment before end of file.";
}

std::string
ParseHeader::skipStatement (void)
{
    std::string ret;
    std::string token;

    int scope = 0;
    do
    {
        ret = readToken (token);
        if (token == "{")
        {
            ++scope;
        }
        else if (token == "}")
        {
            --scope;
        }
    }
    while (ret.empty () && (scope != 0 || token != ";"));

    return ret;
}

std::string
ParseHeader::parseMethodParameter (std::vector <struct parameter> & parameterList)
{
    std::string ret;
    std::string token;

    while (! mFile.eof () && token != ")")
    {
        struct  parameter   p;

        // Empty parameter list
        ret = readToken (token);
        if (token == ")")
        {
            break;
        }

        // Parameter is (void)
        p.mTypePrefix = token;
        ret = readToken (p.mName);
        if (p.mTypePrefix == "void" && p.mName == ")")
        {
            break;
        }

        // callback parameter
        if (p.mName == "(")
        {
            p.mTypePrefix += ' ';
            p.mTypePrefix += p.mName;

            ret = readToken (p.mName);

            ret = readToken (token);

            while (token != ")")
            {
                p.mTypePrefix += ' ';
                p.mTypePrefix += p.mName;

                p.mName = token;
                ret = readToken (token);
            }

            p.mTypeSuffix = token;

            // This should be "(":
            ret = readToken (token);

            p.mIsCallback = true;
            ret = parseMethodParameter (p.mParameterList);

            // This should be ',' or ')':
            ret = readToken (token);

//          std::cerr << "Parameter type: " << p.mTypePrefix
//              << ", name: " << p.mName
//              << ", suffix: " << p.mTypeSuffix
//              << "\n";


            continue;
        }

        ret = readToken (token);

        while (token != "," && token != ")" && token != "[")
        {
            p.mTypePrefix += ' '; 
            p.mTypePrefix += p.mName; 
            p.mName = token;

            ret = readToken (token);
        }

        if (token == "[")
        {
            p.mTypeSuffix += token;
            ret = readToken (token);

            if (token != "]")
            {
                std::stringstream msg;
                msg << "Expected ']' in line " << mLine << "\n";
                ret = msg.str ();
            }
            else
            {
                p.mTypeSuffix += token;
                ret = readToken (token);
                // token should be ',' or ')'.

                if (token != "," && token != ")")
                {
                    std::stringstream msg;
                    msg << "Expected ',' or ')' in line " << mLine << "\n";
                    ret = msg.str ();
                }
            }
        }

        parameterList.push_back (p);
    }

    return ret;
}

