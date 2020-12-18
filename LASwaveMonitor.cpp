
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



#include "LASwaveMonitor.hpp"
#include "LASwaveMonitorHelp.hpp"


double settings_version = 2.01;


LASwaveMonitor::LASwaveMonitor (int32_t *argc, char **argv, QWidget * parent):
  QMainWindow (parent, 0)
{
  extern char     *optarg;


  strcpy (progname, argv[0]);
  filError = NULL;
  lock_track = NVFalse;

  endian = big_endian ();


  time_t                  gps_tv_sec;
  long                    gps_tv_nsec;


  //  Set the GPS start time (00:00:00 on 6 January 1980) in POSIX form.

  inv_cvtime (80, 6, 0, 0, 0.0, &gps_tv_sec, &gps_tv_nsec);
  gps_start_time = (int64_t) gps_tv_sec;



  QResource::registerResource ("/icons.rcc");


  //  Read the acknowledgments file for the Acknowledgments Help button.  This way I only have
  //  to change one file and copy it to the other programs icon folders to change the Acknowledgments
  //  Help text instead of changing it in every single program I use it in.

  QFile *aDataFile = new QFile (":/icons/ACKNOWLEDGMENTS");

  if (aDataFile->open (QIODevice::ReadOnly))
    {
      char string[256];

      while (aDataFile->readLine (string, sizeof (string)) > 0)
        {
	  acknowledgmentsText.append (string);
        }
      aDataFile->close ();
    }


  //  Have to set the focus policy or keypress events don't work properly at first in Focus Follows Mouse mode

  setFocusPolicy (Qt::WheelFocus);


  //  Set the main icon

  setWindowIcon (QIcon (":/icons/LASwaveMonitor.png"));


  kill_switch = ANCILLARY_FORCE_EXIT;

  int32_t option_index = 0;
  while (NVTrue) 
    {
      static struct option long_options[] = {{"actionkey00", required_argument, 0, 0},
                                             {"actionkey01", required_argument, 0, 0},
                                             {"actionkey02", required_argument, 0, 0},
                                             {"actionkey03", required_argument, 0, 0},
                                             {"shared_memory_key", required_argument, 0, 0},
                                             {"kill_switch", required_argument, 0, 0},
                                             {0, no_argument, 0, 0}};

      char c = (char) getopt_long (*argc, argv, "o", long_options, &option_index);
      if (c == -1) break;

      switch (c) 
        {
        case 0:

          //  The parent ID argument

          switch (option_index)
            {
            case 4:
              sscanf (optarg, "%d", &key);
              break;

            case 5:
              sscanf (optarg, "%d", &kill_switch);
              break;

            default:
              char tmp;
              sscanf (optarg, "%1c", &tmp);
              ac[option_index] = (uint32_t) tmp;
              break;
            }
          break;
        }
    }


  //  This is the "tools" toolbar.  We have to do this here so that we can restore the toolbar location(s).

  QToolBar *tools = addToolBar (tr ("Tools"));
  tools->setObjectName (tr ("LASwaveMonitor main toolbar"));


  envin ();


  // Set the application font

  QApplication::setFont (font);


  setWindowTitle (QString (VERSION));


  /******************************************* IMPORTANT NOTE ABOUT SHARED MEMORY **************************************** \

      This is a little note about the use of shared memory within the Area-Based Editor (ABE) programs.  If you read
      the Qt documentation (or anyone else's documentation) about the use of shared memory they will say "Dear [insert
      name of omnipotent being of your choice here], whatever you do, always lock shared memory when you use it!".
      The reason they say this is that access to shared memory is not atomic.  That is, reading shared memory and then
      writing to it is not a single operation.  An example of why this might be important - two programs are running,
      the first checks a value in shared memory, sees that it is a zero.  The second program checks the same location
      and sees that it is a zero.  These two programs have different actions they must perform depending on the value
      of that particular location in shared memory.  Now the first program writes a one to that location which was
      supposed to tell the second program to do something but the second program thinks it's a zero.  The second program
      doesn't do what it's supposed to do and it writes a two to that location.  The two will tell the first program 
      to do something.  Obviously this could be a problem.  In real life, this almost never occurs.  Also, if you write
      your program properly you can make sure this doesn't happen.  In ABE we almost never lock shared memory because
      something much worse than two programs getting out of sync can occur.  If we start a program and it locks shared
      memory and then dies, all the other programs will be locked up.  When you look through the ABE code you'll see
      that we very rarely lock shared memory, and then only for very short periods of time.  This is by design.

  \******************************************* IMPORTANT NOTE ABOUT SHARED MEMORY ****************************************/


  //  Get the shared memory area.  If it doesn't exist, quit.  It should have already been created 
  //  by pfmView or fileEdit3D.

  QString skey;
  skey.sprintf ("%d_abe", key);

  abeShare = new QSharedMemory (skey);

  if (!abeShare->attach (QSharedMemory::ReadWrite))
    {
      fprintf (stderr, "%s %s %s %d - abeShare - %s\n", progname, __FILE__, __FUNCTION__, __LINE__, strerror (errno));
      exit (-1);
    }

  abe_share = (ABE_SHARE *) abeShare->data ();


  //  Set the window size and location from the defaults

  this->resize (width, height);
  this->move (window_x, window_y);


  //  Set all of the default values.

  wave_read = 0;


  //  Set the map values from the defaults

  mapdef.projection = NO_PROJECTION;
  mapdef.draw_width = width;
  mapdef.draw_height = height;
  mapdef.overlap_percent = 5;
  mapdef.grid_inc_x = 0.0;
  mapdef.grid_inc_y = 0.0;

  mapdef.coasts = NVFalse;
  mapdef.landmask = NVFalse;

  mapdef.border = 0;
  mapdef.coast_color = Qt::white;
  mapdef.grid_color = QColor (160, 160, 160, 127);
  mapdef.background_color = backgroundColor;


  mapdef.initial_bounds.min_x = 0;
  mapdef.initial_bounds.min_y = 0;
  mapdef.initial_bounds.max_x = 300;
  mapdef.initial_bounds.max_y = 500;


  QFrame *frame = new QFrame (this, 0);

  setCentralWidget (frame);


  //  Make the map.

  map = new nvMap (this, &mapdef);
  map->setWhatsThis (mapText);


  //  Connect to the signals from the map class.

  connect (map, SIGNAL (mousePressSignal (QMouseEvent *, double, double)), this, SLOT (slotMousePress (QMouseEvent *, double, double)));
  connect (map, SIGNAL (mouseMoveSignal (QMouseEvent *, double, double)), this, SLOT (slotMouseMove (QMouseEvent *, double, double)));
  connect (map, SIGNAL (resizeSignal (QResizeEvent *)), this, SLOT (slotResize (QResizeEvent *)));
  connect (map, SIGNAL (postRedrawSignal (NVMAP_DEF)), this, SLOT (slotPlotWaves (NVMAP_DEF)));


  //  Layouts, what fun!

  QVBoxLayout *vBox = new QVBoxLayout (frame);


  vBox->addWidget (map);


  for (int32_t i = 0 ; i < 8 ; i++)
    {
      statusBar[i] = new QStatusBar (frame);
      statusBar[i]->setSizeGripEnabled (false);
      statusBar[i]->show ();
      vBox->addWidget (statusBar[i]);
    }


  dateLabel = new QLabel ("0000-00-00 (000) 00:00:00.00", this);
  dateLabel->setAlignment (Qt::AlignCenter);
  dateLabel->setMinimumSize (dateLabel->sizeHint ());
  dateLabel->setWhatsThis (dateLabelText);
  dateLabel->setToolTip (tr ("Date and time"));

  intensity = new QLabel (" 00000 ", this);
  intensity->setAlignment (Qt::AlignCenter);
  intensity->setMinimumSize (intensity->sizeHint ());
  intensity->setToolTip (tr ("Intensity"));

  returnNum = new QLabel (" 00 ", this);
  returnNum->setAlignment (Qt::AlignCenter);
  returnNum->setMinimumSize (returnNum->sizeHint ());
  returnNum->setToolTip (tr ("Return number"));

  numReturns = new QLabel (" 00 ", this);
  numReturns->setAlignment (Qt::AlignCenter);
  numReturns->setMinimumSize (numReturns->sizeHint ());
  numReturns->setToolTip (tr ("Number of returns"));

  synthetic = new QLabel (" 0 ", this);
  synthetic->setAlignment (Qt::AlignCenter);
  synthetic->setMinimumSize (synthetic->sizeHint ());
  synthetic->setWhatsThis (syntheticText);
  synthetic->setToolTip (tr ("Synthetic flag"));

  keypoint = new QLabel (" 0 ", this);
  keypoint->setAlignment (Qt::AlignCenter);
  keypoint->setMinimumSize (keypoint->sizeHint ());
  keypoint->setWhatsThis (keypointText);
  keypoint->setToolTip (tr ("Keypoint flag"));

  withheld = new QLabel (" 0 ", this);
  withheld->setAlignment (Qt::AlignCenter);
  withheld->setMinimumSize (withheld->sizeHint ());
  withheld->setWhatsThis (withheldText);
  withheld->setToolTip (tr ("Withheld flag"));

  overlap = new QLabel (" 0 ", this);
  overlap->setAlignment (Qt::AlignCenter);
  overlap->setMinimumSize (overlap->sizeHint ());
  overlap->setWhatsThis (overlapText);
  overlap->setToolTip (tr ("Overlap flag"));

  scannerChan = new QLabel (" 0 ", this);
  scannerChan->setAlignment (Qt::AlignCenter);
  scannerChan->setMinimumSize (scannerChan->sizeHint ());
  scannerChan->setWhatsThis (scannerChanText);
  scannerChan->setToolTip (tr ("Scanner channel"));

  scanDir = new QLabel (" 0 ", this);
  scanDir->setAlignment (Qt::AlignCenter);
  scanDir->setMinimumSize (scanDir->sizeHint ());
  scanDir->setWhatsThis (scanDirText);
  scanDir->setToolTip (tr ("Scan direction flag"));

  edge = new QLabel (" 0 ", this);
  edge->setAlignment (Qt::AlignCenter);
  edge->setMinimumSize (edge->sizeHint ());
  edge->setWhatsThis (edgeText);
  edge->setToolTip (tr ("Flightline edge flag"));

  classification = new QLabel (" 0 ", this);
  classification->setAlignment (Qt::AlignCenter);
  classification->setMinimumSize (classification->sizeHint ());
  classification->setToolTip (tr ("Classification"));

  userData = new QLabel (" 000 ", this);
  userData->setAlignment (Qt::AlignCenter);
  userData->setMinimumSize (userData->sizeHint ());
  userData->setToolTip (tr ("User data"));

  scanAngle = new QLabel (" 000 ", this);
  scanAngle->setAlignment (Qt::AlignCenter);
  scanAngle->setMinimumSize (scanAngle->sizeHint ());
  scanAngle->setToolTip (tr ("Scan angle"));

  pointSource = new QLabel (" 000 ", this);
  pointSource->setAlignment (Qt::AlignCenter);
  pointSource->setMinimumSize (pointSource->sizeHint ());
  pointSource->setToolTip (tr ("Point source ID"));

  red = new QLabel (" 00000 ", this);
  red->setAlignment (Qt::AlignCenter);
  red->setMinimumSize (red->sizeHint ());
  red->setToolTip (tr ("Red"));

  green = new QLabel (" 00000 ", this);
  green->setAlignment (Qt::AlignCenter);
  green->setMinimumSize (green->sizeHint ());
  green->setToolTip (tr ("Green"));

  blue = new QLabel (" 00000 ", this);
  blue->setAlignment (Qt::AlignCenter);
  blue->setMinimumSize (blue->sizeHint ());
  blue->setToolTip (tr ("Blue"));

  NIR = new QLabel (" 00000 ", this);
  NIR->setAlignment (Qt::AlignCenter);
  NIR->setMinimumSize (NIR->sizeHint ());
  NIR->setWhatsThis (NIRText);
  NIR->setToolTip (tr ("NIR"));

  wdi = new QLabel (" 000 ", this);
  wdi->setAlignment (Qt::AlignCenter);
  wdi->setMinimumSize (wdi->sizeHint ());
  wdi->setToolTip (tr ("Waveform descriptor index"));

  bowd = new QLabel (" 000000000000 ", this);
  bowd->setAlignment (Qt::AlignCenter);
  bowd->setMinimumSize (bowd->sizeHint ());
  bowd->setToolTip (tr ("Byte offset to waveform data"));

  wps = new QLabel (" 000000 ", this);
  wps->setAlignment (Qt::AlignCenter);
  wps->setMinimumSize (wps->sizeHint ());
  wps->setToolTip (tr ("Waveform packet size"));

  rpwl = new QLabel (" 000000.00 ", this);
  rpwl->setAlignment (Qt::AlignCenter);
  rpwl->setMinimumSize (rpwl->sizeHint ());
  rpwl->setToolTip (tr ("Return point waveform location"));

  Xt = new QLabel (" 00000.00 ", this);
  Xt->setAlignment (Qt::AlignCenter);
  Xt->setMinimumSize (Xt->sizeHint ());
  Xt->setToolTip (tr ("X(t)"));

  Yt = new QLabel (" 00000.00 ", this);
  Yt->setAlignment (Qt::AlignCenter);
  Yt->setMinimumSize (Yt->sizeHint ());
  Yt->setToolTip (tr ("Y(t)"));

  Zt = new QLabel (" 00000.00 ", this);
  Zt->setAlignment (Qt::AlignCenter);
  Zt->setMinimumSize (Zt->sizeHint ());
  Zt->setToolTip (tr ("Z(t)"));


  statusBar[0]->addWidget (dateLabel);
  statusBar[0]->addWidget (intensity);
  statusBar[0]->addWidget (returnNum);

  statusBar[1]->addWidget (numReturns);
  statusBar[1]->addWidget (synthetic);
  statusBar[1]->addWidget (keypoint);
  statusBar[1]->addWidget (withheld);
  statusBar[1]->addWidget (overlap);

  statusBar[2]->addWidget (scannerChan);
  statusBar[2]->addWidget (scanDir);
  statusBar[2]->addWidget (edge);

  statusBar[3]->addWidget (classification);
  statusBar[3]->addWidget (userData);
  statusBar[3]->addWidget (scanAngle);

  statusBar[4]->addWidget (pointSource);
  statusBar[4]->addWidget (red);
  statusBar[4]->addWidget (green);
  statusBar[4]->addWidget (blue);
  statusBar[4]->addWidget (NIR);

  statusBar[5]->addWidget (wdi);
  statusBar[5]->addWidget (bowd);

  statusBar[6]->addWidget (wps);
  statusBar[6]->addWidget (rpwl);

  statusBar[7]->addWidget (Xt);
  statusBar[7]->addWidget (Yt);
  statusBar[7]->addWidget (Zt);


  //  Button, button, who's got the buttons?

  bQuit = new QToolButton (this);
  bQuit->setIcon (QIcon (":/icons/quit.png"));
  bQuit->setToolTip (tr ("Quit"));
  bQuit->setWhatsThis (quitText);
  connect (bQuit, SIGNAL (clicked ()), this, SLOT (slotQuit ()));
  tools->addWidget (bQuit);


  bMode = new QToolButton (this);
  if (wave_line_mode)
    {
      bMode->setIcon (QIcon (":/icons/mode_line.png"));
    }
  else
    {
      bMode->setIcon (QIcon (":/icons/mode_dot.png"));
    }
  bMode->setToolTip (tr ("Toggle wave drawing mode between line and dots"));
  bMode->setWhatsThis (modeText);
  bMode->setCheckable (true);
  bMode->setChecked (wave_line_mode);
  connect (bMode, SIGNAL (toggled (bool)), this, SLOT (slotMode (bool)));
  tools->addWidget (bMode);


  bPrefs = new QToolButton (this);
  bPrefs->setIcon (QIcon (":/icons/prefs.png"));
  bPrefs->setToolTip (tr ("Change application preferences"));
  bPrefs->setWhatsThis (prefsText);
  connect (bPrefs, SIGNAL (clicked ()), this, SLOT (slotPrefs ()));
  tools->addWidget (bPrefs);


  tools->addSeparator ();
  tools->addSeparator ();


  QAction *bHelp = QWhatsThis::createAction (this);
  bHelp->setIcon (QIcon (":/icons/contextHelp.png"));
  tools->addAction (bHelp);



  //  Setup the file menu.

  QAction *fileQuitAction = new QAction (tr ("&Quit"), this);
  fileQuitAction->setShortcut (tr ("Ctrl+Q"));
  fileQuitAction->setStatusTip (tr ("Exit from application"));
  connect (fileQuitAction, SIGNAL (triggered ()), this, SLOT (slotQuit ()));


  QMenu *fileMenu = menuBar ()->addMenu (tr ("&File"));
  fileMenu->addAction (fileQuitAction);


  //  Setup the help menu.

  QAction *aboutAct = new QAction (tr ("&About"), this);
  aboutAct->setShortcut (tr ("Ctrl+A"));
  aboutAct->setStatusTip (tr ("Information about LASwaveMonitor"));
  connect (aboutAct, SIGNAL (triggered ()), this, SLOT (about ()));

  QAction *acknowledgments = new QAction (tr ("A&cknowledgments"), this);
  acknowledgments->setShortcut (tr ("Ctrl+c"));
  acknowledgments->setStatusTip (tr ("Information about supporting libraries"));
  connect (acknowledgments, SIGNAL (triggered ()), this, SLOT (slotAcknowledgments ()));

  QAction *aboutQtAct = new QAction (tr ("About&Qt"), this);
  aboutQtAct->setShortcut (tr ("Ctrl+Q"));
  aboutQtAct->setStatusTip (tr ("Information about Qt"));
  connect (aboutQtAct, SIGNAL (triggered ()), this, SLOT (aboutQt ()));

  QMenu *helpMenu = menuBar ()->addMenu (tr ("&Help"));
  helpMenu->addAction (aboutAct);
  helpMenu->addSeparator ();
  helpMenu->addAction (acknowledgments);
  helpMenu->addAction (aboutQtAct);


  map->setCursor (Qt::ArrowCursor);

  map->enableSignals ();


  QTimer *track = new QTimer (this);
  connect (track, SIGNAL (timeout ()), this, SLOT (trackCursor ()));
  track->start (10);
}



