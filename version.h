///////////////////////////////////////////////////////////////////////////////////////
//
//  version.h  --  defines the build version '#define' strings for the product
//
///////////////////////////////////////////////////////////////////////////////////////

#pragma once


#define VERMAJOR_NUM  1         /* MAJOR Release (program radically changed) */
#define VERMAJOR_STR "1"

#define VERINTER_NUM  2         /* Minor Enhancements (new/modified feature, etc) */
#define VERINTER_STR "2"

#define VERMINOR_NUM  0         /* Bug Fix */
#define VERMINOR_STR "0"


//----------------------------------------------------------------------------------------
// Custom automatic repo versioning...
//----------------------------------------------------------------------------------------

#if 0 // Fish: disable for public consumption  (AutoBuildCount is my custom versioning)

  //////////////////////////////////
  #include "AutoBuildCount.h"     //  (where SVNREV_STR and SVNREV_NUM are defined)
  //////////////////////////////////

#else // Fish: public releases don't have my custom AutoBuildCount versioning system)

  #define REV_NUM  0              //  (normally the SVN rev num or the git repo hash)
  #define REV_STR "0"             //  (normally the SVN rev num or the git repo hash)

#endif


//----------------------------------------------------------------------------------------
// Construct VERSION string consisting of above MAJOR/INTER/MINOR strings + repo info
//----------------------------------------------------------------------------------------

#ifdef _DEBUG
  #define VERSION_STR  VERMAJOR_STR "." VERINTER_STR "." VERMINOR_STR "." REV_STR "-D"
#else
  #define VERSION_STR  VERMAJOR_STR "." VERINTER_STR "." VERMINOR_STR "." REV_STR
#endif


///////////////////////////////////////////////////////////////////////////////////////
