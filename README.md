Windows 'TOTP'
==============

### Copyright (C) 2023 "Fish" (David B. Trout) <fish@softdevlabs.com>
### Based on [original work](https://github.com/arachsys/totp), Copyright (C) 2020 Chris Webb <chris@arachsys.com> 

This is a simple time-based one-time password generator, compatible with the
RFC 6238 TOTP scheme popularised by Google Authenticator, written for Windows.


Usage
-----

The **`totp`** utility reads lines from standard input, containing TOTP secrets
in the format:
<pre>
              [*#;][TEST:][DIGEST:]<b>SECRET</b>[:DIGITS[:INTERVAL[:OFFSET]]]  [any text...]
</pre>

Where:

Field | Description
------|------------
`*#;`      |   If the statement begins with either `*`, `#` or `;` it is treated as a comment and the entire line is completely ignored. Blank lines are also ignored. This allows creation of self-documenting test files used as input. &nbsp;_(See e.g. file `stdin.txt` provided with the product.)_
`TEST`     |   Changes the meaning of `OFFSET` from it's normal meaning to the exact `time()` value to be used as per Table 1 of Appendix B on Page 15 of RFC 6238, used to validate correct program functionality.
`DIGEST`   |   Can be `sha1`, `sha256` or `sha512`, and if left unspecified, defaults to the most commonplace choice of `sha1`.
`SECRET`   |   Is the only compulsory field. As with Google Authenticator, it should be [base32](https://en.wikipedia.org/wiki/Base32) encoded using the standard [A-Z2-7] alphabet without padding.
`DIGITS`   |   Is the number of decimal output digits and can be 6, 7 or 8. It defaults to the standard length of 6.
`INTERVAL,`<br>`OFFSET`   |   Are the counter interval in seconds, and the (signed) Unix time at which the counter starts. These should usually be left at the defaults of 30 and 0 respectively, but it is sometimes handy to specify an `OFFSET` of +/- the interval to get the next or previous code in the sequence.<br><br>_**NOTE:** &nbsp;if the `TEST` option was specified, then `OFFSET` is the exact `time()` value to be used for testing._


For each valid secret line read, the tool prints the current value of the
code corresponding to that secret to standard output. Any trailing text on
the secret line is also appended, allowing multiple keys in the same input
file to be labelled and distinguished.

For example, given the input:
```
  sha256:ABCDEFGHIJKLMNOPQRSTUVWXYZ234567:8 adam@acme.org
  765432ZYXWVUTSRQPONMLKJIHGFEDCBA brian@megacorp.com (current)
  765432ZYXWVUTSRQPONMLKJIHGFEDCBA:6:30:-30 brian@megacorp.com (next)
```
**`totp`** might generate:
```
  01360115 adam@acme.org
  889775 brian@megacorp.com (current)
  140851 brian@megacorp.com (next)
```

Compatibility
-------------

Google Authenticator hard-codes the sha1 digest, 30 second time step and
zero offset, hence those parameters default to those values in **`totp`**.

[Authy](https://authy.com/) also natively supports 7-digit codes. These use the same `sha1` digest
with zero offset, but with an update interval of only 10 seconds.

Although these applications store keys verbatim, they have no easy interface
to inspect or extract the keys. You may need to create new/replacement
two-factor authentication keys if you stored keys in these programs without
retaining your own copy.


Changes to this Windows version by Fish:
----------------------------------------

  * Made it a Visual Studio 2008 .cpp project (Duh!)
    &nbsp;_(Note: later versions of Visual Studio should be able to easily
    convert this project to their own Visual Studio project format.)_

  * Added my own Copyright to "COPYING".

  * Coding style (Duh!) &nbsp;_(This is **my** version after all, not Chris's!)_

  * Added a bunch of comments to try and explain what's going on
    (i.e. how the program works). I also changed some of the variable
    names and added a few new variables too.

  * Added stdin `TEST:` option to allow testing for correctness of
    functionality by verifying values produced match those that are
    documented in RFC 6328 Appendix B. Refer to comments in source
    file `totp.cpp` for usage information.

  * Added support for comments in `stdin.txt` input file to test with,
    as well as expected test results `stdout.txt` file.

  * _**Note:**_ &nbsp;comments in `stdin.txt` contain
    [RFC 6328 errata corrections](https://www.rfc-editor.org/errata/eid2866).

  * Wrote my own `getline()` function since my Windows didn't have one.

  * Added support to disable stdin keyboard echoing for security.


Security
--------

This tool was written as a work-around for sites and systems that insist on
two-factor authentication tied to an easily-lost mobile device running a
user-hostile, unauditable, proprietary operating system.

Disclosure of the secrets used to seed this generator allows an attacker to
generate valid authentication codes and should be viewed as equivalent to
a compromised password.

**Fish edit:** &nbsp;_The following is a Linux/Unix-ism. I recommend using a
password safe product instead._

You might consider encrypted storage of secrets, decrypting them when
required with something like

      age -d ~/.config/2fa.age | totp

or

      gpg -d ~/.config/2fa.gpg | totp

In this case, secrets are safe at rest assuming sufficient passphrase
entropy, but decrypting them to run **`totp`** on a compromised/untrusted host
would present an easy target to an attacker.

Please assess the security implications and trade-offs in your own
environment and circumstances.

**Fish edit:** &nbsp;_I personally recommend using a password safe, such as [Bruce Schneier](https://en.wikipedia.org/wiki/Bruce_Schneier)'s
excellent [Password Safe](https://en.wikipedia.org/wiki/Password_Safe) product._

Building and installing
-----------------------

**Fish edit:** &nbsp;_The following are the original *nix build instructions, but my (this) Windows version is a Visual Studio project. See further below for Windows build instructions._

Unpack the source tar.gz file and change to the unpacked directory.

Run 'make', then 'make install' as root to install the **`totp`** binary in /bin.
Alternatively, you can set DESTDIR and/or BINDIR to install in a different
location, or strip and copy the compiled binary into the correct place
manually.

**`totp`** depends on libcrypto and the associated header files from LibreSSL or
OpenSSL, but should otherwise be portable to any reasonable POSIX platform.

**Fish edit:** &nbsp;_Installing [Win32/Win64 OpenSSL](https://slproweb.com/products/Win32OpenSSL.html) is a prerequisite to building `totp`. Refer to file the "Fish OpenSSL README.txt" file for **important OpenSSL installation instructions.**_

It may be necessary to add include paths to CFLAGS or library paths to
LDLIBS if your compiler cannot find the LibreSSL/OpenSSL headers and
libraries in the directories it searches by default.

Bug reporting:
--------------

Please report any problems or bugs as a new "GitHub's Issue" at:

   * https://github.com/Fish-Git/totp/issues

Copying
-------

The original version of this software was written by Chris Webb <chris@arachsys.com>.
This Windows version was written by "Fish" (David B. Trout) <fish@softdevlabs.com>
and is based on Chris's [original work](https://github.com/arachsys/totp), and is
distributed as Free Software under the terms of the [MIT license](https://opensource.org/license/mit/)
in `COPYING`.

Windows build instructions
--------------------------

First, install [OpenSSL for Windows](https://slproweb.com/products/Win32OpenSSL.html)
_(**Important OpenSSL installation instructions** are detailed below)_, and then append
its directories to your system's environment variables:

  PATH:
  
    C:\Program Files\OpenSSL-Win64\bin

  INCLUDE:
  
    C:\Program Files\OpenSSL-Win64\include
    C:\Program Files\OpenSSL-Win64\include\openssl

  LIB:
  
    C:\Program Files\OpenSSL-Win64\lib
    C:\Program Files\OpenSSL-Win64\lib\VC
    C:\Program Files\OpenSSL-Win64\lib\VC\static

Then simply build **`totp`** just as you would for any other Visual Studio project.

___

Win32/Win64 OpenSSL
===================

The [Win32/Win64 OpenSSL Installation Project](https://slproweb.com/products/Win32OpenSSL.html)
is dedicated to providing a simple installation of OpenSSL for Microsoft Windows. It is easy
to set up and easy to use through the simple, effective installer. No need to compile anything
or jump through any hoops, just click a few times and it is installed, leaving you to doing
real work.

Recommended system requirements (as of Sept. 2023): Windows XP or later.


### _IMPORTANT!_

Be sure to use the _**FULL 140MB**_ installer, _not_ the 5MB Light installer,
as it is designed specifically for **Software Development** _(i.e. it installs
the necessary \#include headers, link libraries, and runtime DLLs, etc)._


### _IMPORTANT!_

When installing, I recommend choosing to install it into "The OpenSSL
binaries (/bin) directory", and _not_ into "The Windows system directory":

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`[ ]` &nbsp; _The Windows system directory_<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`[X]` &nbsp; _**The OpenSSL binaries (/bin) directory**_

In my opinion, you should _**never**_ install _any_ 3rd party
software into the  Windows system directory. The  Windows system directory
is reserved for Microsoft and Windows operating system usage only.

After installing, you can then add the OpenSSL `/bin` directory to your Windows search
PATH if you want to _(preferrably at the very end; see section previous above)_.
Or, you can just temporarily add it to your PATH whenever you need to _(via e.g. a simple batch file)_.

____
&nbsp;