LASwaveMonitor::~LASwaveMonitor ()
{
}



void 
LASwaveMonitor::closeEvent (QCloseEvent *event __attribute__ ((unused)))
{
  slotQuit ();
}



void 
LASwaveMonitor::slotResize (QResizeEvent *e __attribute__ ((unused)))
{
  force_redraw = NVTrue;
}



void
LASwaveMonitor::slotHelp ()
{
  QWhatsThis::enterWhatsThisMode ();
}



void 
LASwaveMonitor::leftMouse (double x __attribute__ ((unused)), double y __attribute__ ((unused)))
{
  //  Placeholder
}



void 
LASwaveMonitor::midMouse (double x __attribute__ ((unused)), double y __attribute__ ((unused)))
{
  //  Placeholder
}



void 
LASwaveMonitor::rightMouse (double x __attribute__ ((unused)), double y __attribute__ ((unused)))
{
  //  Placeholder
}



//  Signal from the map class.

void 
LASwaveMonitor::slotMousePress (QMouseEvent * e, double x, double y)
{
  if (e->button () == Qt::LeftButton) leftMouse (x, y);
  if (e->button () == Qt::MidButton) midMouse (x, y);
  if (e->button () == Qt::RightButton) rightMouse (x, y);
}



//  Signal from the map class.

