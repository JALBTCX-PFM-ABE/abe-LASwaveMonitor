
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! or / / ! are being used by Doxygen to
    document the software.  Dashes in these comment blocks are used to create bullet lists.
    The lack of blank lines after a block of dash preceeded comments means that the next
    block of dash preceeded comments is a new, indented bullet list.  I've tried to keep the
    Doxygen formatting to a minimum but there are some other items (like <br> and <pre>)
    that need to be left alone.  If you see a comment that starts with / * ! or / / ! and
    there is something that looks a bit weird it is probably due to some arcane Doxygen
    syntax.  Be very careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



QString prefsText = 
  LASwaveMonitor::tr ("<img source=\":/icons/prefs.png\"> Click this button to change program preferences.  This includes "
                      "position format and the colors.");
QString modeText = 
  LASwaveMonitor::tr ("<img source=\":/icons/mode_line.png\"> <img source=\":/icons/mode_dot.png\"> Click this button to toggle between "
                      "line and dot drawing modes for the wave display.  When selected the waves are drawn as lines, when unselected the "
                      "waves are drawn as discrete dots.");

QString quitText = 
  LASwaveMonitor::tr ("<img source=\":/icons/quit.png\"> Click this button to <b><em>exit</em></b> from the program.  "
                      "You can also use the <b>Quit</b> entry in the <b>File</b> menu.");
QString mapText = 
  LASwaveMonitor::tr ("This is the LASwaveMonitor program, a companion to the pfmEdit3D program for viewing LAS "
                      "waveforms.<br><br>"
                      "Help is available on most fields in LASwaveMonitor using the What's This pointer.");

QString bGrpText = 
  LASwaveMonitor::tr ("Select the format in which you want all geographic positions to be displayed.");

QString closePrefsText = 
  LASwaveMonitor::tr ("Click this button to close the preferences dialog.");

QString restoreDefaultsText = 
  LASwaveMonitor::tr ("Click this button to restore colors, size, and position format to the default settings.");

QString dateLabelText = 
  LASwaveMonitor::tr ("This is the date and time of the point.  The fields are, in order, year, month, day, "
                      "(julian day), hour, minutes, and second.  Unless, of course, the LAS file used the old GPS seconds of the "
                      "week to store time, in which case the field is GPS seconds of the week.");

QString syntheticText = 
  LASwaveMonitor::tr ("This is the synthetic flag.  Normally this flag is set to mean that the point was created by a technique other "
                      "than LiDAR collection.  In the case of the PFM Area-Based Editor it may be used to denote <b>reference</b> data. "
                      "That is, data that is not invalid but is optionally displayed or used for making products. An example might be "
                      "a tethered buoy that was detected by LiDAR but whose location is not constant.  This data is important so we don't "
                      "want to mark it as <b>withheld</b> but we usually don't want to view it or use it in a hydrographic product (other "
                      "than as an aid to navigation).");


QString keypointText = 
  LASwaveMonitor::tr ("This is the keypoint flag.  Normally this flag is set to flag a point that is a model key point.  In the PFM Area-Based "
                      "Editor we use this flag to denote an IHO selected feature such as a shoal point (which is certainly a key point).");

QString withheldText = 
  LASwaveMonitor::tr ("This is the withheld flag.  This is used to mark <b>Deleted</b> or, in the case of PFM, <b>invalid</b> points.");

QString overlapText = 
  LASwaveMonitor::tr ("This is the overlap flag.  If set, the point is in the overlap region of two swaths or takes.  The PFM Area-Based "
                      "Editor does not make use of this flag.  It was added in LAS v1.4");

QString scannerChanText = 
  LASwaveMonitor::tr ("This is the scanner channel.  It is used to indicate the head of a multi-channel (head) system.");

QString scanDirText = 
  LASwaveMonitor::tr ("This is the scan direction flag.  A value of 1 indicates a positive scan direction.");

QString edgeText = 
  LASwaveMonitor::tr ("This is the flightline edge flag.  A value of 1 indicates the point is at the edge of scan.");

QString NIRText = 
  LASwaveMonitor::tr ("This is the Near Infrared channel value.");
