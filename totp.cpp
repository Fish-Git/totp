// Copyright (C) Chris Webb <chris@arachsys.com>
// Copyright (C) "Fish" (David B. Trout) <fish@softdevlabs.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "stdafx.h"

//------------------------------------------------------------------------------
//                                 TOTP
//------------------------------------------------------------------------------
//
//  The only input is the "shared secret" line, which is read from stdin
//  (i.e. either from the keyboard or from a file redirected to stdin).
//
//  Note: this program is purposely written to NOT obtain any of its runtime
//  options via command-line arguments, since: 1) each option pertains to
//  the specific shared secret and its resulting calculated totp code, and
//  2) the logic to parse the command line options would only unnecessarily
//  complicate the logic for what should be a very simple program.
//
//  The format of the "shared secret" input line is as follows:
//
//
//   [*#;][TEST:][DIGEST:]SECRET[:DIGITS[:INTERVAL[:OFFSET]]][*#;][any text...]
//
//
//  Thus the only required portion of the shared secret input line is the
//  shared secret itself. All of the other directives on the input line are
//  optional.
//
//   *#;          If the statement begins with either '*', '#' or ';',
//                then the entire line is completely ignored. This allows
//                creation of self-documenting test files used as input.
//
//   TEST         Changes the meaning of OFFSET from it's normal meaning
//                to instead be the exact time() value to be used as per
//                Table 1 of Appendix B on Page 15 of RFC 6238, used to
//                validate correct program functionality.
//
//   DIGEST       Can be "sha1", "sha256" or "sha512", and defaults to
//                the Google Authenticator value of "sha1" if unspecified.
//
//   SECRET       Is the only compulsory field. As with Google Authenticator,
//                it should be base32 encoded using the standard [A-Z2-7]
//                alphabet. Lowercase and/or blanks may be used for readability
//                as all blanks are removed and all non-blanks are automatically
//                converted to uppercase.
//
//   DIGITS       Is the number of decimal output digits and can be 6, 7,
//                or 8. It defaults to the Google Authenticator value of 6.
//
//   INTERVAL,
//   OFFSET       Are the counter interval in seconds and the (signed) Unix
//                time at which the counter starts. These should usually be
//                left at the Google Authenticator defaults of 30 and 0,
//                but it is sometimes handy to specify an OFFSET of +/- the
//                interval to get the next or previous code in the sequence.
//
//                NOTE: when the "TEST:" option is specified, then OFFSET
//                is the exact time() value to be used for testing.
//
//   COMMENT...   Any trailing comment MUST start with a non-base32 character!
//
//  There are no command line options. The only required input is your secret,
//  always from stdin. You can either paste it directly into the program, or
//  redirect it from a file.
//
//  If you choose to paste it, don't forget to press the enter key afterwards.
//  The program is line oriented, so you must end each line with a newline.
//
//  If not redirecting input from a file (i.e. if pasting from the keyboard),
//  use Ctrl+C to exit the program. Otherwise if input is from a file, the
//  program will exit automatically once EOF is reached on stdin.

//------------------------------------------------------------------------------

#define TOTP_VERSION    VERSION_STR

#define SEP_CHAR        ':'         // colon

static const char* base32 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

#define DEF_DIGITS      6           // (default output digits) (Google Authenticator)
#define DEF_INTERVAL    30          // (default time interval) (Google Authenticator)
#define DEF_OFFSET      0           // (default time offset)   (Google Authenticator)

#define BASE32_BITS     5           // (because 2**5 = 32, Duh!)
#define BITS_PER_BYTE   CHAR_BIT    // (there are 8 bits in a byte/char)
#define GETLINE_CHUNK   128         // (getline function chunk size)

static bool g_bStdinKeyboard = true;

//---------------------------------------------------------------------
//                      is_keyboard_stdin
//---------------------------------------------------------------------

static bool is_keyboard_stdin( HANDLE hStdin )
{
    return (g_bStdinKeyboard = (GetFileType( hStdin ) == FILE_TYPE_CHAR));
}