void
LASwaveMonitor::slotMouseMove (QMouseEvent *e __attribute__ ((unused)), double x __attribute__ ((unused)), double y __attribute__ ((unused)))
{
  //Placeholder
}



//  Timer - timeout signal.  Very much like an X workproc.

void
LASwaveMonitor::trackCursor ()
{
  static uint32_t         prev_rec = -1;
  static ABE_SHARE        l_share;
  FILE                    *las_fp = NULL, *las_wfp = NULL;
  LASreadOpener           lasreadopener;
  LASreader               *lasreader;
  SLAS_WAVEFORM_PACKET_DESCRIPTOR slas_wf_packet_desc[255];


  //  Since this is always a child process of something we want to exit if we see the CHILD_PROCESS_FORCE_EXIT key.
  //  We also want to exit on the ANCILLARY_FORCE_EXIT key (from pfmEdit) or if our own personal kill signal
  //  has been placed in abe_share->key.

  if (abe_share->key == CHILD_PROCESS_FORCE_EXIT || abe_share->key == ANCILLARY_FORCE_EXIT ||
      abe_share->key == kill_switch) slotQuit ();


  //  We want to exit if we have locked the tracker to update our saved waveforms (in slotPlotWaves).

  if (lock_track) return;


  //  Locking makes sure another process does not have memory locked.  It will block until it can lock it.
  //  At that point we copy the contents and then unlock it so other processes can continue.

  abeShare->lock ();


  //  Check for change of record and correct record type

  uint8_t hit = NVFalse;
  if (prev_rec != abe_share->mwShare.multiRecord[0] && abe_share->mwShare.multiType[0] == PFM_LAS_DATA)
    {
      l_share = *abe_share;
      prev_rec = l_share.mwShare.multiRecord[0];
      hit = NVTrue;
    }


  if (abe_share->modcode == WAVEMONITOR_FORCE_REDRAW) force_redraw = NVTrue;


  abeShare->unlock ();


  //  Hit or force redraw from above.

  if (hit || force_redraw)
    {
      force_redraw = NVFalse;

 
      //  Save for slotPlotWaves.

      recnum = l_share.mwShare.multiRecord[0];
      strcpy (filename, l_share.nearest_filename);


      //  Open the LAS file with LASlib and read the header.

      lasreadopener.set_file_name (filename);
      lasreader = lasreadopener.open ();
      if (!lasreader)
        {
          if (filError) filError->close ();
          filError = new QMessageBox (QMessageBox::Warning, "LASwaveMonitor", tr ("Error opening ") + 
                                      QDir::toNativeSeparators (QString (filename)) + " : " + 
                                      QString (strerror (errno)), QMessageBox::NoButton, this, 
                                      (Qt::WindowFlags) Qt::WA_DeleteOnClose);
          filError->show ();
          return;
        }


      lasheader = lasreader->header;


      if (lasheader.version_major != 1)
        {
          lasreader->close ();
          if (filError) filError->close ();
          string.sprintf ("\nLAS major version %d incorrect, file %s : %s %s %d\n\n", lasheader.version_major, filename, __FILE__, __FUNCTION__, __LINE__);
          filError = new QMessageBox (QMessageBox::Warning, "LASwaveMonitor", string, QMessageBox::NoButton, this, 
                                      (Qt::WindowFlags) Qt::WA_DeleteOnClose);
          filError->show ();
          return;
        }


      if (lasheader.version_minor != 3 && lasheader.version_minor != 4)
        {
          lasreader->close ();
          string.sprintf ("\nLAS minor version %d incorrect, file %s : %s %s %d\n\n", lasheader.version_minor, filename, __FILE__, __FUNCTION__, __LINE__);
          filError = new QMessageBox (QMessageBox::Warning, "LASwaveMonitor", string, QMessageBox::NoButton, this, 
                                      (Qt::WindowFlags) Qt::WA_DeleteOnClose);
          filError->show ();
          return;
        }


      //  No waveforms.

      if (!(lasheader.global_encoding & 0x6))
        {
          lasreader->close ();

          string = tr ("<b>No waveforms!</b>");
          dateLabel->setText (string);

          return;
        }


      //  Look for the waveform information in the VLRs.

      for (int32_t i = 0 ; i < (int32_t) lasheader.number_of_variable_length_records ; i++)
        {
          if (lasheader.vlrs[i].record_id > 99 && lasheader.vlrs[i].record_id < 355)
            {
              LASvlr_wave_packet_descr *vlr_wave_packet_descr = (LASvlr_wave_packet_descr *) lasheader.vlrs[i].data;

              int32_t ndx = lasheader.vlrs[i].record_id - 99;
              slas_wf_packet_desc[ndx].bits_per_sample = vlr_wave_packet_descr->getBitsPerSample ();
              slas_wf_packet_desc[ndx].compression_type = vlr_wave_packet_descr->getCompressionType ();
              slas_wf_packet_desc[ndx].number_of_samples = vlr_wave_packet_descr->getNumberOfSamples ();
              slas_wf_packet_desc[ndx].temporal_spacing = vlr_wave_packet_descr->getTemporalSpacing ();
              slas_wf_packet_desc[ndx].digitizer_gain = vlr_wave_packet_descr->getDigitizerGain ();
              slas_wf_packet_desc[ndx].digitizer_offset = vlr_wave_packet_descr->getDigitizerOffset ();
            }
        }


      //  Now close it since all we really wanted was the header.

      lasreader->close ();


      if ((las_fp = fopen64 (filename, "rb")) == NULL)
        {
          string = tr ("Error opening ") + QDir::toNativeSeparators (QString (filename)) + " : " + QString (strerror (errno));
          filError = new QMessageBox (QMessageBox::Warning, "LASwaveMonitor", string, QMessageBox::NoButton, this, 
                                      (Qt::WindowFlags) Qt::WA_DeleteOnClose);
          filError->show ();
          return;
        }


      slas_read_point_data (las_fp, l_share.mwShare.multiRecord[0] - 1, &lasheader, endian, &slas);


      //  If the waveforms are external...

      if (lasheader.global_encoding & 0x4)
        {
          QString waveName = QString (filename).replace (".las", ".wdp");
          char wave_name[1024];
          strcpy (wave_name, waveName.toLatin1 ());

          if ((las_wfp = fopen64 (wave_name, "rb")) == NULL)
            {
              string = tr ("Error opening ") + QDir::toNativeSeparators (QString (wave_name)) + " : " + QString (strerror (errno));
              filError = new QMessageBox (QMessageBox::Warning, "LASwaveMonitor", string, QMessageBox::NoButton, this, 
                                          (Qt::WindowFlags) Qt::WA_DeleteOnClose);
              filError->show ();
              fclose (las_fp);
              return;
            }
        }


      //  Otherwise, the waveforms are internal (since we already checked global_encoding to make sure there were waveforms).

      else
        {
          las_wfp = las_fp;
        }


      if (slas.wavepacket_descriptor_index && (lasheader.point_data_format == 4 || lasheader.point_data_format == 5 ||
                                               lasheader.point_data_format == 9 || lasheader.point_data_format == 10))
        {
          int32_t ndx = slas.wavepacket_descriptor_index;
          bounds.length = slas_wf_packet_desc[ndx].number_of_samples;
          bounds.temporal_spacing = slas_wf_packet_desc[ndx].temporal_spacing;


          bounds.min_y = 0;
          bounds.height = bounds.max_y = NINT (pow (2.0, (double) slas_wf_packet_desc[ndx].bits_per_sample));
          bounds.min_x = 0;
          bounds.max_x = bounds.length;


          //  Add pixels to the X axis.

          bounds.range_x = bounds.max_x - bounds.min_x;
          bounds.buffer_size = 25;
          bounds.min_x = bounds.min_x - bounds.buffer_size;
          bounds.max_x = bounds.max_x + 10;
          bounds.range_x = bounds.max_x - bounds.min_x;
          bounds.range_y = bounds.max_y - bounds.min_y;


          //  Add pixels to the Y axis.

          bounds.range_y = bounds.max_y - bounds.min_y;
          bounds.buffer_size = 25;
          bounds.min_y = bounds.min_y - bounds.buffer_size;
          bounds.max_y = bounds.max_y + 10;
          bounds.range_y = bounds.max_y - bounds.min_y;
          bounds.range_y = bounds.max_y - bounds.min_y;

          try
            {
              sample.resize (bounds.length);
            }
          catch (std::bad_alloc&)
            {
              fclose (las_fp);
              fprintf (stderr, "%s %s %s %d - sample - %s\n", progname, __FILE__, __FUNCTION__, __LINE__, strerror (errno));
              abeShare->detach ();
              exit (-1);
            }

          slas_read_waveform_data (las_wfp, &lasheader, &slas, slas_wf_packet_desc, sample.data ());

          wave_read = NVTrue;
        }
      else
        {
          fclose (las_fp);
          if (lasheader.global_encoding & 0x4) fclose (las_wfp);
          return;
        }


      fclose (las_fp);
      if (lasheader.global_encoding & 0x4) fclose (las_wfp);


      l_share.key = 0;
      abe_share->key = 0;
      abe_share->modcode = PFM_LAS_DATA;


      map->redrawMapArea (NVTrue);
    }
}



