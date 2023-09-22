// Copyright (C) 2020 Chris Webb <chris@arachsys.com>
// Copyright (C) 2023 "Fish" (David B. Trout) <fish@softdevlabs.com>
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
//                               TOTP
//------------------------------------------------------------------------------
//
//  Input "shared secret" line read from stdin:
//
//
//   [*#;][TEST:][DIGEST:]SECRET[:DIGITS[:INTERVAL[:OFFSET]]]  [any text...]
//
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
//   DIGEST       Can be "sha1:", "sha256:" or "sha512:", and defaults to
//                the most commonplace choice of "sha1:" if unspecified.
//
//   SECRET       Is the only compulsory field. As with Google Authenticator,
//                it should be base32 encoded using the standard [A-Z2-7]
//                alphabet without padding.
//
//   DIGITS       Is the number of decimal output digits and can be 6, 7 or 8.
//                It defaults to the standard key_len of 6.
//
//   INTERVAL
//   OFFSET       Are the counter interval in seconds and the (signed) Unix
//                time at which the counter starts. These should usually be
//                left at the defaults of 30 and 0 respectively, but it is
//                sometimes handy to specify an OFFSET of +/- the interval
//                to get the next or previous code in the sequence.
//
//                NOTE: if the "TEST:" option was specified, then OFFSET is
//                the exact time() value to be used for testing.
//
//------------------------------------------------------------------------------

static const char* base32 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

#define DEF_DIGITS      6           // (default output digits)
#define DEF_INTERVAL    30          // (default time interval)
#define DEF_OFFSET      0           // (default time offset)

#define BASE32_BITS     5           // (because 2**5 = 32, Duh!)
#define BITS_PER_BYTE   CHAR_BIT    // (there are 8 bits in a byte/char)
#define GETLINE_CHUNK   128         // (getline function chunk size)

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

    if (GetFileType( hStdin ) != FILE_TYPE_CHAR)
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
            && (p - bufptr + 1) > (ssize_t)size
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
        printf("ERROR: disable_stdin_echo() FAILED!\n");
        return EXIT_FAILURE;
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
        secret = line + strspn( line, "\t " ); // (ignore any leading whitespace)

        // Ignore comment lines...

        if (0
            || secret[0] == '*'
            || secret[0] == '#'
            || secret[0] == ';'
        )
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
                break; // (either ':' separator or end of SECRET string)

            // Convert this base32 character to its 5-bit binary eqivalent,
            // and add it to our running total...

            secret_num = (uint32_t) ((secret_num << BASE32_BITS) | (p - base32));
            accum_bits += BASE32_BITS;

            // When we have enough bits, save another byte...

            while (accum_bits >= BITS_PER_BYTE)
            {
                // (make room for another byte...)

                while (key_len >= key_size)  // (Note! ">=", NOT just ">"!)
                {
                    key_size = key_size ? key_size << 1 : 64;

                    if (!(key = (uint8_t*) realloc( key, key_size )))
                    {
                        printf("ERROR: realloc( key, %d ) FAILED! - %s\n",
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

        digits   = *secret == ':' ? (uint8_t) strtoul(  secret + 1, &secret, 10 ) : DEF_DIGITS;
        interval = *secret == ':' ?           strtoull( secret + 1, &secret, 10 ) : DEF_INTERVAL;
        offset   = *secret == ':' ?           strtoll(  secret + 1, &secret, 10 ) : DEF_OFFSET;

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

                 if (digits == 6) printf( "%06u  %s", code %   1000000, secret );
            else if (digits == 7) printf( "%07u  %s", code %  10000000, secret );
            else if (digits == 8) printf( "%08u  %s", code % 100000000, secret );

            fflush( stdout );   // (make sure they see our output!)
        }
    }

    // Cleanup and exit...

    free( key  );
    free( line );

    return EXIT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