//---------------------------------------------------------------------
//                      disable_stdin_echo
//---------------------------------------------------------------------

static bool disable_stdin_echo()
{
    DWORD mode;
    HANDLE hStdin;

    hStdin = GetStdHandle( STD_INPUT_HANDLE );

    if (!hStdin || hStdin == INVALID_HANDLE_VALUE)
        return false;

    if (!is_keyboard_stdin( hStdin ))
        return true; // (presume redirected)

    // It's a console device; disable echoing
    return (1
            && GetConsoleMode( hStdin, &mode )
            && SetConsoleMode( hStdin, mode & (~ENABLE_ECHO_INPUT))
    );
}

//---------------------------------------------------------------------
//                         my_getline
//---------------------------------------------------------------------

static ssize_t  my_getline( char** buf, size_t* bufsiz, FILE* f )
{
    char*   bufptr;
    char*   p;
    size_t  size;
    int     c;

    if (0
        || !buf
        || !bufsiz
        || !f
    )
    {
        return EOF;
    }

    bufptr = *buf;
    size = *bufsiz;

    if ((c = fgetc( f )) == EOF)
        return EOF;

    if (!bufptr)
    {
        if (!(bufptr = (char*) malloc( GETLINE_CHUNK )))
            return EOF;
        size = GETLINE_CHUNK;
    }

    p = bufptr;

    while (c != EOF)
    {
        if (1
            && (p - bufptr + 2) > (ssize_t)size
            && !(bufptr = (char*) realloc( bufptr, (size += GETLINE_CHUNK) ))
        )
            return EOF;

        *p++ = c;

        if (c == '\n')
            break;

        c = fgetc( f );
    }

    *p++ = '\0';
    *buf = bufptr;
    *bufsiz = size;

    return p - bufptr - 1;
}

//---------------------------------------------------------------------
//                         is_comment_char
//---------------------------------------------------------------------

static bool is_comment_char( char c )
{
    return (0
            || c == '*'     // asterisk
            || c == '#'     // number sign, pound sign, or hash tag
            || c == ';'     // semi-colon
            );
}

//---------------------------------------------------------------------
//                         my_rem_blanks
//---------------------------------------------------------------------

static size_t my_rem_blanks( char* src )
{
    char* dst = src;
    do
    {
        while (isspace( *src ))
            ++src;
    }
    while (*dst++ = *src++);

    return strlen( src );
}

//---------------------------------------------------------------------
//                           M A I N
//---------------------------------------------------------------------