//  Signal from the map class.

void 
LASwaveMonitor::slotKeyPress (QKeyEvent *e)
{
  char key[20];
  strcpy (key, e->text ().toLatin1 ());

  if (key[0] == (char) ac[0])
    {
      force_redraw = NVTrue;
    }


  if (key[0] == (char) ac[1])
    {
      force_redraw = NVTrue;
    }


  if (key[0] == (char) ac[2])
    {
      force_redraw = NVTrue;
    }


  if (key[0] == (char) ac[3])
    {
      force_redraw = NVTrue;
    }
}



//  A bunch of slots.

void 
LASwaveMonitor::slotQuit ()
{
  //  Let the parent program know that we have died from something other than the kill switch from the parent.

  if (abe_share->key != kill_switch) abe_share->killed = kill_switch;


  envout ();


  //  Let go of the shared memory.

  abeShare->detach ();


  exit (0);
}



void 
LASwaveMonitor::slotMode (bool on)
{
  wave_line_mode = on;

  if (on)
    {
      bMode->setIcon (QIcon (":/icons/mode_line.png"));
    }
  else
    {
      bMode->setIcon (QIcon (":/icons/mode_dot.png"));
    }

  force_redraw = NVTrue;
}



void 
LASwaveMonitor::setFields ()
{
  if (pos_format == "hdms") bGrp->button (0)->setChecked (true);
  if (pos_format == "hdm") bGrp->button (1)->setChecked (true);
  if (pos_format == "hd") bGrp->button (2)->setChecked (true);
  if (pos_format == "dms") bGrp->button (3)->setChecked (true);
  if (pos_format == "dm") bGrp->button (4)->setChecked (true);
  if (pos_format == "d") bGrp->button (5)->setChecked (true);


  int32_t hue, sat, val;

  waveColor.getHsv (&hue, &sat, &val);
  if (val < 128)
    {
      bWavePalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::white);
      bWavePalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::white);
    }
  else
    {
      bWavePalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::black);
      bWavePalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::black);
    }
  bWavePalette.setColor (QPalette::Normal, QPalette::Button, waveColor);
  bWavePalette.setColor (QPalette::Inactive, QPalette::Button, waveColor);
  bWaveColor->setPalette (bWavePalette);


  primaryColor.getHsv (&hue, &sat, &val);
  if (val < 128)
    {
      bPrimaryPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::white);
      bPrimaryPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::white);
    }
  else
    {
      bPrimaryPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::black);
      bPrimaryPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::black);
    }
  bPrimaryPalette.setColor (QPalette::Normal, QPalette::Button, primaryColor);
  bPrimaryPalette.setColor (QPalette::Inactive, QPalette::Button, primaryColor);
  bPrimaryColor->setPalette (bPrimaryPalette);


  backgroundColor.getHsv (&hue, &sat, &val);
  if (val < 128)
    {
      bBackgroundPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::white);
      bBackgroundPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::white);
    }
  else
    {
      bBackgroundPalette.setColor (QPalette::Normal, QPalette::ButtonText, Qt::black);
      bBackgroundPalette.setColor (QPalette::Inactive, QPalette::ButtonText, Qt::black);
    }
  bBackgroundPalette.setColor (QPalette::Normal, QPalette::Button, backgroundColor);
  bBackgroundPalette.setColor (QPalette::Inactive, QPalette::Button, backgroundColor);
  bBackgroundColor->setPalette (bBackgroundPalette);
}



