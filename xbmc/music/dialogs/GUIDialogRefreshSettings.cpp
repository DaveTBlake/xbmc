/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogRefreshSettings.h"

#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/infoscanner/MusicInfoScanner.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"

#include <limits.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

const std::string SETTING_APPLYTOITEMS = "applysettingstoitems";
const std::string SETTING_SKIPSCRAPED = "skipscraped";
const std::string SETTING_REPLACEALL = "replaceallfields";
const std::string SETTING_MERGEOPTIONS = "mergeoptions";
const std::string SETTING_INGORENFO = "ignorenfofiles";
const std::string SETTING_REPLACEART = "replaceart";

CGUIDialogRefreshSettings::CGUIDialogRefreshSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_MUSICREFRESH_SETTINGS, "DialogSettings.xml")
{
}

int CGUIDialogRefreshSettings::Show(const MediaType& mediatype, int& flags, int fixedApply /*=-1*/)
{
  CGUIDialogRefreshSettings* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogRefreshSettings>(
          WINDOW_DIALOG_MUSICREFRESH_SETTINGS);
  if (!dialog)
    return -1;
  if (mediatype != MediaTypeAlbum && mediatype != MediaTypeArtist)
    return -1;

  dialog->m_mediaType = mediatype;
  if (fixedApply >= 0)
  {
    dialog->m_bSkipScraped = false;
    dialog->m_bFixedApply = true;
    dialog->m_applyToItems = static_cast<unsigned int>(fixedApply);
  }
  dialog->Open();

  bool confirmed = dialog->IsConfirmed();
  unsigned int applyToItems = dialog->m_applyToItems;

  if (mediatype == MediaTypeAlbum)
    flags = MUSIC_INFO::CMusicInfoScanner::SCAN_ALBUMS;
  else
    flags = MUSIC_INFO::CMusicInfoScanner::SCAN_ARTISTS;
  if (!dialog->m_bSkipScraped || applyToItems == REFRESH_THISITEM)
    flags |= MUSIC_INFO::CMusicInfoScanner::SCAN_RESCAN;
  if (dialog->m_bIgnoreNFOFiles)
    flags |= MUSIC_INFO::CMusicInfoScanner::SCAN_INGORENFO;
  if (dialog->m_bReplaceArt)
    flags |= MUSIC_INFO::CMusicInfoScanner::SCAN_REPLACEART;
  if (!dialog->m_bReplaceAll)
    flags |= MUSIC_INFO::CMusicInfoScanner::SCAN_NOTMETADATA;

  dialog->ResetDefaults();

  if (confirmed)
    return applyToItems;
  else
    return -1;
}

void CGUIDialogRefreshSettings::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();
}

void CGUIDialogRefreshSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string& settingId = setting->GetId();

  if (settingId == SETTING_APPLYTOITEMS)
    m_applyToItems = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  else if (settingId == SETTING_SKIPSCRAPED)
    m_bSkipScraped = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_REPLACEALL)
    m_bReplaceAll = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_INGORENFO)
    m_bIgnoreNFOFiles = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  else if (settingId == SETTING_REPLACEART)
    m_bReplaceArt = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
}

void CGUIDialogRefreshSettings::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string& settingId = setting->GetId();
  if (settingId == SETTING_MERGEOPTIONS)
  {
    //!@Todo: Show album or artist merge settings dialog that doesn't exist yet
  }
}

void CGUIDialogRefreshSettings::Save()
{
  return; //Save done by caller of ::Show
}

void CGUIDialogRefreshSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);

  if (m_mediaType == MediaTypeAlbum)
    SetHeading(39149); // Refresh Albums
  else
    SetHeading(39148); // Refresh Artists
}

void CGUIDialogRefreshSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  std::shared_ptr<CSettingCategory> category = AddCategory("refreshsettings", -1);
  if (category == nullptr)
  {
    CLog::Log(LOGERROR, "%s: unable to setup settings", __FUNCTION__);
    return;
  }
  if (!m_bFixedApply)
  {
    std::shared_ptr<CSettingGroup> group = AddGroup(category);
    if (group == nullptr)
    {
      CLog::Log(LOGERROR, "%s: unable to setup settings", __FUNCTION__);
      return;
    }
    TranslatableIntegerSettingOptions entries;
    entries.clear();
    if (m_mediaType == MediaTypeAlbum)
    {
      entries.push_back(TranslatableIntegerSettingOption(39153, REFRESH_THISITEM));
      entries.push_back(TranslatableIntegerSettingOption(39154, REFRESH_ALLVIEW));
      entries.push_back(TranslatableIntegerSettingOption(39155, REFRESH_LIBRARY));
    }
    else
    {
      entries.push_back(TranslatableIntegerSettingOption(39150, REFRESH_THISITEM));
      entries.push_back(TranslatableIntegerSettingOption(39151, REFRESH_ALLVIEW));
      entries.push_back(TranslatableIntegerSettingOption(39152, REFRESH_LIBRARY));
    }
    // "Refresh..."
    AddList(group, SETTING_APPLYTOITEMS, 39157, SettingLevel::Basic, m_applyToItems, entries,
            39158);
  }

  // Create dependancies for settings enabled
  CSettingDependency dependency1(SettingDependencyType::Enable, GetSettingsManager());
  dependency1.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(
      SETTING_APPLYTOITEMS, std::to_string(REFRESH_THISITEM), SettingDependencyOperator::Equals,
      true, GetSettingsManager())));
  SettingDependencies depsApplyMany;
  depsApplyMany.push_back(dependency1);

  CSettingDependency dependency2(SettingDependencyType::Enable, GetSettingsManager());
  dependency2.Or()
      ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(
          SETTING_SKIPSCRAPED, "true", SettingDependencyOperator::Equals, true,
          GetSettingsManager())))
      ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(
          SETTING_APPLYTOITEMS, std::to_string(REFRESH_THISITEM), SettingDependencyOperator::Equals,
          false, GetSettingsManager())));
  SettingDependencies depsNotSkipOrSingle;
  depsNotSkipOrSingle.push_back(dependency2);

  CSettingDependency dependency3(SettingDependencyType::Visible, GetSettingsManager());
  dependency3.And()
      ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(
          SETTING_REPLACEALL, "true", SettingDependencyOperator::Equals, true,
          GetSettingsManager())))
      ->Add(CSettingDependencyConditionPtr(
          new CSettingDependencyCondition(SETTING_REPLACEALL, "", "", true, GetSettingsManager())));
  SettingDependencies depsReplace;
  depsReplace.push_back(dependency3);

  std::shared_ptr<CSettingGroup> group1;
  if (!m_bFixedApply)
    group1 = AddGroup(category, 38337);
  else if (m_mediaType == MediaTypeAlbum)
  {
    if (m_applyToItems == REFRESH_THISITEM)
      group1 = AddGroup(category, 39165);
    else
      group1 = AddGroup(category, 39166);
  }
  else
  {
    if (m_applyToItems == REFRESH_THISITEM)
      group1 = AddGroup(category, 39163);
    else
      group1 = AddGroup(category, 39164);
  }
  if (group1 == nullptr)
  {
    CLog::Log(LOGERROR, "%s: unable to setup settings", __FUNCTION__);
    return;
  }
  std::shared_ptr<CSettingBool> toggle;
  if (!m_bFixedApply)
  {
    if (m_mediaType == MediaTypeAlbum)
      toggle =
          AddToggle(group1, SETTING_SKIPSCRAPED, 39160, SettingLevel::Basic, m_bSkipScraped, true);
    else
      toggle =
          AddToggle(group1, SETTING_SKIPSCRAPED, 39159, SettingLevel::Basic, m_bSkipScraped, true);
    toggle->SetDependencies(depsApplyMany);
  }
  if (m_mediaType == MediaTypeAlbum)
    toggle =
        AddToggle(group1, SETTING_REPLACEALL, 39172, SettingLevel::Basic, m_bReplaceAll, false);
  else
    toggle =
        AddToggle(group1, SETTING_REPLACEALL, 39171, SettingLevel::Basic, m_bReplaceAll, false);
  if (!m_bFixedApply)
    toggle->SetDependencies(depsNotSkipOrSingle);

  // Action button for artist/album merge whitelists to replace "pref online info" setting
  std::shared_ptr<CSettingAction> subsetting;
  subsetting = AddButton(group1, SETTING_MERGEOPTIONS, 39173, SettingLevel::Basic);
  if (subsetting)
  {
    subsetting->SetParent(SETTING_REPLACEALL);
    subsetting->SetDependencies(depsReplace);
  }
  AddToggle(group1, SETTING_INGORENFO, 39161, SettingLevel::Basic, m_bIgnoreNFOFiles, false);
  toggle = AddToggle(group1, SETTING_REPLACEART, 39162, SettingLevel::Basic, m_bReplaceArt, false);
  if (!m_bFixedApply)
    toggle->SetDependencies(depsNotSkipOrSingle);
}

void CGUIDialogRefreshSettings::ResetDefaults()
{
  m_mediaType = MediaTypeAlbum;
  m_bFixedApply = false;
  m_applyToItems = REFRESH_ALLVIEW;
  m_bSkipScraped = true;
  m_bReplaceAll = true;
  m_bIgnoreNFOFiles = false;
  m_bReplaceArt = false;
}
