--------------------------------------------------------------------------------

                                TOTP

            Fish's 'TOTP' (Time-Based One-Time-Password) Utility
                    https://github.com/Fish-Git/totp

                        Based on original work:

            Copyright (C) Chris Webb <chris@arachsys.com> 
                    https://github.com/arachsys/totp

--------------------------------------------------------------------------------

This program requires the "libcrypto-3-x64.dll" from the OpenSSL project.

OpenSSL for Windows may be installed from:

   * https://slproweb.com/products/Win32OpenSSL.html

Afterwards, append its directories to your system's environment variables:

  PATH (always!)
  
    C:\Program Files\OpenSSL-Win64\bin

  INCLUDE (only if building)
  
    C:\Program Files\OpenSSL-Win64\include
    C:\Program Files\OpenSSL-Win64\include\openssl

  LIB (only if building)

    C:\Program Files\OpenSSL-Win64\lib
    C:\Program Files\OpenSSL-Win64\lib\VC
    C:\Program Files\OpenSSL-Win64\lib\VC\static


--------------------------------------------------------------------------------


Changes and enhancements by Fish (version 1.2):


  * Remove spaces from input line. This is mostly for Google Authenticator
    compatibility as its SECRET is always shown to the user as a lowercase
    string with a blank/space inserted after every 4 characters for read-
    ability.

    NOTE! THIS CHANGE REQUIRES ANY TRAILING COMMENT (IF ANY) TO BEGIN WITH
    A NON-BASE32 CHARACTER (such as a '*', '#' or ';' comment char) SO THAT
    THE END OF THE 'SECRET' STRING CAN BE PROPERLY DETECTED!


Changes and enhancements by Fish (version 1.1):


  * Convert input to uppercase, since A-Z2-7 base32 encoding is required,
    and currently we reject anything that is not valid base32. This is
    mostly for Google Authenticator compatibility as its SECRET is always
    shown to the user as a lowercase string.


Changes and enhancements by Fish (version 1.0):


  . Made it a Visual Studio 2008 project (Duh!)

  . Added my own Copyright to "COPYING".

  . Coding style (Duh!). It's my own, not Chris's.

  . Added a bunch of comments to try and explain what's going on
    (i.e. how the program works). I also changed some of the variable
    names and added a few new variables too. I hope I got them right!

  . Added stdin "TEST:" option to allow testing for correctness of
    functionality by verifying values produced match those that are
    documented in RFC 6328 Appendix B. Refer to comments in source
    file "totp.cpp" for usage information.

  . Added support for comments in stdin input. Created stdin.txt file
    to test with, as well as expected test results stdout.txt file.

  . NOTE: comments in stdin.txt contain RFC 6328 errata corrections.

  . Wrote my own "getline" function since Windows didn't have one.

  . Added support to disable stdin keyboard echoing for security.


--------------------------------------------------------------------------------