void
LASwaveMonitor::slotPrefs ()
{
  prefsD = new QDialog (this, (Qt::WindowFlags) Qt::WA_DeleteOnClose);
  prefsD->setWindowTitle (tr ("LASwaveMonitor Preferences"));
  prefsD->setModal (true);

  QVBoxLayout *vbox = new QVBoxLayout (prefsD);
  vbox->setMargin (5);
  vbox->setSpacing (5);

  QGroupBox *fbox = new QGroupBox (tr ("Position Format"), prefsD);
  fbox->setWhatsThis (bGrpText);

  QRadioButton *hdms = new QRadioButton (tr ("Hemisphere Degrees Minutes Seconds.decimal"));
  QRadioButton *hdm_ = new QRadioButton (tr ("Hemisphere Degrees Minutes.decimal"));
  QRadioButton *hd__ = new QRadioButton (tr ("Hemisphere Degrees.decimal"));
  QRadioButton *sdms = new QRadioButton (tr ("+/-Degrees Minutes Seconds.decimal"));
  QRadioButton *sdm_ = new QRadioButton (tr ("+/-Degrees Minutes.decimal"));
  QRadioButton *sd__ = new QRadioButton (tr ("+/-Degrees.decimal"));

  bGrp = new QButtonGroup (prefsD);
  bGrp->setExclusive (true);
  connect (bGrp, SIGNAL (buttonClicked (int)), this, SLOT (slotPosClicked (int)));

  bGrp->addButton (hdms, 0);
  bGrp->addButton (hdm_, 1);
  bGrp->addButton (hd__, 2);
  bGrp->addButton (sdms, 3);
  bGrp->addButton (sdm_, 4);
  bGrp->addButton (sd__, 5);

  QHBoxLayout *fboxSplit = new QHBoxLayout;
  QVBoxLayout *fboxLeft = new QVBoxLayout;
  QVBoxLayout *fboxRight = new QVBoxLayout;
  fboxSplit->addLayout (fboxLeft);
  fboxSplit->addLayout (fboxRight);
  fboxLeft->addWidget (hdms);
  fboxLeft->addWidget (hdm_);
  fboxLeft->addWidget (hd__);
  fboxRight->addWidget (sdms);
  fboxRight->addWidget (sdm_);
  fboxRight->addWidget (sd__);
  fbox->setLayout (fboxSplit);

  vbox->addWidget (fbox, 1);

  if (pos_format == "hdms") bGrp->button (0)->setChecked (true);
  if (pos_format == "hdm") bGrp->button (1)->setChecked (true);
  if (pos_format == "hd") bGrp->button (2)->setChecked (true);
  if (pos_format == "dms") bGrp->button (3)->setChecked (true);
  if (pos_format == "dm") bGrp->button (4)->setChecked (true);
  if (pos_format == "d") bGrp->button (5)->setChecked (true);


  QGroupBox *cbox = new QGroupBox (tr ("Colors"), this);
  QVBoxLayout *cboxLayout = new QVBoxLayout;
  cbox->setLayout (cboxLayout);
  QHBoxLayout *cboxTopLayout = new QHBoxLayout;
  QHBoxLayout *cboxBottomLayout = new QHBoxLayout;
  cboxLayout->addLayout (cboxTopLayout);
  cboxLayout->addLayout (cboxBottomLayout);


  bWaveColor = new QPushButton (tr ("Waveform"), this);
  bWaveColor->setToolTip (tr ("Change waveform color"));
  bWaveColor->setWhatsThis (bWaveColor->toolTip ());
  connect (bWaveColor, SIGNAL (clicked ()), this, SLOT (slotWaveColor ()));
  bWavePalette = bWaveColor->palette ();
  cboxTopLayout->addWidget (bWaveColor);


  bPrimaryColor = new QPushButton (tr ("Return point"), this);
  bPrimaryColor->setToolTip (tr ("Change return point location marker color"));
  bPrimaryColor->setWhatsThis (bPrimaryColor->toolTip ());
  bPrimaryPalette = bPrimaryColor->palette ();
  connect (bPrimaryColor, SIGNAL (clicked ()), this, SLOT (slotPrimaryColor ()));
  cboxBottomLayout->addWidget (bPrimaryColor);


  bBackgroundColor = new QPushButton (tr ("Background"), this);
  bBackgroundColor->setToolTip (tr ("Change display background color"));
  bBackgroundColor->setWhatsThis (bBackgroundColor->toolTip ());
  bBackgroundPalette = bBackgroundColor->palette ();
  connect (bBackgroundColor, SIGNAL (clicked ()), this, SLOT (slotBackgroundColor ()));
  cboxBottomLayout->addWidget (bBackgroundColor);


  vbox->addWidget (cbox, 1);


  QHBoxLayout *actions = new QHBoxLayout (0);
  vbox->addLayout (actions);

  QPushButton *bHelp = new QPushButton (prefsD);
  bHelp->setIcon (QIcon (":/icons/contextHelp.png"));
  bHelp->setToolTip (tr ("Enter What's This mode for help"));
  connect (bHelp, SIGNAL (clicked ()), this, SLOT (slotHelp ()));
  actions->addWidget (bHelp);

  actions->addStretch (10);

  bRestoreDefaults = new QPushButton (tr ("Restore Defaults"), this);
  bRestoreDefaults->setToolTip (tr ("Restore all preferences to the default state"));
  bRestoreDefaults->setWhatsThis (restoreDefaultsText);
  connect (bRestoreDefaults, SIGNAL (clicked ()), this, SLOT (slotRestoreDefaults ()));
  actions->addWidget (bRestoreDefaults);

  QPushButton *closeButton = new QPushButton (tr ("Close"), prefsD);
  closeButton->setToolTip (tr ("Close the preferences dialog"));
  connect (closeButton, SIGNAL (clicked ()), this, SLOT (slotClosePrefs ()));
  actions->addWidget (closeButton);


  setFields ();


  prefsD->show ();
}



