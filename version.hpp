
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


#ifndef VERSION

#define     VERSION     "PFM Software - LASwaveMonitor V1.18 - 06/23/17"

#endif

/*! <pre>

    Version 1.00
    Jan C. Depner
    03/21/15

    First working version.


    Version 1.01
    Jan C. Depner
    03/24/15

    Instead of throwing up reams of QMessageBox dialogs when their are no internal waveforms it
    now just writes "No internal waveforms" in the date/time slot of the first status bar.


    Version 1.02
    Jan C. Depner (PFM Software)
    03/27/15

    - Added slas_update_point_data to slas.cpp.


    Version 1.10
    Jan C. Depner (PFM Software)
    03/30/15

    - Now supports external (*.wdp) waveform files.


    Version 1.11
    Jan C. Depner (PFM Software)
    04/09/15

    - Modified slas.cpp to use extended_number_of_point_records for LAS v1.4 files.


    Version 1.12
    Jan C. Depner (PFM Software)
    06/27/15

    - Fixed PROJ4 init problem.


    Version 1.13
    Jan C. Depner (PFM Software)
    07/03/15

    - Finally straightened out the GPS time/leap second problem (I hope).


    Version 1.14
    Jan C. Depner (PFM Software)
    07/09/15

    - Now handles Riegl LAS files that don't have GeographicTypeGeoKey and ProjectedCSTypeGeoKey defined in
      the GeoKeyDirectoryTag (34735) required VLR.


    Version 1.15
    Jan C. Depner (PFM Software)
    05/02/16

    - Removed all of the coordinate system checking.  We don't care about that, we're just displaying waveforms.  The 
      coordinate system checking was done in pfmLoader.


    Version 1.16
    Jan C. Depner (PFM Software)
    08/26/16

    - Now uses the same font as all other ABE GUI apps.  Font can only be changed in pfmView Preferences.


    Version 1.17
    Jan C. Depner (PFM Software)
    10/22/16

    - Supports 1.3 as well as 1.4.


    Version 1.18
    Jan C. Depner (PFM Software)
    06/23/17

    -  Removed redundant functions from slas.cpp that are available in the nvutility library.

</pre>*/
