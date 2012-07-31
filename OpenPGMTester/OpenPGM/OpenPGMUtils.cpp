#include "stdafx.h"
#include "OpenPGMUtils.h"

using namespace std;
// break input string into tokens according to the separator
//  param:  inputString
//          separator
//          isSeparatorLeading: true - separator is at the beginning, or false - separator is at the end
//          tokens
int OpenPGMUtils::intoTokens( std::string& inputString, std::string& separator, bool isSeparatorLeading, std::vector< std::string >& tokens )
{
    int retval = PGM_FAILURE;
    size_t pos1 = inputString.find( separator );
    size_t pos2 = pos1;
    string curToken;

    if ( !isSeparatorLeading )
    {
        if ( pos1 != string::npos ) // get the first token
        {
            curToken = inputString.substr( 0, pos1 );
        }
        else    // no separator, the only token
        {
            curToken = inputString;
        }
        retval = trim( curToken );
        tokens.push_back( curToken );
    }

    while ( pos1 != string::npos )
    {
        // get the token, which contains only option char and its value
        pos2 = inputString.find( separator, pos1 );

        if ( pos2 == string::npos )
        {
            curToken = inputString.substr( pos1 );
        }
        else
        {
            curToken = inputString.substr( pos1, pos2 - pos1 + 1 );
        }

        if ( trim( curToken ) == PGM_SUCCESS )
            tokens.push_back( curToken );

        pos1 = pos2;
    }

    return retval;
}

int OpenPGMUtils::trim( std::string& aString )
{
    int retval = PGM_FAILURE;
    size_t begin = aString.find_first_not_of( " \"" );
    size_t end = aString.find_last_not_of( " \"" );

    if ( (begin == string::npos) || (end == string::npos) )
        return PGM_FAILURE;

    aString = aString.substr( begin, end - begin + 1 );
    retval = PGM_SUCCESS;
    return retval;
}


// break the whole option string into option pairs, each option pair contains an option switch and its value (if any)
int OpenPGMUtils::intoOptions( string& options, map< char, string >& optionPairs )
{
    int retval = PGM_FAILURE;
    size_t pos1 = options.find( "-" );
    size_t pos2 = pos1;
    string curOption;
    char optionSwitch = 0;
    string optionValue;
    while ( pos1 != string::npos )
    {
        // get the token, which contains only option char and its value
        pos2 = options.find( "-", pos1 );

        if ( pos2 == string::npos )
        {
            curOption = options.substr( pos1 );
        }
        else
        {
            curOption = options.substr( pos1, pos2 - pos1 + 1 );
        }

        // trim leading and trailing space, can use boost later
        size_t begin = curOption.find_first_not_of( "- " );
        if ( begin == string::npos )
        {
            // bad input option, skip it
            continue;
        }

        // the first char is supposed to be the switch
        optionSwitch = curOption.at( begin );
        optionValue = "";

        if ( ( begin + 1 ) < curOption.length() )   // check if there is value available for this option
        {
            begin = curOption.find_first_not_of( " ", begin + 1 );
            size_t end = curOption.find_last_not_of( " " );
            if ( ( begin != string::npos ) && ( end == string::npos ) )
            {
                optionValue = curOption.substr( begin, end - begin + 1 );
            }
        }
        optionPairs.insert(map< char, string >::value_type( optionSwitch, optionValue ) );

        pos1 = pos2;
    }

    if (( pos1 == string::npos ) && ( pos2 == string::npos ))
    {
        retval = PGM_SUCCESS;
    }

    return retval;
}

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
// int getopt(int nargc, char* const* nargv, const char* ostr)
// {
// 	static char *place = EMSG;		/* option letter processing */
// 	char *oli;				/* option letter list index */
// 
// 	if (optreset || !*place) {		/* update scanning pointer */
// 		optreset = 0;
// 		if (optind >= nargc || *(place = nargv[optind]) != '-') {
// 			place = EMSG;
// 			return (-1);
// 		}
// 		if (place[1] && *++place == '-') {	/* found "--" */
// 			++optind;
// 			place = EMSG;
// 			return (-1);
// 		}
// 	}					/* option letter okay? */
// 	if ((optopt = (int)*place++) == (int)':' ||
// 	    !(oli = strchr(ostr, optopt))) {
// 		/*
// 		 * if the user didn't specify '-' as an option,
// 		 * assume it means -1.
// 		 */
// 		if (optopt == (int)'-')
// 			return (-1);
// 		if (!*place)
// 			++optind;
// 		if (opterr && *ostr != ':' && optopt != BADCH)
// 			(void)fprintf(stderr, "%s: illegal option -- %c\n",
// 			    "progname", optopt);
// 		return (BADCH);
// 	}
// 	if (*++oli != ':') {			/* don't need argument */
// 		optarg = NULL;
// 		if (!*place)
// 			++optind;
// 	}
// 	else {					/* need an argument */
// 		if (*place)			/* no white space */
// 			optarg = place;
// 		else if (nargc <= ++optind) {	/* no arg */
// 			place = EMSG;
// 			if (*ostr == ':')
// 				return (BADARG);
// 			if (opterr)
// 				(void)fprintf(stderr,
// 				    "%s: option requires an argument -- %c\n",
// 				    "progname", optopt);
// 			return (BADCH);
// 		}
// 	 	else				/* white space */
// 			optarg = nargv[optind];
// 		place = EMSG;
// 		++optind;
// 	}
// 	return (optopt);			/* dump back option letter */
//     return EXIT_SUCCESS;
// }