void
LASwaveMonitor::slotPosClicked (int id)
{
  switch (id)
    {
    case 0:
    default:
      pos_format = "hdms";
      break;

    case 1:
      pos_format = "hdm";
      break;

    case 2:
      pos_format = "hd";
      break;

    case 3:
      pos_format = "dms";
      break;

    case 4:
      pos_format = "dm";
      break;

    case 5:
      pos_format = "d";
      break;
    }
}



void
LASwaveMonitor::slotClosePrefs ()
{
  prefsD->close ();
}



void
LASwaveMonitor::slotWaveColor ()
{
  QColor clr; 
  clr = QColorDialog::getColor (waveColor, this, tr ("LASwaveMonitor Waveform Color"), QColorDialog::ShowAlphaChannel);
  if (clr.isValid ()) waveColor = clr;

  setFields ();

  force_redraw = NVTrue;
}



void
LASwaveMonitor::slotPrimaryColor ()
{
  QColor clr;

  clr = QColorDialog::getColor (primaryColor, this, tr ("LASwaveMonitor Return Point Marker Color"),
                                QColorDialog::ShowAlphaChannel);

  if (clr.isValid ()) primaryColor = clr;

  setFields ();

  force_redraw = NVTrue;
}



void
LASwaveMonitor::slotBackgroundColor ()
{
  QColor clr;

  clr = QColorDialog::getColor (backgroundColor, this, tr ("LASwaveMonitor Background Color"));

  if (clr.isValid ()) backgroundColor = clr;

  setFields ();

  force_redraw = NVTrue;
}



void 
LASwaveMonitor::scaleWave (int32_t x, int32_t y, int32_t *new_x, int32_t *new_y, NVMAP_DEF l_mapdef, BOUNDS *bnds)
{
  *new_y = NINT (((float) (x - bnds->min_x) / (float) bnds->range_x) * (float) l_mapdef.draw_height);
  *new_x = NINT (((float) (y - bnds->min_y) / (float) bnds->range_y) * (float) l_mapdef.draw_width);
}



