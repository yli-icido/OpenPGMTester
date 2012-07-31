#ifndef PGMUTILS_H_
#define PGMUTILS_H_

class OpenPGMUtils
{
public: 
//     int     getopt();
    static int     intoOptions( std::string& options, std::map< char, std::string >& optionPairs );
    static int     intoTokens( std::string& inputString, std::string& separator, bool isSeparatorLeading, std::vector< std::string >& tokens );
    static int     trim( std::string& aString );

};

#endif // PGMUTILS_H_