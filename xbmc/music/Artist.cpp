/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Artist.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/XMLUtils.h"

#include <algorithm>

void CArtist::MergeScrapedArtist(const CArtist& source, bool override /* = true */)
{
  /*
  Initial scraping of artist information when the mbid is derived from tags is done directly
  using that ID, otherwise the lookup is based on name and can mis-identify the artist
  (many have same name). It is useful to store the scraped mbid, but we need to be
  able to correct any mistakes. Hence a manual refresh of artist information uses either
  the mbid is derived from tags or the artist name, not any previously scraped mbid.

   A Musicbrainz artist ID derived from music file tags is always taken as accurate and so can
   not be overwritten by a scraped value. When the artist does not already have an mbid or has
   a previously scraped mbid, merge the new scraped value, flagging it as being from the
   scraper rather than derived from music file tags.
   */
  if (!source.strMusicBrainzArtistID.empty() && (strMusicBrainzArtistID.empty() || bScrapedMBID))
  {
    strMusicBrainzArtistID = source.strMusicBrainzArtistID;
    bScrapedMBID = true;
  }

  if ((override && !source.strArtist.empty()) || strArtist.empty())
    strArtist = source.strArtist;

  if ((override && !source.strSortName.empty()) || strSortName.empty())
    strSortName = source.strSortName;

  strType = source.strType;
  strGender = source.strGender;
  strDisambiguation = source.strDisambiguation;
  genre = source.genre;
  strBiography = source.strBiography;
  styles = source.styles;
  moods = source.moods;
  instruments = source.instruments;
  strBorn = source.strBorn;
  strFormed = source.strFormed;
  strDied = source.strDied;
  strDisbanded = source.strDisbanded;
  yearsActive = source.yearsActive;

  thumbURL = source.thumbURL; // Available remote thumbs
  fanart = source.fanart;  // Available remote fanart
  // Current artwork - thumb, fanart etc., to be stored in art table
  if (!source.art.empty())
    art = source.art;

  discography = source.discography;
}

void CArtist::MergeScrapedArtist(const CArtist& source, const std::string& strReplaceFields)
{
  /*
  Initial scraping of artist information when the mbid is derived from tags is done directly
  using that ID, otherwise the lookup is based on name and can mis-identify the artist
  (many have same name). It is useful to store the scraped mbid, but we need to be
  able to correct any mistakes. Hence a manual refresh of artist information uses either
  the mbid is derived from tags or the artist name, not any previously scraped mbid.

   A Musicbrainz artist ID derived from music file tags is always taken as accurate and so can
   not be overwritten by a scraped value. When the artist does not already have an mbid or has
   a previously scraped mbid, merge the new scraped value, flagging it as being from the
   scraper rather than derived from music file tags.
   */
  if (!source.strMusicBrainzArtistID.empty() && (strMusicBrainzArtistID.empty() || bScrapedMBID))
  {
    strMusicBrainzArtistID = source.strMusicBrainzArtistID;
    bScrapedMBID = true;
  }

  // Only full empty artist name and sortname values
  if (strArtist.empty())
    strArtist = source.strArtist;
  if (strSortName.empty())
    strSortName = source.strSortName;

  bool bType(true);
  bool bGender(true);
  bool bDisambiguation(true);
  bool bGenre(true);
  bool bBio(true);
  bool bStyles(true);
  bool bMoods(true);
  bool bInstruments(true);
  bool bBorn(true); 
  bool bFormed(true);
  bool bDied(true); 
  bool bDisbanded(true);
  bool bYearsActive(true);
  bool bArtURL(true);
  bool bDiscography(true);
  
  std::vector<std::string> fields;
  if (!strReplaceFields.empty())
  {
    bType = false;
    bGender = false;
    bDisambiguation = false;
    bGenre = false;
    bBio = false;
    bStyles = false;
    bMoods = false;
    bInstruments = false;
    bBorn = false;
    bFormed = false;
    bDied = false;
    bDisbanded = false;
    bYearsActive = false;
    bArtURL = false;
    bDiscography = false;
    fields = StringUtils::Split(strReplaceFields, ",");
    for (const auto& f : fields)
    {
      if (f == "type")
        bType = true;
      else if (f == "gender")
        bGender = true;
      else if (f == "disambiguation")
        bDisambiguation = true;
      else if (f == "genre")
        bGenre = true;
      else if (f == "biography")
        bBio = true;
      else if (f == "styles")
        bStyles = true;
      else if (f == "moods")
        bMoods = true;
      else if (f == "instruments")
        bInstruments = true;
      else if (f == "born")
        bBorn = true;
      else if (f == "formed")
        bFormed = true;
      else if (f == "died")
        bDied = true;
      else if (f == "disbanded")
        bDisbanded = true;
      else if (f == "yearsactive")
        bYearsActive = true;
      else if (f == "art")
        bArtURL = true;
      else if (f == "Discography")
      bDiscography = true;
    }
  }

  if (bType || strType.empty())
    strType = source.strType;
  if (bGender || strGender.empty())
    strGender = source.strGender;
  if (bDisambiguation || strDisambiguation.empty())
    strDisambiguation = source.strDisambiguation;
  if (bGenre || genre.empty())
    genre = source.genre;
  if (bBio || strBiography.empty())
    strBiography = source.strBiography;
  if (bStyles || styles.empty())
    styles = source.styles;
  if (bMoods || moods.empty())
    moods = source.moods;
  if (bInstruments || instruments.empty())
    instruments = source.instruments;
  if (bBorn || strBorn.empty())
    strBorn = source.strBorn;
  if (bFormed || strFormed.empty())
    strFormed = source.strFormed;
  if (bDied || strDied.empty())
    strDied = source.strDied;
  if (bDisbanded || strDisbanded.empty())
    strDisbanded = source.strDisbanded;
  if (bYearsActive || yearsActive.empty())
    yearsActive = source.yearsActive;
  if (bArtURL || !thumbURL.HasData())
    thumbURL = source.thumbURL; // Available remote thumbs
  if (bArtURL || fanart.GetNumFanarts() == 0)
    fanart = source.fanart;  // Available remote fanart  
  if (bDiscography || discography.empty())
    discography = source.discography;
}