void 
LASwaveMonitor::slotPlotWaves (NVMAP_DEF l_mapdef)
{
  static std::vector<uint32_t> save_sample;
  static SLAS_POINT_DATA  save_slas;
  static QString          save_name;
  static LASheader        save_lasheader;
  static BOUNDS           save_bounds;
  int32_t                 pix_x[2], pix_y[2];
  QString                 stat;
  int64_t                 las_timestamp, tv_sec;
  int32_t                 year, day, mday, month, hour, minute;
  float                   second;


  if (!wave_read) return;


  //  Because the trackCursor function may be changing the data while we're still plotting it we save it
  //  to this static structure.  lock_track stops trackCursor from updating while we're trying to get an
  //  atomic snapshot of the data for the latest point.

  lock_track = NVTrue;

  try
    {
      save_sample.resize (bounds.length);
    }
  catch (std::bad_alloc&)
    {
      fprintf (stderr, "%s %s %s %d - save_sample - %s\n", progname, __FILE__, __FUNCTION__, __LINE__, strerror (errno));
      abeShare->detach ();
      exit (-1);
    }

  save_sample = sample;
  sample.clear ();
  save_slas = slas;
  save_lasheader = lasheader;
  save_bounds = bounds;

  lock_track = NVFalse;


  //  Draw the axes

  int32_t inc_x = bounds.length / 5;
  int32_t inc_y = bounds.height / 5;
  int32_t x_tics = bounds.length / inc_x + 1;
  int32_t y_tics = bounds.height / inc_y + 1;

  scaleWave (0, 0, &pix_x[0], &pix_y[0], l_mapdef, &save_bounds);
  scaleWave (0, bounds.length, &pix_x[1], &pix_y[1], l_mapdef, &save_bounds);

  map->drawLine (pix_x[0], pix_y[0], pix_x[1], pix_y[1], Qt::gray, 2, NVFalse, Qt::SolidLine);

  scaleWave (0, 0, &pix_x[0], &pix_y[0], l_mapdef, &save_bounds);
  scaleWave (bounds.height, 0, &pix_x[1], &pix_y[1], l_mapdef, &save_bounds);

  map->drawLine (pix_x[0], pix_y[0], pix_x[1], pix_y[1], Qt::gray, 2, NVFalse, Qt::SolidLine);

  for (int32_t i = 0 ; i < x_tics ; i++)
    {
      int32_t num = i * inc_x;

      scaleWave (num, 0, &pix_x[0], &pix_y[0], l_mapdef, &save_bounds);
      scaleWave (num, -2, &pix_x[1], &pix_y[1], l_mapdef, &save_bounds);

      map->drawLine (pix_x[0], pix_y[0], pix_x[1], pix_y[1], Qt::gray, 2, NVFalse, Qt::SolidLine);

      QString number;
      number.setNum (num);
      scaleWave (num + 4, -22, &pix_x[1], &pix_y[1], l_mapdef, &save_bounds);
      map->drawText (number, pix_x[1], pix_y[1], 90.0, 8, Qt::gray, NVTrue);
    }

  for (int32_t i = 0 ; i < y_tics ; i++)
    {
      int32_t num = i * inc_y;

      scaleWave (0, num, &pix_x[0], &pix_y[0], l_mapdef, &save_bounds);
      scaleWave (-2, num, &pix_x[1], &pix_y[1], l_mapdef, &save_bounds);

      map->drawLine (pix_x[0], pix_y[0], pix_x[1], pix_y[1], Qt::gray, 2, NVFalse, Qt::SolidLine);

      QString number;
      number.setNum (num);
      scaleWave (-10, num - 10, &pix_x[1], &pix_y[1], l_mapdef, &save_bounds);
      map->drawText (number, pix_x[1], pix_y[1], 90.0, 8, Qt::gray, NVTrue);
    }


  //  Draw the waveform.

  scaleWave (1, save_sample[0], &pix_x[0], &pix_y[0], l_mapdef, &save_bounds);

  for (int32_t i = 1 ; i < save_bounds.length ; i++)
    {
      scaleWave (i, save_sample[i], &pix_x[1], &pix_y[1], l_mapdef, &save_bounds);

      if (wave_line_mode)
        {
          map->drawLine (pix_x[0], pix_y[0], pix_x[1], pix_y[1], waveColor, 2, NVFalse, Qt::SolidLine);
        }
      else
        {
          map->fillRectangle (pix_x[0], pix_y[0], SPOT_SIZE, SPOT_SIZE, waveColor, NVFalse);
        }
      pix_x[0] = pix_x[1];
      pix_y[0] = pix_y[1];
    }


  //  Figure out where the return location is on the waveform.

  int32_t bin = save_slas.return_point_waveform_location / save_bounds.temporal_spacing;

  scaleWave (bin, save_sample[bin], &pix_x[0], &pix_y[0], l_mapdef, &save_bounds);
  drawX (pix_x[0], pix_y[0], 10, 2, primaryColor);


  save_sample.clear ();


  //  Set the status bar labels

  if (save_lasheader.point_data_format != 2)
    {
      if (save_lasheader.global_encoding & 0x01)
        {
	  //  Note, GPS time is ahead of UTC time by some number of leap seconds depending on the date of the survey.
	  //  The leap seconds that are relevant for CHARTS and/or CZMIL data are as follows
	  //
	  //  December 31 2005 23:59:59 - 1136073599 - 14 seconds ahead
	  //  December 31 2008 23:59:59 - 1230767999 - 15 seconds ahead
	  //  June 30 2012 23:59:59     - 1341100799 - 16 seconds ahead
	  //  June 30 2015 23:59:59     - 1435708799 - 17 seconds ahead

	  las_time_offset = 1000000000.0 - 13.0;

	  tv_sec = slas.gps_time + gps_start_time + las_time_offset;
	  if (tv_sec > 1136073599) las_time_offset -= 1.0;
	  if (tv_sec > 1230767999) las_time_offset -= 1.0;
	  if (tv_sec > 1341100799) las_time_offset -= 1.0;
	  if (tv_sec > 1435708799) las_time_offset -= 1.0;

          las_timestamp = (int64_t) ((slas.gps_time + gps_start_time + las_time_offset) * 1000000.0);
          time_t tv_sec = las_timestamp / 1000000;
          long tv_nsec = las_timestamp % 1000000;

          cvtime (tv_sec, tv_nsec, &year, &day, &hour, &minute, &second);
          jday2mday (year, day, &month, &mday);
          month++;

          string.sprintf ("<b>Date/time : </b>%d-%02d-%02d (%03d) %02d:%02d:%05.2f", year + 1900, month, mday, day, hour, minute, second);
        }
      else
        {
          string.sprintf ("<b>GPS seconds : </b>%.6f", save_slas.gps_time);
        }
    }

  dateLabel->setText (string);

  string.sprintf ("<b>Intensity : </b>%hd", save_slas.intensity);
  intensity->setText (string);

  string.sprintf ("<b>Return : </b>%d", save_slas.return_number);
  returnNum->setText (string);

  string.sprintf ("<b>Number of returns : </b>%d", save_slas.number_of_returns);
  numReturns->setText (string);

  string.sprintf ("<b>Synthetic : </b>%d", save_slas.synthetic);
  synthetic->setText (string);

  string.sprintf ("<b>Keypoint : </b>%d", save_slas.keypoint);
  keypoint->setText (string);

  string.sprintf ("<b>Withheld : </b>%d", save_slas.withheld);
  withheld->setText (string);

  string.sprintf ("<b>Overlap : </b>%d", save_slas.overlap);
  overlap->setText (string);

  if (save_lasheader.point_data_format > 5)
    {
      string.sprintf ("<b>Scanner channel : </b>%d", save_slas.scanner_channel);
    }
  else
    {
      string = "<b>Scanner channel : </b>N/A";
    }
  scannerChan->setText (string);

  string.sprintf ("<b>Scan direction flag : </b>%d", save_slas.scan_direction_flag);
  scanDir->setText (string);

  string.sprintf ("<b>Edge of flightline flag : </b>%d", save_slas.edge_of_flightline);
  edge->setText (string);

  string.sprintf ("<b>Classification : </b>%d", save_slas.classification);
  classification->setText (string);

  string.sprintf ("<b>User data : </b>%d", save_slas.user_data);
  userData->setText (string);

  string.sprintf ("<b>Scan angle : </b>%d", save_slas.scan_angle);
  scanAngle->setText (string);

  string.sprintf ("<b>Point source ID : </b>%d", save_slas.point_source_id);
  pointSource->setText (string);

  QString R, G, B;

  if (save_lasheader.point_data_format == 2 || save_lasheader.point_data_format == 3 || save_lasheader.point_data_format == 5 ||
      save_lasheader.point_data_format == 7 || save_lasheader.point_data_format == 8 || save_lasheader.point_data_format == 10)
    {
      R.sprintf ("<b>Red : </b>%hd", save_slas.red);
      G.sprintf ("<b>Green : </b>%hd", save_slas.green);
      B.sprintf ("<b>Blue : </b>%hd", save_slas.blue);
    }
  else
    {
      R = "<b>Red : </b>N/A";
      G = "<b>Green : </b>N/A";
      B = "<b>Blue : </b>N/A";
    }

  red->setText (R);
  green->setText (G);
  blue->setText (B);

  if (save_lasheader.point_data_format == 8 || save_lasheader.point_data_format == 10)
    {
      string.sprintf ("<b>NIR : </b>%hd", save_slas.NIR);
    }
  else
    {
      string = "<b>NIR : </b>N/A";
    }
  NIR->setText (string);

  QString wdi_s, bowd_s, wps_s, rpwl_s, Xt_s, Yt_s, Zt_s;

  if (save_lasheader.point_data_format == 4 || save_lasheader.point_data_format == 5 || save_lasheader.point_data_format == 9 ||
      save_lasheader.point_data_format == 10)
    {
      wdi_s.sprintf ("<b>Waveform descriptor index : </b>%d", save_slas.wavepacket_descriptor_index);

#ifdef _WIN32
      bowd_s.sprintf ("<b>Byte offset to waveforms : </b>%I64d", save_slas.byte_offset_to_waveform_data);
#else
      bowd_s.sprintf ("<b>Byte offset to waveforms : </b>%ld", save_slas.byte_offset_to_waveform_data);
#endif
      wps_s.sprintf ("<b>Waveform packet size : </b>%d", save_slas.waveform_packet_size);
      rpwl_s.sprintf ("<b>Return point waveform location : </b>%.2f", save_slas.return_point_waveform_location);
      Xt_s.sprintf ("<b>X(t) : </b>%.11f", save_slas.Xt);
      Yt_s.sprintf ("<b>Y(t) : </b>%.11f", save_slas.Yt);
      Zt_s.sprintf ("<b>Z(t) : </b>%.11f", save_slas.Zt);
    }
  else
    {
      wdi_s = "<b>Waveform descriptor index : </b>N/A";
      bowd_s = "<b>Byte offset to waveforms : </b>N/A";
      wps_s = "<b>Waveform packet size : </b>N/A";
      rpwl_s = "<b>Return point waveform location : </b>N/A";
      Xt_s = "<b>X(t) : </b>N/A";
      Yt_s = "<b>Y(t) : </b>N/A";
      Zt_s = "<b>Z(t) : </b>N/A<";
    }

  wdi->setText (wdi_s);
  bowd->setText (bowd_s);
  wps->setText (wps_s);
  rpwl->setText (rpwl_s);
  Xt->setText (Xt_s);
  Yt->setText (Yt_s);
  Zt->setText (Zt_s);
}



