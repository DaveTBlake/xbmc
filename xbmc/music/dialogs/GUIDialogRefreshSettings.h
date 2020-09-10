/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaType.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include <map>

class CFileItemList;

// Enumeration of what combination of items to apply the scraper settings
enum REFRESHAPPLYOPTIONS
{
  REFRESH_LIBRARY = 0x0000,
  REFRESH_ALLVIEW = 0x0001,
  REFRESH_THISITEM = 0x0002
};

class CGUIDialogRefreshSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogRefreshSettings();

  // specialization of CGUIWindow
  bool HasListItems() const override { return true; };

  /*! \brief Show dialog to refresh info and artwork for either artists or albums (not both).
   Has a list to select what is to be refreshed - all library, to just current item or to all the filtered items on the node.
  \param mediatype [in] Media type - artists or albums.
  \param flags [in/out] Media type - artists or albums.
  \param bHideApply [in] if true then dialog configured for call after changing information provider
  \return 0 refresh all artists/albums in library, 1 to all artists/albums on node, 2 to just the selected artist/album or -1 if dialog cancelled or error occurs
  */
  static int Show(const MediaType& mediatype, int& flags, int fixedApply = -1);

protected:
  // specializations of CGUIWindow
  void OnInitWindow() override;

  // implementations of ISettingCallback
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  void Save() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

private:
  void ResetDefaults();
  bool ChooseMergeOptions();

  MediaType m_mediaType = MediaTypeAlbum;
  bool m_bFixedApply = false;
  bool m_bSkipScraped = true;
  bool m_bReplaceAll = true;
  bool m_bIgnoreNFOFiles = false;
  bool m_bReplaceArt = false;
  std::string m_strOverwriteFields;
  unsigned int m_applyToItems = REFRESH_ALLVIEW;
};