int main( void )
{
    char*          line;
    char*          secret;
    const char*    p;

    int64_t        offset;
    uint64_t       clock;
    uint64_t       interval;

    size_t         key_size;
    size_t         key_len;
    size_t         linesize;

    uint32_t       i;
    uint32_t       code;
    uint32_t       secret_num;
    uint32_t       accum_bits;

    uint8_t*       hmac;
    uint8_t*       key;
    uint8_t        digits;
    uint8_t        msg[8];
    bool           test_mode;

    unsigned int   hmacsize;

    const EVP_MD*  digest;

    //-----------------------------------------------------------------
    // Allow reading shared SECRET from stdin in a secure manner...
    //-----------------------------------------------------------------

    if (!disable_stdin_echo())
    {
        fprintf( stderr, "ERROR: disable_stdin_echo() FAILED!\n" );
        return EXIT_FAILURE;
    }

    //-----------------------------------------------------------------
    // Display version information and cmdline usage (i.e. help info)
    //-----------------------------------------------------------------

    if (g_bStdinKeyboard)
    {
        fprintf( stderr,

            "\n  Fish's x64 Windows TOTP, version "TOTP_VERSION".\n\n"

            "  There are no command line options. The only required input\n"
            "  is your secret, always from stdin. You can either paste it\n"
            "  directly into the program, or redirect it from a file.\n\n"

            "  If you choose to paste it, don't forget to press the enter\n"
            "  key afterwards. The program is line oriented, so you must\n"
            "  end each line with a newline.\n\n"

            "  Refer to documentation for input format.\n\n"

            "  If not redirecting input from a file (i.e. if pasting from\n"
            "  the keyboard), use Ctrl+C to exit the program. Otherwise if\n"
            "  input is from a file, the program will exit automatically\n"
            "  once EOF is reached on stdin.\n\n"
        );
    }

    //-----------------------------------------------------------------
    // Read input string (see documentation for format) from stdin,
    // and calculate the corresponding TOTP value. Keep doing this
    // until EOF is reached on stdin.
    //-----------------------------------------------------------------

    key   = NULL,  key_size  = 0;
    line  = NULL,  linesize  = 0;

    while (my_getline( &line, &linesize, stdin ) >= 0)
    {
        linesize = my_rem_blanks( line );
        secret = line;

        if (is_comment_char( secret[0] ))
            continue;

        // Extract the TEST option, if specified...

        if (!strncasecmp( secret, "test:", strlen( "test:" ))) {
             test_mode = true,  secret +=  strlen( "test:" );  }
        else test_mode = false;

        // Extract the requested DIGEST function to be used...

             if (!strncasecmp( secret, "sha1:",   strlen( "sha1:"   ))) digest = EVP_sha1(),   secret += strlen( "sha1:"   );
        else if (!strncasecmp( secret, "sha256:", strlen( "sha256:" ))) digest = EVP_sha256(), secret += strlen( "sha256:" );
        else if (!strncasecmp( secret, "sha512:", strlen( "sha512:" ))) digest = EVP_sha512(), secret += strlen( "sha512:" );
        else            /* use default "sha1:" */                       digest = EVP_sha1();

        //-------------------------------------------------------------
        // Now extract the SECRET string, and convert it from its base32
        // character format to its 32-bit binary equivalent value in 'key'...
        //-------------------------------------------------------------

        for (accum_bits=0, secret_num=0, key_len=0; *secret; secret++)
        {
            // Ensure the next character to be processed is still one
            // of the SECRET value's base32 characters...

            if (!(p = strchr( base32, *secret )))
                break; // (either SEP_CHAR or end of SECRET string)

            // Convert this base32 character to its 5-bit binary eqivalent,
            // and add it to our running total...

            secret_num = (uint32_t) ((secret_num << BASE32_BITS) | (p - base32));
            accum_bits += BASE32_BITS;

            // When we have enough bits, save another byte...

            while (accum_bits >= BITS_PER_BYTE)
            {
                // (make room for another byte...)

                while (key_len >= key_size)
                {
                    key_size = key_size ? key_size << 1 : 64;

                    if (!(key = (uint8_t*) realloc( key, key_size )))
                    {
                        fprintf( stderr, "ERROR: realloc( key, %d ) FAILED! - %s\n",
                            (int) key_size, strerror( errno ));
                        return EXIT_FAILURE;
                    }
                }

                // (save the next byte...)

                accum_bits -= BITS_PER_BYTE;
                key[ key_len++ ] = (uint8_t) (secret_num >> accum_bits);
            }
        }

        //-------------------------------------------------------------
        // "key" array now contains our "SECRET" as a string of binary
        // bytes. "key_len" holds how long "key" is in number of bytes.
        //-------------------------------------------------------------

        // Extract other i/p parameters: DIGITS, INTERVAL and OFFSET...

        digits   = *secret == SEP_CHAR ? (uint8_t) strtoul(  secret + 1, &secret, 10 ) : DEF_DIGITS;
        interval = *secret == SEP_CHAR ?           strtoull( secret + 1, &secret, 10 ) : DEF_INTERVAL;
        offset   = *secret == SEP_CHAR ?           strtoll(  secret + 1, &secret, 10 ) : DEF_OFFSET;

        // Calculate and output the resulting "Time-based One-Time-
        // Password" (TOTP) verification code... (but don't bother
        // unless *ALL* of our parameters are valid!)

        if (1
            && digits   > 0     // (valid?)
            && interval > 0     // (valid?)
            && key_len  > 0     // (valid?)
        )
        {
            //---------------------------------------------------------
            // Use time-based interval as message to be hashed...
            //
            // Note: We cannot directly use the 8-byte 64-bit 'clock'
            // variable value itself since the value thus inputted to
            // the hashing algorithm would then vary depending on the
            // endianess of the system we were running on. Therefore
            // to ensure consistency, we always convert the value to
            // big-endian format in 'msg[8]'.
            //---------------------------------------------------------

            if (!test_mode)
                clock = (time( NULL ) - offset) / interval;
            else
                clock = offset / interval;

            for (i=0; i < (uint32_t) sizeof( msg ); i++)
                msg[ (sizeof( msg )-1) - i ] = (uint8_t) (clock >> (8 * i));

            // ("msg" is now clock value in big-endian format)

            //---------------------------------------------------------
            // Calculate the HMAC (Hash-Based Message Authentication
            // Code), which is the resulting verification code we're
            // to produce, using our shared SECRET and current time-
            // of-day value.
            //---------------------------------------------------------

            hmac = HMAC
            (
                digest,         // (which hashing algorithm to use)

                key,            // (shared SECRET to be used to hash with)
                (int) key_len,

                msg,            // ("message" to be hashed = time interval number)
                sizeof( msg ),

                NULL,           // (not used)

                &hmacsize       // (length of resulting hash value: 20
                                //  if sha1, 32 if sha256, 64 if sha512)
            );

            //---------------------------------------------------------
            //              OKAY, THIS IS WEIRD!
            //---------------------------------------------------------

            uint8_t   rrr;      // random 0-15 value from rightmost nibble of hmac

            uint32_t  hbyte;    // one of the 4 consecutive right-to-left hmac bytes

            uint32_t  index;    // (used to index into hmac)

            uint32_t  shift;    // (work; shifts hbyte into position
                                //  to accumulate extracted hmac bytes
                                //  into 4-byte 32-bit big-endian value)

            // Extract verification code from the generated hash...

            code = 0;

            rrr = hmac[ hmacsize - 1 ];  // (rightmost hmac byte)
            rrr &= 0x0f;                 // (rightmost nibble)

            for (i=0; i < 4; i++)
            {
                index = (3 - i);                // 3, 2, 1, 0
                shift = (BITS_PER_BYTE * i);    // 0, 8, 16, 24

                hbyte = hmac[ rrr + index ];    // (extract next byte)
                hbyte <<= shift;                // (shift into position)
                code += hbyte;                  // (accumulate here)
            }

            code &= 0x7fffffff;     // (must always be signed >= 0;
                                    // code value is now big-endian)

            //---------------------------------------------------------
            // Output the result, being the rightmost number of desired
            // digits of the calculated verification code...
            //
            // Also note that that the variable "secret" still points
            // to whatever text string data (if any) that happened to
            // follow the input parameters portion of the input string.
            // So if, for example, the input was:
            //
            //   "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567:8  adam@acme.org"
            //
            // then we would output:
            //
            //   "01360115  adam@acme.org"
            //
            // where "adam@acme.org" would be the extra string data the
            // current "secret" variable value would be pointing to.
            //---------------------------------------------------------

            static bool did_this = false;

            if (test_mode && !did_this)
            {
                printf(" Actual   Expected\n");
                printf("--------  --------\n");
                did_this = true;
            }

            while (*secret == ' ') ++secret;  // (skip past preceding blanks)

                 if (digits == 6) printf( "%06u  %s\n", code %   1000000, secret );
            else if (digits == 7) printf( "%07u  %s\n", code %  10000000, secret );
            else if (digits == 8) printf( "%08u  %s\n", code % 100000000, secret );

            fflush( stdout );   // (make sure they see our output!)
        }
    }

    // Cleanup and exit...

    free( key  );
    free( line );

    return EXIT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