bool CArtist::Load(const TiXmlElement *artist, bool append, bool prioritise)
{
  if (!artist) return false;
  if (!append)
    Reset();

  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  XMLUtils::GetString(artist,                "name", strArtist);
  XMLUtils::GetString(artist, "musicBrainzArtistID", strMusicBrainzArtistID);
  XMLUtils::GetString(artist,            "sortname", strSortName);
  XMLUtils::GetString(artist, "type", strType);
  XMLUtils::GetString(artist, "gender", strGender);
  XMLUtils::GetString(artist, "disambiguation", strDisambiguation);
  XMLUtils::GetStringArray(artist,       "genre", genre, prioritise, itemSeparator);
  XMLUtils::GetStringArray(artist,       "style", styles, prioritise, itemSeparator);
  XMLUtils::GetStringArray(artist,        "mood", moods, prioritise, itemSeparator);
  XMLUtils::GetStringArray(artist, "yearsactive", yearsActive, prioritise, itemSeparator);
  XMLUtils::GetStringArray(artist, "instruments", instruments, prioritise, itemSeparator);

  XMLUtils::GetString(artist,      "born", strBorn);
  XMLUtils::GetString(artist,    "formed", strFormed);
  XMLUtils::GetString(artist, "biography", strBiography);
  XMLUtils::GetString(artist,      "died", strDied);
  XMLUtils::GetString(artist, "disbanded", strDisbanded);

  size_t iThumbCount = thumbURL.GetUrls().size();
  std::string xmlAdd = thumbURL.GetData();

  // Available artist thumbs
  const TiXmlElement* thumb = artist->FirstChildElement("thumb");
  while (thumb)
  {
    thumbURL.ParseAndAppendUrl(thumb);
    if (prioritise)
    {
      std::string temp;
      temp << *thumb;
      xmlAdd = temp+xmlAdd;
    }
    thumb = thumb->NextSiblingElement("thumb");
  }
  // prefix thumbs from nfos
  if (prioritise && iThumbCount && iThumbCount != thumbURL.GetUrls().size())
  {
    auto thumbUrls = thumbURL.GetUrls();
    rotate(thumbUrls.begin(), thumbUrls.begin() + iThumbCount, thumbUrls.end());
    thumbURL.SetUrls(thumbUrls);
    thumbURL.SetData(xmlAdd);
  }

  // Discography
  const TiXmlElement* node = artist->FirstChildElement("album");
  if (node)
    discography.clear();
  while (node)
  {
    if (node->FirstChild())
    {
      CDiscoAlbum album;
      XMLUtils::GetString(node, "title", album.strAlbum);
      XMLUtils::GetString(node, "year", album.strYear);
      XMLUtils::GetString(node, "musicbrainzreleasegroupid", album.strReleaseGroupMBID);
      discography.push_back(album);
    }
    node = node->NextSiblingElement("album");
  }

  // Available artist fanart
  const TiXmlElement *fanart2 = artist->FirstChildElement("fanart");
  if (fanart2)
  {
    // we prefix to handle mixed-mode nfo's with fanart set
    if (prioritise)
    {
      std::string temp;
      temp << *fanart2;
      fanart.m_xml = temp+fanart.m_xml;
    }
    else
      fanart.m_xml << *fanart2;
    fanart.Unpack();
  }

 // Current artwork  - thumb, fanart etc. (the chosen art, not the lists of those available)
  node = artist->FirstChildElement("art");
  if (node)
  {
    const TiXmlNode *artdetailNode = node->FirstChild();
    while (artdetailNode && artdetailNode->FirstChild())
    {
      art.insert(make_pair(artdetailNode->ValueStr(), artdetailNode->FirstChild()->ValueStr()));
      artdetailNode = artdetailNode->NextSibling();
    }
  }

  return true;
}

