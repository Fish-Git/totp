--------------------------------------------------------------------------------

                                TOTP

            Fish's 'TOTP' (Time-Based One-Time-Password) Utility

                        Based on original work:

            Copyright (C) 2020 Chris Webb <chris@arachsys.com> 
                    https://github.com/arachsys/totp

--------------------------------------------------------------------------------

First, install OpenSSL for Windows:

   * https://slproweb.com/products/Win32OpenSSL.html

And then append its directories to your system's environment variables:

  PATH
    C:\Program Files\OpenSSL-Win64\bin

  INCLUDE
    C:\Program Files\OpenSSL-Win64\include
    C:\Program Files\OpenSSL-Win64\include\openssl

  LIB
    C:\Program Files\OpenSSL-Win64\lib
    C:\Program Files\OpenSSL-Win64\lib\VC
    C:\Program Files\OpenSSL-Win64\lib\VC\static


--------------------------------------------------------------------------------

Changes and enhancements by Fish:


  . Made it a Visual Studio 2008 .cpp project (Duh!)

  . Also added my own Copyright to "COPYING".

  . Coding style (Duh!): It's my own, not Chris's!

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