void 
LASwaveMonitor::drawX (int32_t x, int32_t y, int32_t size, int32_t width, QColor color)
{
  int32_t hs = size / 2;

  map->drawLine (x - hs, y + hs, x + hs, y - hs, color, width, NVTrue, Qt::SolidLine);
  map->drawLine (x + hs, y + hs, x - hs, y - hs, color, width, NVTrue, Qt::SolidLine);
}



void
LASwaveMonitor::slotRestoreDefaults ()
{
  static uint8_t first = NVTrue;

  pos_format = "hdms";
  width = WAVE_X_SIZE;
  height = WAVE_Y_SIZE;
  wave_line_mode = NVTrue;
  window_x = 0;
  window_y = 0;
  waveColor = Qt::white;
  primaryColor = Qt::green;
  backgroundColor = Qt::black;


  //  The first time will be called from envin and the prefs dialog won't exist yet.

  if (!first) setFields ();
  first = NVFalse;

  force_redraw = NVTrue;
}



void
LASwaveMonitor::about ()
{
  QMessageBox::about (this, VERSION,
                      "LASwaveMonitor - LAS wave form monitor."
                      "\n\nAuthor : Jan C. Depner (area.based.editor@gmail.com)");
}


void
LASwaveMonitor::slotAcknowledgments ()
{
  QMessageBox::about (this, VERSION, acknowledgmentsText);
}



void
LASwaveMonitor::aboutQt ()
{
  QMessageBox::aboutQt (this, VERSION);
}



//  Get the users defaults.

void
LASwaveMonitor::envin ()
{
  //  We need to get the font from the global settings.

#ifdef NVWIN3X
  QString ini_file2 = QString (getenv ("USERPROFILE")) + "/ABE.config/" + "globalABE.ini";
#else
  QString ini_file2 = QString (getenv ("HOME")) + "/ABE.config/" + "globalABE.ini";
#endif

  font = QApplication::font ();

  QSettings settings2 (ini_file2, QSettings::IniFormat);
  settings2.beginGroup ("globalABE");


  QString defaultFont = font.toString ();
  QString fontString = settings2.value (QString ("ABE map GUI font"), defaultFont).toString ();
  font.fromString (fontString);


  settings2.endGroup ();


  double saved_version = 0.0;


  // Set Defaults so the if keys don't exist the parms are defined

  slotRestoreDefaults ();
  force_redraw = NVFalse;


  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/LASwaveMonitor.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/LASwaveMonitor.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("LASwaveMonitor");

  saved_version = settings.value (tr ("settings version"), saved_version).toDouble ();


  //  If the settings version has changed we need to leave the values at the new defaults since they may have changed.

  if (settings_version != saved_version) return;


  pos_format = settings.value (tr ("position form"), pos_format).toString ();

  width = settings.value (tr ("width"), width).toInt ();

  height = settings.value (tr ("height"), height).toInt ();

  window_x = settings.value (tr ("window x"), window_x).toInt ();

  window_y = settings.value (tr ("window y"), window_y).toInt ();


  wave_line_mode = settings.value (tr ("Wave line mode flag"), wave_line_mode).toBool ();

  int32_t r = settings.value (tr ("Wave color/red"), waveColor.red ()).toInt ();
  int32_t g = settings.value (tr ("Wave color/green"), waveColor.green ()).toInt ();
  int32_t b = settings.value (tr ("Wave color/blue"), waveColor.blue ()).toInt ();
  int32_t a = settings.value (tr ("Wave color/alpha"), waveColor.alpha ()).toInt ();
  waveColor.setRgb (r, g, b, a);

  r = settings.value (tr ("Return point marker color/red"), primaryColor.red ()).toInt ();
  g = settings.value (tr ("Return point marker color/green"), primaryColor.green ()).toInt ();
  b = settings.value (tr ("Return point marker color/blue"), primaryColor.blue ()).toInt ();
  a = settings.value (tr ("Return point marker color/alpha"), primaryColor.alpha ()).toInt ();
  primaryColor.setRgb (r, g, b, a);

  r = settings.value (tr ("Background color/red"), backgroundColor.red ()).toInt ();
  g = settings.value (tr ("Background color/green"), backgroundColor.green ()).toInt ();
  b = settings.value (tr ("Background color/blue"), backgroundColor.blue ()).toInt ();
  a = settings.value (tr ("Background color/alpha"), backgroundColor.alpha ()).toInt ();
  backgroundColor.setRgb (r, g, b, a);

  settings.endGroup ();
}




//  Save the users defaults.

void
LASwaveMonitor::envout ()
{
  //  Use frame geometry to get the absolute x and y.

  QRect tmp = this->frameGeometry ();

  window_x = tmp.x ();
  window_y = tmp.y ();


  //  Use geometry to get the width and height.

  tmp = this->geometry ();
  width = tmp.width ();
  height = tmp.height ();


  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/LASwaveMonitor.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/LASwaveMonitor.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("LASwaveMonitor");


  settings.setValue (tr ("settings version"), settings_version);

  settings.setValue (tr ("position form"), pos_format);

  settings.setValue (tr ("width"), width);

  settings.setValue (tr ("height"), height);

  settings.setValue (tr ("window x"), window_x);

  settings.setValue (tr ("window y"), window_y);


  settings.setValue (tr ("Wave line mode flag"), wave_line_mode);


  settings.setValue (tr ("Wave color/red"), waveColor.red ());
  settings.setValue (tr ("Wave color/green"), waveColor.green ());
  settings.setValue (tr ("Wave color/blue"), waveColor.blue ());
  settings.setValue (tr ("Wave color/alpha"), waveColor.alpha ());

  settings.setValue (tr ("Return point marker color/red"), primaryColor.red ());
  settings.setValue (tr ("Return point marker color/green"), primaryColor.green ());
  settings.setValue (tr ("Return point marker color/blue"), primaryColor.blue ());
  settings.setValue (tr ("Return point marker color/alpha"), primaryColor.alpha ());

  settings.setValue (tr ("Background color/red"), backgroundColor.red ());
  settings.setValue (tr ("Background color/green"), backgroundColor.green ());
  settings.setValue (tr ("Background color/blue"), backgroundColor.blue ());
  settings.setValue (tr ("Background color/alpha"), backgroundColor.alpha ());

  settings.endGroup ();
}
