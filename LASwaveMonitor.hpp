
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



/*  LASwaveMonitor class definitions.  */

#ifndef LASWAVEMONITOR_H
#define LASWAVEMONITOR_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <cmath>

#include "nvutility.h"
#include "nvutility.hpp"

#include "pfm.h"
#include "ABE.h"

#include "lasreader.hpp"
#include "slas.hpp"

#include "version.hpp"


#include <QtCore>
#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif


#define WAVE_X_SIZE       440
#define WAVE_Y_SIZE       620
#define SPOT_SIZE         2

#define GCS_NAD83 4269
#define GCS_WGS_84 4326


//   WGS84 / UTM northern hemisphere: 326zz where zz is UTM zone number
//   WGS84 / UTM southern hemisphere: 327zz where zz is UTM zone number
//   US State Plane (NAD83): 269xx

#define PCS_NAD83_UTM_zone_3N 26903
#define PCS_NAD83_UTM_zone_23N 26923
#define PCS_WGS84_UTM_zone_1N 32601
#define PCS_WGS84_UTM_zone_60N 32660
#define PCS_WGS84_UTM_zone_1S 32701
#define PCS_WGS84_UTM_zone_60S 32760



typedef struct
{
  int32_t             min_x;
  int32_t             max_x;
  int32_t             min_y;
  int32_t             max_y;
  int32_t             range_x;
  int32_t             range_y;
  int32_t             length;
  int32_t             height;
  uint32_t            temporal_spacing;
  uint32_t            buffer_size;
} BOUNDS;


class LASwaveMonitor:public QMainWindow
{
  Q_OBJECT 


public:

  LASwaveMonitor (int32_t *argc = 0, char **argv = 0, QWidget *parent = 0);
  ~LASwaveMonitor ();


protected:

  int32_t         key;

  QSharedMemory   *abeShare;

  ABE_SHARE       *abe_share, l_share;

  uint32_t        kill_switch;

  BOUNDS          bounds;

  int32_t         width, height, window_x, window_y;

  char            filename[512], progname[256];

  int32_t         recnum;

  uint8_t         wave_line_mode, lock_track, endian;

  double          gps_start_time;  //  For LAS data using GPS time instead of GPS seconds of the week.

  double          las_time_offset;

  SLAS_POINT_DATA slas;

  LASheader       lasheader;

  std::vector<uint32_t> sample;

  int32_t         wave_read;

  QMessageBox     *filError;

  QStatusBar      *statusBar[8];

  QLabel          *dateLabel, *intensity, *returnNum, *numReturns, *classFlags, *synthetic, *keypoint, *withheld, *overlap, *scannerChan,
                  *scanDir, *edge, *classification, *userData, *scanAngle, *pointSource, *red, *green, *blue, *NIR, *wdi, *bowd, *wps,
                  *rpwl, *Xt, *Yt, *Zt, *curLoc;

  QColor          waveColor, primaryColor, backgroundColor;

  QFont           font;

  QPalette        bWavePalette, bPrimaryPalette, bBackgroundPalette;

  QPushButton     *bWaveColor, *bPrimaryColor, *bBackgroundColor, *bRestoreDefaults; 

  QButtonGroup    *bGrp;

  uint8_t         force_redraw;

  nvMap           *map;

  NVMAP_DEF       mapdef;

  uint32_t        ac[7];

  QDialog         *prefsD;

  QToolButton     *bQuit, *bPrefs, *bMode;

  QString         pos_format, parentName, acknowledgmentsText, string;


  void envin ();
  void envout ();

  void leftMouse (double x, double y);
  void midMouse (double x, double y);
  void rightMouse (double x, double y);
  void scaleWave (int32_t x, int32_t y, int32_t *new_x, int32_t *new_y, NVMAP_DEF l_mapdef, BOUNDS *bnds);
  void drawX (int32_t x, int32_t y, int32_t size, int32_t width, QColor color);
  void setFields ();


protected slots:

  void slotMousePress (QMouseEvent *e, double x, double y);
  void slotMouseMove (QMouseEvent *e, double x, double y);
  void slotResize (QResizeEvent *e);
  void closeEvent (QCloseEvent *event);
  void slotPlotWaves (NVMAP_DEF l_mapdef);

  void trackCursor ();

  void slotKeyPress (QKeyEvent *e);

  void slotHelp ();

  void slotQuit ();

  void slotMode (bool state);

  void slotPrefs ();
  void slotPosClicked (int id);
  void slotClosePrefs ();

  void slotWaveColor ();
  void slotPrimaryColor ();
  void slotBackgroundColor ();
  void slotRestoreDefaults ();

  void about ();
  void slotAcknowledgments ();
  void aboutQt ();


 private:
};

#endif