bool CArtist::Save(TiXmlNode *node, const std::string &tag, const std::string& strPath)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement artistElement(tag.c_str());
  TiXmlNode *artist = node->InsertEndChild(artistElement);

  if (!artist) return false;

  XMLUtils::SetString(artist,                      "name", strArtist);
  XMLUtils::SetString(artist,       "musicBrainzArtistID", strMusicBrainzArtistID);
  XMLUtils::SetString(artist,                  "sortname", strSortName);
  XMLUtils::SetString(artist,                      "type", strType);
  XMLUtils::SetString(artist,                    "gender", strGender);
  XMLUtils::SetString(artist,            "disambiguation", strDisambiguation);
  XMLUtils::SetStringArray(artist,                "genre", genre);
  XMLUtils::SetStringArray(artist,                "style", styles);
  XMLUtils::SetStringArray(artist,                 "mood", moods);
  XMLUtils::SetStringArray(artist,          "yearsactive", yearsActive);
  XMLUtils::SetStringArray(artist,          "instruments", instruments);
  XMLUtils::SetString(artist,                      "born", strBorn);
  XMLUtils::SetString(artist,                    "formed", strFormed);
  XMLUtils::SetString(artist,                 "biography", strBiography);
  XMLUtils::SetString(artist,                      "died", strDied);
  XMLUtils::SetString(artist,                 "disbanded", strDisbanded);
  // Available thumbs
  if (thumbURL.HasData())
  {
    CXBMCTinyXML doc;
    doc.Parse(thumbURL.GetData());
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      artist->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  XMLUtils::SetString(artist,        "path", strPath);
  // Available fanart
  if (fanart.m_xml.size())
  {
    CXBMCTinyXML doc;
    doc.Parse(fanart.m_xml);
    artist->InsertEndChild(*doc.RootElement());
  }

  // Discography
  for (const auto& it : discography)
  {
    // add a <album> tag
    TiXmlElement discoElement("album");
    TiXmlNode* node = artist->InsertEndChild(discoElement);
    XMLUtils::SetString(node, "title", it.strAlbum);
    XMLUtils::SetString(node, "year", it.strYear);
    XMLUtils::SetString(node, "musicbrainzreleasegroupid", it.strReleaseGroupMBID);
  }

  return true;
}

void CArtist::SetDateAdded(const std::string& strDateAdded)
{
  dateAdded.SetFromDBDateTime(strDateAdded);
}

void CArtist::SetDateUpdated(const std::string& strDateUpdated)
{
  dateUpdated.SetFromDBDateTime(strDateUpdated);
}

void CArtist::SetDateNew(const std::string& strDateNew)
{
  dateNew.SetFromDBDateTime(strDateNew);
}

