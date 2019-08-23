/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SmartPlayList.h"

#include "Util.h"
#include "dbwrappers/Database.h"
#include "filesystem/File.h"
#include "filesystem/SmartPlaylistDirectory.h"
#include "guilib/LocalizeStrings.h"
#include "utils/DatabaseUtils.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StreamDetails.h"
#include "utils/StringUtils.h"
#include "utils/StringValidation.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace XFILE;

typedef struct
{
  char string[17];
  Field field;
  CDatabaseQueryRule::FIELD_TYPE type;
  StringValidation::Validator validator;
  bool browseable;
  int localizedString;
} translateField;

// clang-format off
static const translateField fields[] = {
  { "none",              FieldNone,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 231 },
  { "filename",          FieldFilename,                CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 561 },
  { "path",              FieldPath,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  573 },
  { "album",             FieldAlbum,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  558 },
  { "albumartist",       FieldAlbumArtist,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  566 },
  { "artist",            FieldArtist,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  557 },
  { "tracknumber",       FieldTrackNumber,             CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 554 },
  { "role",              FieldRole,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true, 38033 },
  { "comment",           FieldComment,                 CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 569 },
  { "review",            FieldReview,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 183 },
  { "themes",            FieldThemes,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21895 },
  { "moods",             FieldMoods,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 175 },
  { "styles",            FieldStyles,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 176 },
  { "type",              FieldAlbumType,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 564 },
  { "compilation",       FieldCompilation,             CDatabaseQueryRule::BOOLEAN_FIELD,  NULL,                                 false, 204 },
  { "label",             FieldMusicLabel,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21899 },
  { "title",             FieldTitle,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  556 },
  { "sorttitle",         FieldSortTitle,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 171 },
  { "originaltitle",     FieldOriginalTitle,           CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 20376 },
  { "year",              FieldYear,                    CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  562 },
  { "time",              FieldTime,                    CDatabaseQueryRule::SECONDS_FIELD,  StringValidation::IsTime,             false, 180 },
  { "playcount",         FieldPlaycount,               CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 567 },
  { "lastplayed",        FieldLastPlayed,              CDatabaseQueryRule::DATE_FIELD,     NULL,                                 false, 568 },
  { "inprogress",        FieldInProgress,              CDatabaseQueryRule::BOOLEAN_FIELD,  NULL,                                 false, 575 },
  { "rating",            FieldRating,                  CDatabaseQueryRule::REAL_FIELD,     CSmartPlaylistRule::ValidateRating,   false, 563 },
  { "userrating",        FieldUserRating,              CDatabaseQueryRule::REAL_FIELD,     CSmartPlaylistRule::ValidateMyRating, false, 38018 },
  { "votes",             FieldVotes,                   CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 205 },
  { "top250",            FieldTop250,                  CDatabaseQueryRule::NUMERIC_FIELD,  NULL,                                 false, 13409 },
  { "mpaarating",        FieldMPAA,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 20074 },
  { "dateadded",         FieldDateAdded,               CDatabaseQueryRule::DATE_FIELD,     NULL,                                 false, 570 },
  { "genre",             FieldGenre,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  515 },
  { "plot",              FieldPlot,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 207 },
  { "plotoutline",       FieldPlotOutline,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 203 },
  { "tagline",           FieldTagline,                 CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 202 },
  { "set",               FieldSet,                     CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20457 },
  { "director",          FieldDirector,                CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20339 },
  { "actor",             FieldActor,                   CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20337 },
  { "writers",           FieldWriter,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20417 },
  { "airdate",           FieldAirDate,                 CDatabaseQueryRule::DATE_FIELD,     NULL,                                 false, 20416 },
  { "hastrailer",        FieldTrailer,                 CDatabaseQueryRule::BOOLEAN_FIELD,  NULL,                                 false, 20423 },
  { "studio",            FieldStudio,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  572 },
  { "country",           FieldCountry,                 CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  574 },
  { "tvshow",            FieldTvShowTitle,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20364 },
  { "status",            FieldTvShowStatus,            CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 126 },
  { "season",            FieldSeason,                  CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20373 },
  { "episode",           FieldEpisodeNumber,           CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  false, 20359 },
  { "numepisodes",       FieldNumberOfEpisodes,        CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 20360 },
  { "numwatched",        FieldNumberOfWatchedEpisodes, CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21457 },
  { "videoresolution",   FieldVideoResolution,         CDatabaseQueryRule::REAL_FIELD,     NULL,                                 false, 21443 },
  { "videocodec",        FieldVideoCodec,              CDatabaseQueryRule::TEXTIN_FIELD,   NULL,                                 false, 21445 },
  { "videoaspect",       FieldVideoAspectRatio,        CDatabaseQueryRule::REAL_FIELD,     NULL,                                 false, 21374 },
  { "audiochannels",     FieldAudioChannels,           CDatabaseQueryRule::REAL_FIELD,     NULL,                                 false, 21444 },
  { "audiocodec",        FieldAudioCodec,              CDatabaseQueryRule::TEXTIN_FIELD,   NULL,                                 false, 21446 },
  { "audiolanguage",     FieldAudioLanguage,           CDatabaseQueryRule::TEXTIN_FIELD,   NULL,                                 false, 21447 },
  { "audiocount",        FieldAudioCount,              CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21481 },
  { "subtitlecount",     FieldSubtitleCount,           CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 21482 },
  { "subtitlelanguage",  FieldSubtitleLanguage,        CDatabaseQueryRule::TEXTIN_FIELD,   NULL,                                 false, 21448 },
  { "random",            FieldRandom,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 590 },
  { "playlist",          FieldPlaylist,                CDatabaseQueryRule::PLAYLIST_FIELD, NULL,                                 true,  559 },
  { "virtualfolder",     FieldVirtualFolder,           CDatabaseQueryRule::PLAYLIST_FIELD, NULL,                                 true,  614 },
  { "tag",               FieldTag,                     CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  20459 },
  { "instruments",       FieldInstruments,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21892 },
  { "biography",         FieldBiography,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21887 },
  { "born",              FieldBorn,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21893 },
  { "bandformed",        FieldBandFormed,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21894 },
  { "disbanded",         FieldDisbanded,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21896 },
  { "died",              FieldDied,                    CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21897 },
  { "artisttype",        FieldArtistType,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 564 },
  { "gender",            FieldGender,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 39025 },
  { "disambiguation",    FieldDisambiguation,          CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 39026 },
  { "source",            FieldSource,                  CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 true,  39030 },
  { "disctitle", FieldDiscTitle, CDatabaseQueryRule::TEXT_FIELD, NULL, false, 38076 },
  { "isboxset", FieldIsBoxset, CDatabaseQueryRule::BOOLEAN_FIELD, NULL, false, 38074 },
  { "totaldiscs", FieldTotalDiscs, CDatabaseQueryRule::NUMERIC_FIELD,
    StringValidation::IsPositiveInteger,  false, 38077 },
  { "artistid",          FieldArtistId,                CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  0 },
  { "albumid",           FieldAlbumId,                 CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  0 },
  { "songid",            FieldSongId,                  CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  0 },
  { "sourceid",          FieldSourceId,                CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  0 },
  { "genreid",           FieldGenreId,                 CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  0 },
  { "artist genre",      FieldArtistGenre,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false,  515 },
  { "artist scraped",    FieldArtistLastScrape,        CDatabaseQueryRule::DATE_FIELD,     NULL,                                 false, 0 },
  { "artist mbid",       FieldArtistMBId,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false,  0 },
  { "artist moods",      FieldArtistMoods,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 175 },
  { "born/formed",       FieldBornFormed,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21893 },
  { "died/disbanded",    FieldDiedDisband,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 21897 },
  { "years active",      FieldYearsActive,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false,  0 },
  { "album genre",       FieldAlbumGenre,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false,  515 },
  { "album scraped",     FieldAlbumLastScrape,         CDatabaseQueryRule::DATE_FIELD,     NULL,                                 false, 0 },
  { "album mbid",        FieldAlbumMBId,               CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false,  0 },
  { "releasegroup id",   FieldReleaseGroupMBId,        CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false,  0 },
  { "album moods",       FieldAlbumMoods,              CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 175 },
  { "album styles",      FieldAlbumStyles,             CDatabaseQueryRule::TEXT_FIELD,     NULL,                                 false, 176 },
  { "album rating",      FieldAlbumRating,             CDatabaseQueryRule::REAL_FIELD,     CSmartPlaylistRule::ValidateRating,   false, 563 },
  { "album userrating",  FieldAlbumUserRating,         CDatabaseQueryRule::REAL_FIELD,     CSmartPlaylistRule::ValidateMyRating, false, 38018 },
  { "album votes",       FieldAlbumVotes,              CDatabaseQueryRule::REAL_FIELD,     StringValidation::IsPositiveInteger,  false, 205 },
  { "album year",        FieldAlbumYear,               CDatabaseQueryRule::NUMERIC_FIELD,  StringValidation::IsPositiveInteger,  true,  562 },
};
// clang-format on

typedef struct
{
  std::string name;
  Field field;
  bool canMix;
  int localizedString;
} group;

static const group groups[] = { { "",           FieldUnknown,   false,    571 },
                                { "none",       FieldNone,      false,    231 },
                                { "sets",       FieldSet,       true,   20434 },
                                { "genres",     FieldGenre,     false,    135 },
                                { "years",      FieldYear,      false,    652 },
                                { "actors",     FieldActor,     false,    344 },
                                { "directors",  FieldDirector,  false,  20348 },
                                { "writers",    FieldWriter,    false,  20418 },
                                { "studios",    FieldStudio,    false,  20388 },
                                { "countries",  FieldCountry,   false,  20451 },
                                { "artists",    FieldArtist,    false,    133 },
                                { "albums",     FieldAlbum,     false,    132 },
                                { "tags",       FieldTag,       false,  20459 },
                              };

#define RULE_VALUE_SEPARATOR  " / "

CSmartPlaylistRule::CSmartPlaylistRule() = default;

int CSmartPlaylistRule::TranslateField(const char *field) const
{
  for (const translateField& f : fields)
    if (StringUtils::EqualsNoCase(field, f.string)) return f.field;
  return FieldNone;
}

std::string CSmartPlaylistRule::TranslateField(int field) const
{
  for (const translateField& f : fields)
    if (field == f.field) return f.string;
  return "none";
}

SortBy CSmartPlaylistRule::TranslateOrder(const char *order)
{
  return SortUtils::SortMethodFromString(order);
}

std::string CSmartPlaylistRule::TranslateOrder(SortBy order)
{
  std::string sortOrder = SortUtils::SortMethodToString(order);
  if (sortOrder.empty())
    return "none";

  return sortOrder;
}

Field CSmartPlaylistRule::TranslateGroup(const char *group)
{
  for (const auto & i : groups)
  {
    if (StringUtils::EqualsNoCase(group, i.name))
      return i.field;
  }

  return FieldUnknown;
}

std::string CSmartPlaylistRule::TranslateGroup(Field group)
{
  for (const auto & i : groups)
  {
    if (group == i.field)
      return i.name;
  }

  return "";
}

std::string CSmartPlaylistRule::GetLocalizedField(int field)
{
  for (const translateField& f : fields)
    if (field == f.field) return g_localizeStrings.Get(f.localizedString);
  return g_localizeStrings.Get(16018);
}

CDatabaseQueryRule::FIELD_TYPE CSmartPlaylistRule::GetFieldType(int field) const
{
  for (const translateField& f : fields)
    if (field == f.field) return f.type;
  return TEXT_FIELD;
}

bool CSmartPlaylistRule::IsFieldBrowseable(int field)
{
  for (const translateField& f : fields)
    if (field == f.field) return f.browseable;

  return false;
}

bool CSmartPlaylistRule::Validate(const std::string &input, void *data)
{
  if (data == NULL)
    return true;

  CSmartPlaylistRule *rule = static_cast<CSmartPlaylistRule*>(data);

  // check if there's a validator for this rule
  StringValidation::Validator validator = NULL;
  for (const translateField& field : fields)
  {
    if (rule->m_field == field.field)
    {
        validator = field.validator;
        break;
    }
  }
  if (validator == NULL)
    return true;

  // split the input into multiple values and validate every value separately
  std::vector<std::string> values = StringUtils::Split(input, RULE_VALUE_SEPARATOR);
  for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    if (!validator(*it, data))
      return false;
  }

  return true;
}

bool CSmartPlaylistRule::ValidateRating(const std::string &input, void *data)
{
  char *end = NULL;
  std::string strRating = input;
  StringUtils::Trim(strRating);

  double rating = std::strtod(strRating.c_str(), &end);
  return (end == NULL || *end == '\0') &&
         rating >= 0.0 && rating <= 10.0;
}

bool CSmartPlaylistRule::ValidateMyRating(const std::string &input, void *data)
{
  std::string strRating = input;
  StringUtils::Trim(strRating);

  int rating = atoi(strRating.c_str());
  return StringValidation::IsPositiveInteger(input, data) && rating <= 10;
}

std::vector<Field> CSmartPlaylistRule::GetFields(const std::string &type)
{
  std::vector<Field> fields;
  bool isVideo = false;
  if (type == "mixed")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldArtist);
    fields.push_back(FieldAlbumArtist);
    fields.push_back(FieldTitle);
    fields.push_back(FieldOriginalTitle);
    fields.push_back(FieldYear);
    fields.push_back(FieldTime);
    fields.push_back(FieldTrackNumber);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
  }
  else if (type == "songs")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldSource);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldDiscTitle);
    fields.push_back(FieldArtist);
    fields.push_back(FieldAlbumArtist);
    fields.push_back(FieldTitle);
    fields.push_back(FieldYear);
    fields.push_back(FieldTime);
    fields.push_back(FieldTrackNumber);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldComment);
    fields.push_back(FieldMoods);
  }
  else if (type == "albums")
  {
    fields.push_back(FieldGenre);
    fields.push_back(FieldSource);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldDiscTitle);
    fields.push_back(FieldTotalDiscs);
    fields.push_back(FieldIsBoxset);
    fields.push_back(FieldArtist);        // any artist
    fields.push_back(FieldAlbumArtist);  // album artist
    fields.push_back(FieldYear);
    fields.push_back(FieldReview);
    fields.push_back(FieldThemes);
    fields.push_back(FieldMoods);
    fields.push_back(FieldStyles);
    fields.push_back(FieldCompilation);
    fields.push_back(FieldAlbumType);
    fields.push_back(FieldMusicLabel);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldPath);
  }
  else if (type == "artists")
  {
    fields.push_back(FieldArtist);
    fields.push_back(FieldSource);
    fields.push_back(FieldGenre); // Mapped to Song table field
    fields.push_back(FieldMoods);
    fields.push_back(FieldStyles);
    fields.push_back(FieldInstruments);
    fields.push_back(FieldBiography);
    fields.push_back(FieldArtistType);
    fields.push_back(FieldGender);
    fields.push_back(FieldDisambiguation);
    fields.push_back(FieldBorn);
    fields.push_back(FieldBandFormed);
    fields.push_back(FieldDisbanded);
    fields.push_back(FieldDied);
    fields.push_back(FieldRole);
    fields.push_back(FieldPath);
    fields.push_back(FieldTime);
 // ! @Todo: song field made avialable to rule editor as demo. Need GUI for all song and album fields
  }
  else if (type == "tvshows")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldOriginalTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldTvShowStatus);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldYear);
    fields.push_back(FieldGenre);
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldNumberOfEpisodes);
    fields.push_back(FieldNumberOfWatchedEpisodes);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldPath);
    fields.push_back(FieldStudio);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldTag);
  }
  else if (type == "episodes")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldTvShowTitle);
    fields.push_back(FieldOriginalTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldTime);
    fields.push_back(FieldWriter);
    fields.push_back(FieldAirDate);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldGenre);
    fields.push_back(FieldYear); // premiered
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldEpisodeNumber);
    fields.push_back(FieldSeason);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldStudio);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldDateAdded);
    fields.push_back(FieldTag);
    isVideo = true;
  }
  else if (type == "movies")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldOriginalTitle);
    fields.push_back(FieldPlot);
    fields.push_back(FieldPlotOutline);
    fields.push_back(FieldTagline);
    fields.push_back(FieldVotes);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldTime);
    fields.push_back(FieldWriter);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldInProgress);
    fields.push_back(FieldGenre);
    fields.push_back(FieldCountry);
    fields.push_back(FieldYear); // premiered
    fields.push_back(FieldDirector);
    fields.push_back(FieldActor);
    fields.push_back(FieldMPAA);
    fields.push_back(FieldTop250);
    fields.push_back(FieldStudio);
    fields.push_back(FieldTrailer);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldSet);
    fields.push_back(FieldTag);
    fields.push_back(FieldDateAdded);
    isVideo = true;
  }
  else if (type == "musicvideos")
  {
    fields.push_back(FieldTitle);
    fields.push_back(FieldGenre);
    fields.push_back(FieldAlbum);
    fields.push_back(FieldYear);
    fields.push_back(FieldArtist);
    fields.push_back(FieldFilename);
    fields.push_back(FieldPath);
    fields.push_back(FieldPlaycount);
    fields.push_back(FieldLastPlayed);
    fields.push_back(FieldRating);
    fields.push_back(FieldUserRating);
    fields.push_back(FieldTime);
    fields.push_back(FieldDirector);
    fields.push_back(FieldStudio);
    fields.push_back(FieldPlot);
    fields.push_back(FieldTag);
    fields.push_back(FieldDateAdded);
    isVideo = true;
  }
  if (isVideo)
  {
    fields.push_back(FieldVideoResolution);
    fields.push_back(FieldAudioChannels);
    fields.push_back(FieldAudioCount);
    fields.push_back(FieldSubtitleCount);
    fields.push_back(FieldVideoCodec);
    fields.push_back(FieldAudioCodec);
    fields.push_back(FieldAudioLanguage);
    fields.push_back(FieldSubtitleLanguage);
    fields.push_back(FieldVideoAspectRatio);
  }
  fields.push_back(FieldPlaylist);
  fields.push_back(FieldVirtualFolder);

  return fields;
}

std::vector<SortBy> CSmartPlaylistRule::GetOrders(const std::string &type)
{
  std::vector<SortBy> orders;
  orders.push_back(SortByNone);
  if (type == "mixed")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByArtist);
    orders.push_back(SortByTitle);
    orders.push_back(SortByYear);
    orders.push_back(SortByTime);
    orders.push_back(SortByTrackNumber);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
  }
  else if (type == "songs")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByArtist);
    orders.push_back(SortByTitle);
    orders.push_back(SortByYear);
    orders.push_back(SortByTime);
    orders.push_back(SortByTrackNumber);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
  }
  else if (type == "albums")
  {
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByTotalDiscs);
    orders.push_back(SortByArtist);        // any artist
    orders.push_back(SortByYear);
    //orders.push_back(SortByThemes);
    //orders.push_back(SortByMoods);
    //orders.push_back(SortByStyles);
    orders.push_back(SortByAlbumType);
    //orders.push_back(SortByMusicLabel);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
  }
  else if (type == "artists")
  {
    orders.push_back(SortByArtist);
  }
  else if (type == "tvshows")
  {
    orders.push_back(SortBySortTitle);
    orders.push_back(SortByTvShowStatus);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByYear);
    orders.push_back(SortByGenre);
    orders.push_back(SortByNumberOfEpisodes);
    orders.push_back(SortByNumberOfWatchedEpisodes);
    //orders.push_back(SortByPlaycount);
    orders.push_back(SortByPath);
    orders.push_back(SortByStudio);
    orders.push_back(SortByMPAA);
    orders.push_back(SortByDateAdded);
    orders.push_back(SortByLastPlayed);
  }
  else if (type == "episodes")
  {
    orders.push_back(SortByTitle);
    orders.push_back(SortByTvShowTitle);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByTime);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByYear); // premiered/dateaired
    orders.push_back(SortByEpisodeNumber);
    orders.push_back(SortBySeason);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByStudio);
    orders.push_back(SortByMPAA);
    orders.push_back(SortByDateAdded);
  }
  else if (type == "movies")
  {
    orders.push_back(SortBySortTitle);
    orders.push_back(SortByVotes);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByTime);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByGenre);
    orders.push_back(SortByCountry);
    orders.push_back(SortByYear); // premiered
    orders.push_back(SortByMPAA);
    orders.push_back(SortByTop250);
    orders.push_back(SortByStudio);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByDateAdded);
  }
  else if (type == "musicvideos")
  {
    orders.push_back(SortByTitle);
    orders.push_back(SortByGenre);
    orders.push_back(SortByAlbum);
    orders.push_back(SortByYear);
    orders.push_back(SortByArtist);
    orders.push_back(SortByFile);
    orders.push_back(SortByPath);
    orders.push_back(SortByPlaycount);
    orders.push_back(SortByLastPlayed);
    orders.push_back(SortByTime);
    orders.push_back(SortByRating);
    orders.push_back(SortByUserRating);
    orders.push_back(SortByStudio);
    orders.push_back(SortByDateAdded);
  }
  orders.push_back(SortByRandom);

  return orders;
}

std::vector<Field> CSmartPlaylistRule::GetGroups(const std::string &type)
{
  std::vector<Field> groups;
  groups.push_back(FieldUnknown);

  if (type == "artists")
    groups.push_back(FieldGenre);
  else if (type == "albums")
    groups.push_back(FieldYear);
  if (type == "movies")
  {
    groups.push_back(FieldNone);
    groups.push_back(FieldSet);
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldActor);
    groups.push_back(FieldDirector);
    groups.push_back(FieldWriter);
    groups.push_back(FieldStudio);
    groups.push_back(FieldCountry);
    groups.push_back(FieldTag);
  }
  else if (type == "tvshows")
  {
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldActor);
    groups.push_back(FieldDirector);
    groups.push_back(FieldStudio);
    groups.push_back(FieldTag);
  }
  else if (type == "musicvideos")
  {
    groups.push_back(FieldArtist);
    groups.push_back(FieldAlbum);
    groups.push_back(FieldGenre);
    groups.push_back(FieldYear);
    groups.push_back(FieldDirector);
    groups.push_back(FieldStudio);
    groups.push_back(FieldTag);
  }

  return groups;
}

std::string CSmartPlaylistRule::GetLocalizedGroup(Field group)
{
  for (const auto & i : groups)
  {
    if (group == i.field)
      return g_localizeStrings.Get(i.localizedString);
  }

  return g_localizeStrings.Get(groups[0].localizedString);
}

bool CSmartPlaylistRule::CanGroupMix(Field group)
{
  for (const auto & i : groups)
  {
    if (group == i.field)
      return i.canMix;
  }

  return false;
}

std::string CSmartPlaylistRule::GetLocalizedRule() const
{
  return StringUtils::Format("%s %s %s", GetLocalizedField(m_field).c_str(), GetLocalizedOperator(m_operator).c_str(), GetParameter().c_str());
}

std::string CSmartPlaylistRule::GetVideoResolutionQuery(const std::string &parameter) const
{
  std::string retVal(" IN (SELECT DISTINCT idFile FROM streamdetails WHERE iVideoWidth ");
  int iRes = (int)std::strtol(parameter.c_str(), NULL, 10);

  int min, max;
  if (iRes >= 2160)
  {
    min = 1921;
    max = INT_MAX;
  }
  else if (iRes >= 1080) { min = 1281; max = 1920; }
  else if (iRes >= 720) { min =  961; max = 1280; }
  else if (iRes >= 540) { min =  721; max =  960; }
  else                  { min =    0; max =  720; }

  switch (m_operator)
  {
    case OPERATOR_EQUALS:
      retVal += StringUtils::Format(">= %i AND iVideoWidth <= %i", min, max);
      break;
    case OPERATOR_DOES_NOT_EQUAL:
      retVal += StringUtils::Format("< %i OR iVideoWidth > %i", min, max);
      break;
    case OPERATOR_LESS_THAN:
      retVal += StringUtils::Format("< %i", min);
      break;
    case OPERATOR_GREATER_THAN:
      retVal += StringUtils::Format("> %i", max);
      break;
    default:
      break;
  }

  retVal += ")";
  return retVal;
}

std::string CSmartPlaylistRule::GetBooleanQuery(const std::string &negate, const std::string &strType) const
{
  if (strType == "movies")
  {
    if (m_field == FieldInProgress)
      return "movie_view.idFile " + negate + " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
    else if (m_field == FieldTrailer)
      return negate + GetField(m_field, strType) + "!= ''";
  }
  else if (strType == "episodes")
  {
    if (m_field == FieldInProgress)
      return "episode_view.idFile " + negate + " IN (SELECT DISTINCT idFile FROM bookmark WHERE type = 1)";
  }
  else if (strType == "tvshows")
  {
    if (m_field == FieldInProgress)
      return negate + " ("
                          "(tvshow_view.watchedcount > 0 AND tvshow_view.watchedcount < tvshow_view.totalCount) OR "
                          "(tvshow_view.watchedcount = 0 AND EXISTS "
                            "(SELECT 1 FROM episode_view WHERE episode_view.idShow = " + GetField(FieldId, strType) + " AND episode_view.resumeTimeInSeconds > 0)"
                          ")"
                       ")";
  }
  if (strType == "albums")
  {
    if (m_field == FieldCompilation)
      return negate + GetField(m_field, strType);
    if (m_field == FieldIsBoxset)
      return negate + "albumview.bBoxedSet = 1";
  }
  return "";
}

CDatabaseQueryRule::SEARCH_OPERATOR CSmartPlaylistRule::GetOperator(const std::string &strType) const
{
  SEARCH_OPERATOR op = CDatabaseQueryRule::GetOperator(strType);
  if ((strType == "tvshows" || strType == "episodes") && m_field == FieldYear)
  { // special case for premiered which is a date rather than a year
    //! @todo SMARTPLAYLISTS do we really need this, or should we just make this field the premiered date and request a date?
    if (op == OPERATOR_EQUALS)
      op = OPERATOR_CONTAINS;
    else if (op == OPERATOR_DOES_NOT_EQUAL)
      op = OPERATOR_DOES_NOT_CONTAIN;
  }
  return op;
}

std::string CSmartPlaylistRule::FormatParameters(const std::string& negate,
                                                 const std::string& oper,
                                                 const CDatabase& db,
                                                 const std::string& strType) const
{
  std::string wholeQuery;
  std::string parameter;
  std::string clause;

  if (strType == "songs")
  {
    if (m_field == FieldGenre)
    {
      CDatabase::ExistsSubQuery existsQuery("song_genre", "song_genre.idSong = songview.idSong");
      existsQuery.AppendJoin("JOIN genre ON genre.idGenre = song_genre.idGenre");
      for (auto it : m_parameter)
      {
        clause =
            FormatWhereClause("", oper, it, db, MediaTypeMusic); // e.g. genre.strGenre LIKE "XXX"
        if (negate.empty())
        {
          // Gather genre parameters together into one clause
          if (!wholeQuery.empty())
            wholeQuery += " OR ";
          wholeQuery += clause;
        }
        else
        {
          // Each parameter has separate clause
          existsQuery.AppendWhere(clause);
          existsQuery.BuildSQL(clause);
          existsQuery.where = ""; // Clear where for next parameter
          clause = negate + clause;
          if (!wholeQuery.empty())
            wholeQuery += " AND ";
          wholeQuery += clause;
        }
      }
      if (negate.empty() && !wholeQuery.empty())
      {
        // Build subquery for combined genre value clause
        if (m_parameter.size() > 1)
          wholeQuery = '(' + wholeQuery + ')';
        existsQuery.AppendWhere(wholeQuery);
        existsQuery.BuildSQL(wholeQuery);
      }
    }
  }
  else if (strType == "albums")
  {
    if (m_field == FieldSource)
    {
      CDatabase::ExistsSubQuery existsQuery("album_source",
                                            "album_source.idAlbum = albumview.idAlbum");
      existsQuery.AppendJoin("JOIN source ON album_source.idSource = source.idSource");
      for (auto it : m_parameter)
      {
        clause =
            FormatWhereClause("", oper, it, db, MediaTypeMusic); // e.g. source.strName LIKE "XXX"
        if (negate.empty())
        {
          // Gather source parameters together into one clause
          if (!wholeQuery.empty())
            wholeQuery += " OR ";
          wholeQuery += clause;
        }
        else
        {
          // Each parameter has separate clause
          existsQuery.AppendWhere(clause);
          existsQuery.BuildSQL(clause);
          existsQuery.where = ""; // Clear where for next parameter
          clause = negate + clause;
          if (!wholeQuery.empty())
            wholeQuery += " AND ";
          wholeQuery += clause;
        }
      }
      if (negate.empty() && !wholeQuery.empty())
      {
        // Build subquery for combined genre value clause
        if (m_parameter.size() > 1)
          wholeQuery = '(' + wholeQuery + ')';
        existsQuery.AppendWhere(wholeQuery);
        existsQuery.BuildSQL(wholeQuery);
      }
    }
  }

  if (wholeQuery.empty())
    wholeQuery = CDatabaseQueryRule::FormatParameters(negate, oper, db, strType);
  return wholeQuery;
}

std::string CSmartPlaylistRule::FormatParameter(const std::string &operatorString, const std::string &param, const CDatabase &db, const std::string &strType) const
{
  // special-casing
  if (m_field == FieldTime)
  { // translate time to seconds
    std::string seconds = StringUtils::Format("%li", StringUtils::TimeStringToSeconds(param));
    return db.PrepareSQL(operatorString.c_str(), seconds.c_str());
  }
  return CDatabaseQueryRule::FormatParameter(operatorString, param, db, strType);
}

std::string CSmartPlaylistRule::FormatLinkQuery(const char *field, const char *table, const MediaType& mediaType, const std::string& mediaField, const std::string& parameter)
{
  // NOTE: no need for a PrepareSQL here, as the parameter has already been formatted
  return StringUtils::Format(" EXISTS (SELECT 1 FROM %s_link"
                             "         JOIN %s ON %s.%s_id=%s_link.%s_id"
                             "         WHERE %s_link.media_id=%s AND %s.name %s AND %s_link.media_type = '%s')",
                             field, table, table, table, field, table, field, mediaField.c_str(), table, parameter.c_str(), field, mediaType.c_str());
}

std::string CSmartPlaylistRule::FormatWhereClause(const std::string &negate, const std::string &oper, const std::string &param,
                                                 const CDatabase &db, const std::string &strType) const
{
  std::string parameter = FormatParameter(oper, param, db, strType);

  std::string query;
  std::string table;
  if (strType == "songs")
  {
    table = "songview";
    if (m_field == FieldLastPlayed &&
        (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE ||
         m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
    /* ! @todo: tidy up
    if (m_field == FieldGenre)
      query = negate + " EXISTS (SELECT 1 FROM song_genre, genre WHERE song_genre.idSong = " + GetField(FieldId, strType) + " AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
    else 
    if (m_field == FieldArtist)
      query = negate + " EXISTS (SELECT 1 FROM song_artist, artist WHERE song_artist.idSong = " + GetField(FieldId, strType) + " AND song_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldAlbumArtist)
      query = negate + " EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = " + table + ".idAlbum AND album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldLastPlayed && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
    else if (m_field == FieldSource)
      query = negate + " EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = " + table + ".idAlbum AND album_source.idSource = source.idSource AND source.strName" + parameter + ")"; */
  }
  else if (strType == "albums")
  {
    table = "albumview";
    /* ! @todo: tidy up
    if (m_field == FieldGenre)
      query = negate + " EXISTS (SELECT 1 FROM song, song_genre, genre WHERE song.idAlbum = " + GetField(FieldId, strType) + " AND song.idSong = song_genre.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
    else if (m_field == FieldArtist)
      query = negate + " EXISTS (SELECT 1 FROM song, song_artist, artist WHERE song.idAlbum = " + GetField(FieldId, strType) + " AND song.idSong = song_artist.idSong AND song_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldAlbumArtist)
      query = negate + " EXISTS (SELECT 1 FROM album_artist, artist WHERE album_artist.idAlbum = " + GetField(FieldId, strType) + " AND album_artist.idArtist = artist.idArtist AND artist.strArtist" + parameter + ")";
    else if (m_field == FieldPath)
      query = negate + " EXISTS (SELECT 1 FROM song JOIN path on song.idpath = path.idpath WHERE song.idAlbum = " + GetField(FieldId, strType) + " AND path.strPath" + parameter + ")";
    else */
    if (m_field == FieldLastPlayed &&
        (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE ||
         m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " is NULL or " + GetField(m_field, strType) + parameter;
    /*
    else if (m_field == FieldSource)
      query = negate + " EXISTS (SELECT 1 FROM album_source, source WHERE album_source.idAlbum = " + GetField(FieldId, strType) + " AND album_source.idSource = source.idSource AND source.strName" + parameter + ")";
      */
    else if (m_field == FieldDiscTitle)
      query = negate +
              " EXISTS (SELECT 1 FROM song WHERE song.idAlbum = " + GetField(FieldId, strType) +
              " AND song.strDiscSubtitle" + parameter + ")";
  }
  else if (strType == "artists")
  {
    table = "artistview";
    /*
    if (m_field == FieldGenre)
      query = negate + " EXISTS (SELECT 1 FROM song_genre JOIN genre ON song_genre.idSong = song.idSong AND song_genre.idGenre = genre.idGenre AND genre.strGenre" + parameter + ")";
    */
  }
  else if (strType == "movies")
  {
    table = "movie_view";

    if (m_field == FieldGenre)
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldDirector)
      query = negate + FormatLinkQuery("director", "actor", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldActor)
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldWriter)
      query = negate + FormatLinkQuery("writer", "actor", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldStudio)
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if (m_field == FieldCountry)
      query = negate + FormatLinkQuery("country", "country", MediaTypeMovie, GetField(FieldId, strType), parameter);
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
    else if (m_field == FieldTag)
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeMovie, GetField(FieldId, strType), parameter);
  }
  else if (strType == "musicvideos")
  {
    table = "musicvideo_view";

    if (m_field == FieldGenre)
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
    else if (m_field == FieldArtist || m_field == FieldAlbumArtist)
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
    else if (m_field == FieldStudio)
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
    else if (m_field == FieldDirector)
      query = negate + FormatLinkQuery("director", "actor", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
    else if (m_field == FieldTag)
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeMusicVideo, GetField(FieldId, strType), parameter);
  }
  else if (strType == "tvshows")
  {
    table = "tvshow_view";

    if (m_field == FieldGenre)
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeTvShow, GetField(FieldId, strType), parameter);
    else if (m_field == FieldDirector)
      query = negate + FormatLinkQuery("director", "actor", MediaTypeTvShow, GetField(FieldId, strType), parameter);
    else if (m_field == FieldActor)
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeTvShow, GetField(FieldId, strType), parameter);
    else if (m_field == FieldStudio)
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeTvShow, GetField(FieldId, strType), parameter);
    else if (m_field == FieldMPAA)
      query = negate + " (" + GetField(m_field, strType) + parameter + ")";
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
    else if (m_field == FieldPlaycount)
      query = "CASE WHEN COALESCE(" + GetField(FieldNumberOfEpisodes, strType) + " - " + GetField(FieldNumberOfWatchedEpisodes, strType) + ", 0) > 0 THEN 0 ELSE 1 END " + parameter;
    else if (m_field == FieldTag)
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeTvShow, GetField(FieldId, strType), parameter);
  }
  else if (strType == "episodes")
  {
    table = "episode_view";

    if (m_field == FieldGenre)
      query = negate + FormatLinkQuery("genre", "genre", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == FieldTag)
      query = negate + FormatLinkQuery("tag", "tag", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == FieldDirector)
      query = negate + FormatLinkQuery("director", "actor", MediaTypeEpisode, GetField(FieldId, strType), parameter);
    else if (m_field == FieldActor)
      query = negate + FormatLinkQuery("actor", "actor", MediaTypeEpisode, GetField(FieldId, strType), parameter);
    else if (m_field == FieldWriter)
      query = negate + FormatLinkQuery("writer", "actor", MediaTypeEpisode, GetField(FieldId, strType), parameter);
    else if ((m_field == FieldLastPlayed || m_field == FieldDateAdded) && (m_operator == OPERATOR_LESS_THAN || m_operator == OPERATOR_BEFORE || m_operator == OPERATOR_NOT_IN_THE_LAST))
      query = GetField(m_field, strType) + " IS NULL OR " + GetField(m_field, strType) + parameter;
    else if (m_field == FieldStudio)
      query = negate + FormatLinkQuery("studio", "studio", MediaTypeTvShow, (table + ".idShow").c_str(), parameter);
    else if (m_field == FieldMPAA)
      query = negate + " (" + GetField(m_field, strType) +  parameter + ")";
  }
  if (m_field == FieldVideoResolution)
    query = table + ".idFile" + negate + GetVideoResolutionQuery(param);
  else if (m_field == FieldAudioChannels)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND iAudioChannels " + parameter + ")";
  else if (m_field == FieldVideoCodec)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strVideoCodec " + parameter + ")";
  else if (m_field == FieldAudioCodec)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioCodec " + parameter + ")";
  else if (m_field == FieldAudioLanguage)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strAudioLanguage " + parameter + ")";
  else if (m_field == FieldSubtitleLanguage)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND strSubtitleLanguage " + parameter + ")";
  else if (m_field == FieldVideoAspectRatio)
    query = negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND fVideoAspect " + parameter + ")";
  else if (m_field == FieldAudioCount)
    query = db.PrepareSQL(negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND streamdetails.iStreamtype = %i GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType) " + parameter + ")",CStreamDetail::AUDIO);
  else if (m_field == FieldSubtitleCount)
    query = db.PrepareSQL(negate + " EXISTS (SELECT 1 FROM streamdetails WHERE streamdetails.idFile = " + table + ".idFile AND streamdetails.iStreamType = %i GROUP BY streamdetails.idFile HAVING COUNT(streamdetails.iStreamType) " + parameter + ")",CStreamDetail::SUBTITLE);
  if (m_field == FieldPlaycount && strType != "songs" && strType != "albums" && strType != "tvshows")
  { // playcount IS stored as NULL OR number IN video db
    if ((m_operator == OPERATOR_EQUALS && param == "0") ||
        (m_operator == OPERATOR_DOES_NOT_EQUAL && param != "0") ||
        (m_operator == OPERATOR_LESS_THAN))
    {
      std::string field = GetField(FieldPlaycount, strType);
      query = field + " IS NULL OR " + field + parameter;
    }
  }
  if (query.empty())
    query = CDatabaseQueryRule::FormatWhereClause(negate, oper, param, db, strType);
  return query;
}

std::string CSmartPlaylistRule::GetField(int field, const std::string &type) const
{
  if (field >= FieldUnknown && field < FieldMax)
    return DatabaseUtils::GetField((Field)field, CMediaTypes::FromString(type), DatabaseQueryPartWhere);
  return "";
}

std::string CSmartPlaylistRuleCombination::GetWhereClause(const CDatabase &db, const std::string& strType, std::set<std::string> &referencedPlaylists) const
{
  std::string rule;

  // translate the combinations into SQL
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    if (it != m_combinations.begin())
      rule += m_type == CombinationAnd ? " AND " : " OR ";
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
      rule += "(" + combo->GetWhereClause(db, strType, referencedPlaylists) + ")";
  }

  // translate the rules into SQL
  CSmartPlaylistRuleCombination joinedRules;
  joinedRules.SetType(m_type);
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // don't include playlists that are meant to be displayed
    // as a virtual folders in the SQL WHERE clause
    if ((*it)->m_field == FieldVirtualFolder)
      continue;

    std::string currentRule;
    if ((*it)->m_field == FieldPlaylist)
    {
      std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
      if (!playlistFile.empty() && referencedPlaylists.find(playlistFile) == referencedPlaylists.end())
      {
        referencedPlaylists.insert(playlistFile);
        CSmartPlaylist playlist;
        if (playlist.Load(playlistFile))
        {
          std::string playlistQuery;
          // only playlists of same type will be part of the query
          if (playlist.GetType() == strType || (playlist.GetType() == "mixed" && (strType == "songs" || strType == "musicvideos")) || playlist.GetType().empty())
          {
            playlist.SetType(strType);
            playlistQuery = playlist.GetWhereClause(db, referencedPlaylists);
          }
          if (playlist.GetType() == strType)
          {
            if ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL)
              currentRule = StringUtils::Format("NOT (%s)", playlistQuery.c_str());
            else
              currentRule = playlistQuery;
          }
        }
      }
    }
    // Gather rules that need a combined clause into a separate rule combination
    // ! @todo: Have only added one field (song duration) to "artists" avialable via playlist editor as a demo, add more or make alternative GUI
    else if (!IsFieldRuleSimple((Field)(*it)->m_field, CMediaTypes::FromString(strType)))
    {
      joinedRules.m_rules.push_back(*it);
    }
    else
      currentRule = (*it)->GetWhereClause(db, strType);

    // Append rule to where clause
    rule = CombineClause(rule, currentRule);
  }

  // Now generate the where clause from the combined rules
  if (!joinedRules.empty())
  {
    std::string whereclause;
    whereclause = joinedRules.GetCombinedWhereClause(db, strType);
    // Append combined rules to where clause
    rule = CombineClause(rule, whereclause);
  }

  return rule;
}

std::string CSmartPlaylistRuleCombination::GetCombinedWhereClause(const CDatabase& db,
                                                                  const std::string& strType) const
{
  if (strType == "artists")
    return GetArtistsWhereClause(db); // Rules of "artists" playlist that need combined where clause
  else if (strType == "albums")
    return GetAlbumsWhereClause(db); // Rules of "albums" playlist that need combined where clause
  else if (strType == "songs")
    return GetSongsWhereClause(db); // Rules of "songs" playlist that need combined where clause
  else
    return std::string();
}

std::string CSmartPlaylistRuleCombination::GetRolesWhereClause(const CDatabase& db,
                                                               bool& bAlbumArtists,
                                                               bool& bSongArtists,
                                                               bool& bJoinRole,
                                                               bool& bRoleRules) const
{
  bool bWildRoleRule = false;
  bool bAlbumartistRule = false;
  bool bArtistRule = false;
  bool bNegAlbumartistRule = false;
  bool bNegArtistRule = false;

  // Examine what fields we have
  CSmartPlaylistRuleCombination roleRules;
  roleRules.SetType(m_type);
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    // Gather role rules, they effect how all other rules are implemented
    // song and album rules in artists lists, song and artists rules in albums etc.
    if ((*it)->m_field == FieldRole)
    {
      bRoleRules = true;
      if (StringUtils::EqualsNoCase((*it)->m_parameter.at(0), "albumartist"))
      {
        // Use fake role "album_artist" when AND combo to skip song artists (guest appearences)
        bAlbumartistRule = (m_type == CombinationAnd);
        bNegAlbumartistRule =
            ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_CONTAIN) ||
            ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL);
      }
      else if (StringUtils::EqualsNoCase((*it)->m_parameter.at(0), "artist"))
      {
        // Use "artist" role to have only guest appearences??
        bArtistRule = (m_type == CombinationAnd);
        bNegArtistRule = ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_CONTAIN) ||
                         ((*it)->m_operator == CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL);
      }
      else if (StringUtils::EqualsNoCase((*it)->m_parameter.at(0), "%"))
        bWildRoleRule = true;
      else
        roleRules.m_rules.push_back(*it);
    }
  }

  // Validate role rule logic
  // Default (no role rules) check both album_artist and song_artist tables for artist rules
  bAlbumArtists = true;
  bSongArtists = true;
  bJoinRole = true;
  if (bWildRoleRule)
    roleRules.clear(); // All roles wanted, overrides other role rules
  else
  {
    // "Albumartist" or NOT "Artist" rule => album artists only, no song artists
    // NOT "Albumartist" or "Artist" rule => song artists route only
    if ((bNegAlbumartistRule && bAlbumartistRule) || (!bNegArtistRule && bArtistRule))
      bAlbumArtists = false;
    if ((bNegArtistRule && bArtistRule) || (!bNegAlbumartistRule && bAlbumartistRule))
      bSongArtists = false;

    // But at least one of album artists and song artists must be true
    if (!bAlbumArtists && !bSongArtists)
      bAlbumArtists = true;
  }

  // Translate the rules into SQL

  if (!bWildRoleRule && roleRules.empty())
  {
    bJoinRole = false;
    bRoleRules = false;
    return "song_artist.idRole = 1";
  }
  else
  {
    std::string roleClause;
    std::string currentRule;
    for (CDatabaseQueryRules::const_iterator it = roleRules.m_rules.begin();
         it != roleRules.m_rules.end(); ++it)
    {
      std::string rolequery = (*it)->GetWhereClause(db, MediaTypeMusic);
      if (!rolequery.empty())
      {
        if (m_type == CombinationAnd)
        {
          if (roleClause.empty())
            roleClause = rolequery;
          else
          {
            CDatabase::ExistsSubQuery songRoleX(
                "song_artist AS sa1",
                "sa1.idArtist = song_artist.idArtist AND sa1.idSong = song_artist.idSong");
            songRoleX.AppendJoin("JOIN role as r1 ON sa1.idRole = r1.idRole");
            StringUtils::Replace(rolequery, "role.", "r1.");
            songRoleX.AppendWhere(rolequery);
            songRoleX.BuildSQL(currentRule);
            roleClause = CombineClause(roleClause, currentRule);
          }
        }
        else
        {
          if (!roleClause.empty())
            roleClause += " OR ";
          roleClause += rolequery;
        }
      }
    }
    if (!roleClause.empty() && bSongArtists)
      bAlbumArtists = false;
    return roleClause;
  }

  return std::string();
}

std::string CSmartPlaylistRuleCombination::GetArtistsWhereClause(const CDatabase& db) const
{
  std::string rule;

  // Examine what role fields we have
  bool bAlbumArtists = true;
  bool bSongArtists = true;
  bool bJoinRole = false;
  bool bRoleRules = false;
  std::string roleClause;
  roleClause = GetRolesWhereClause(db, bAlbumArtists, bSongArtists, bJoinRole, bRoleRules);

  // Examine "albums" and "songs" rules, building basic SQL clauses
  bool bAlbumArtistNeedSong = false; // Need song table as well as album_artist (which has idAlbum)
  bool bSongArtistNeedSong = false; // Need song table as well as song_artist (which has idSong)
  std::string songSubclause;
  std::string albumSubclause;
  std::string albumSourceclause;
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    std::string currentRule;
    if ((*it)->m_field == FieldPath)
    {
      std::string pathquery = (*it)->GetWhereClause(db, MediaTypeMusic);
      if (!pathquery.empty())
      {
        // Process FieldPath rules separately from song rules because using song not songview
        CDatabase::ExistsSubQuery songPathX("path", "path.idPath = song.idPath");
        songPathX.AppendWhere(pathquery);
        songPathX.BuildSQL(currentRule);
        songSubclause = CombineClause(songSubclause, currentRule);
        bSongArtistNeedSong = true; // To get idPath
      }
    }
    else if ((*it)->m_field == FieldSource)
    {
      std::string query = (*it)->GetWhereClause(db, "albums");
      StringUtils::Replace(query, "albumview", "album");
      albumSourceclause = CombineClause(albumSourceclause, query);
      bSongArtistNeedSong = true; // To get idAlbum
    }
    else if (IsFieldNative((Field)(*it)->m_field, MediaTypeArtist, "song"))
    { //song field
      std::string songquery = (*it)->GetWhereClause(db, "songs");
      StringUtils::Replace(songquery, "songview", "song");
      songSubclause = CombineClause(songSubclause, songquery);
      if ((*it)->m_field != FieldGenre) // Lookup genre on idSong
        bSongArtistNeedSong = true;
    }
    else if (IsFieldNative((Field)(*it)->m_field, MediaTypeArtist, "album"))
    { //album field
      std::string albumquery = (*it)->GetWhereClause(db, "albums");
      StringUtils::Replace(albumquery, "albumview", "album");
      albumSubclause = CombineClause(albumSubclause, albumquery);
      bSongArtistNeedSong = true; // To get idAlbum
    }
  }

  // Have song clause (inc genre) or role rules that needs to be applied for album artist
  bAlbumArtistNeedSong = (!songSubclause.empty() || (bJoinRole && !bSongArtists));

  // Translate the rules into SQL subqueries
  std::string strSQL;
  std::string album_artistClause;
  std::string song_artistClause;

  // Create song_artist subquery for role roles
  CDatabase::ExistsSubQuery songArtistSub("song_artist",
                                          "song_artist.idArtist = artistview.idArtist");
  if (!roleClause.empty())
  {
    if (bJoinRole)
    {
      roleClause = "(" + roleClause + ")";
      songArtistSub.AppendJoin("JOIN role ON song_artist.idRole = role.idRole");
    }
    songArtistSub.AppendWhere(roleClause);
  }

  // album_artist route
  if (bAlbumArtistNeedSong)
  {
    if (bRoleRules && bAlbumArtists && !bSongArtists)
    {
      // Album artists only with other role rules.
      // Need song_artist table in song subquery to apply the role rules
      /* songArtistSub has just role rules so far e.g.
      EXISTS(SELECT 1 FROM song_artist JOIN role ON song_artist.idRole = role.idRole
      WHERE song_artist.idArtist = artistview.idArtist
      AND role.strRole LIKE 'Composer')
      Need to add correlation to song table:
      song_artist.idSong = song.idSong
      */
      songArtistSub.AppendWhere("song_artist.idSong = song.idSong");
      songArtistSub.BuildSQL(strSQL);
      if (!strSQL.empty())
        songSubclause = CombineClause(songSubclause, strSQL);
    }
    CDatabase::ExistsSubQuery songAASub("song", "song.idAlbum = album_artist.idAlbum");
    songAASub.AppendWhere("(" + songSubclause + ")");
    songAASub.BuildSQL(album_artistClause);
  }

  if (!albumSubclause.empty()) // Album rules 4, 6
  {
    CDatabase::ExistsSubQuery albumSub("album", "album.idAlbum = album_artist.idAlbum");
    // source clause within album subquery
    strSQL = CombineClause(albumSubclause, albumSourceclause);
    // Song subquery for song and role rules within album subquery
    if (!album_artistClause.empty())
      strSQL = CombineClause(strSQL, album_artistClause);
    albumSub.AppendWhere("(" + strSQL + ")");
    albumSub.BuildSQL(album_artistClause);
  }
  else if (!albumSourceclause.empty()) // Source but no album rules 3, 5, 7
  {
    // Correlate with source directly, album_artist has idAlbum
    strSQL = albumSourceclause;
    StringUtils::Replace(strSQL, "album.idAlbum", "album_artist.idAlbum");
    // Combine song subquery for song and role rules
    album_artistClause = CombineClause(strSQL, album_artistClause);
  }

  CDatabase::ExistsSubQuery albumArtistSub("album_artist",
                                           "album_artist.idArtist = artistview.idArtist");
  if (!album_artistClause.empty())
    albumArtistSub.AppendWhere("(" + album_artistClause + ")");

  // song_artist route
  if (!albumSubclause.empty()) // Album rules 4, 6
  {
    CDatabase::ExistsSubQuery albumSub("album", "album.idAlbum = song.idAlbum");
    // source clause within album subquery
    strSQL = CombineClause(albumSubclause, albumSourceclause);
    albumSub.AppendWhere("(" + strSQL + ")");
    albumSub.BuildSQL(song_artistClause);
  }
  else if (!albumSourceclause.empty()) // Source but no album rules 3, 5, 7
  {
    // Correlate with source directly, album_artist has idAlbum
    strSQL = albumSourceclause;
    StringUtils::Replace(strSQL, "album.idAlbum", "song.idAlbum");
    // Combine song subquery for song and role rules
    song_artistClause = CombineClause(strSQL, song_artistClause);
  }

  if (bSongArtistNeedSong) //All but genre rule 2 - 7
  {
    strSQL = song_artistClause;
    if (!songSubclause.empty())
      strSQL = CombineClause(songSubclause, song_artistClause); //2, 5, 6, 7
    CDatabase::ExistsSubQuery songSASub("song", "song.idSong = song_artist.idSong");
    songSASub.AppendWhere("(" + strSQL + ")");
    songSASub.BuildSQL(song_artistClause);
  }
  else if (!songSubclause.empty()) // genre rules only 1
  {
    // song clause based on idSong so directly apply to song_artist table
    StringUtils::Replace(songSubclause, "song.idSong", "song_artist.idSong");
    song_artistClause = songSubclause;
  }
  if (!song_artistClause.empty())
    songArtistSub.AppendWhere("(" + song_artistClause + ")");

  //Combine album_artist and song_artist clauses
  if (bAlbumArtists)
  {
    albumArtistSub.BuildSQL(strSQL);
    if (!strSQL.empty())
      rule += strSQL;
  }

  if (bSongArtists)
  {
    songArtistSub.BuildSQL(strSQL);
    if (!strSQL.empty())
    {
      if (!rule.empty())
      {
        if (!bJoinRole || m_type != CombinationAnd)
          rule += " OR ";
        else
          rule += " AND ";
      }
    }
    rule += strSQL;
  }
  return rule;
}

std::string CSmartPlaylistRuleCombination::GetAlbumsWhereClause(const CDatabase& db) const
{
  std::string rule;
  // Processing for FieldArtist, FieldAlbumArtist and FieldPath fields, and other "artists" or "songs" rule fields
  // FieldGenre is handled with the other "songs"  rule fields

  // Examine what role fields and build roles clause
  bool bAlbumArtists = true;
  bool bSongArtists = true;
  bool bJoinRole = false;
  bool bRoleRules = false;
  std::string roleClause =
      GetRolesWhereClause(db, bAlbumArtists, bSongArtists, bJoinRole, bRoleRules);

  // Examine "artists" and "songs" rules, building basic SQL clauses
  std::string songSubclause;
  std::string artistSubclause;
  std::string albumartistField;
  std::string artistField;
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    std::string currentRule;
    if ((*it)->m_field == FieldPath)
    {
      // Handle FieldPath here as using song table not songview like "songs"
      std::string pathquery = (*it)->GetWhereClause(db, MediaTypeMusic);
      if (!pathquery.empty())
      {
        CDatabase::ExistsSubQuery songPathX("path", "path.idPath = song.idPath");
        songPathX.AppendWhere(pathquery);
        songPathX.BuildSQL(currentRule);
        songSubclause = CombineClause(songSubclause, currentRule);
      }
    }
    else if (IsFieldNative((Field)(*it)->m_field, MediaTypeAlbum, "artist"))
    { //"artists" fields including FieldArtist and FieldAlbumArtist
      std::string artistquery = (*it)->GetWhereClause(db, "artist");
      StringUtils::Replace(artistquery, "artistview", "artist");
      if ((*it)->m_field == FieldArtist)
        artistField = CombineClause(artistField, artistquery);
      else if ((*it)->m_field == FieldAlbumArtist)
        albumartistField = CombineClause(albumartistField, artistquery);
      else
        artistSubclause = CombineClause(artistSubclause, artistquery);
    }
    else if (IsFieldNative((Field)(*it)->m_field, MediaTypeAlbum, "song"))
    { //"songs" field including FieldGenre
      std::string songquery = (*it)->GetWhereClause(db, "songs");
      StringUtils::Replace(songquery, "songview", "song");
      songSubclause = CombineClause(songSubclause, songquery);
    }
  }

  // Album artists only with other role rules flag
  bool bAlbumartistAndRole =
      bRoleRules && (!albumartistField.empty() || (bAlbumArtists && !bSongArtists));

  // Translate the rules into SQL subqueries
  CDatabase::ExistsSubQuery songSub("song", "song.idAlbum = albumview.idAlbum");
  CDatabase::ExistsSubQuery albumArtistSub("album_artist",
                                           "album_artist.idAlbum = albumview.idAlbum");
  albumArtistSub.AppendJoin("JOIN artist ON artist.idArtist = album_artist.idArtist");

  CDatabase::ExistsSubQuery songArtistSub("song_artist", "song_artist.idSong = song.idSong");
  if (bAlbumartistAndRole)
    // Album artists only with other role rules, add correlation to album_artist table
    songArtistSub.AppendWhere("song_artist.idArtist = album_artist.idArtist");
  else if (!artistSubclause.empty() || !albumartistField.empty() || !artistField.empty())
    // JOIN artist when have artist clause (may be just role rule)
    songArtistSub.AppendJoin("JOIN artist ON artist.idArtist = song_artist.idArtist");
  if (!roleClause.empty())
  {
    if (bJoinRole)
    {
      roleClause = "(" + roleClause + ")";
      songArtistSub.AppendJoin("JOIN role ON song_artist.idRole = role.idRole");
    }
    songArtistSub.AppendWhere(roleClause);
  }

  // Build combined artist clause for inclusion in album and song artist subqueries
  std::string artistClauseAlbum;
  std::string artistClauseSong;
  if (!albumartistField.empty() && !artistField.empty())
  { // Separate artist clauses for song and album artist routes
    artistClauseAlbum = CombineClause(artistSubclause, albumartistField);
    artistClauseSong = CombineClause(artistSubclause, artistField);
  }
  else
  {
    artistSubclause = CombineClause(artistSubclause, albumartistField);
    artistSubclause = CombineClause(artistSubclause, artistField);
    artistClauseAlbum = artistSubclause;
    artistClauseSong = artistSubclause;
  }

  // Build song_artist subquery clause
  std::string songartistSubclause;
  if (!artistClauseSong.empty() || bRoleRules)
  {
    if (!artistClauseSong.empty() && !bAlbumartistAndRole)
      songArtistSub.AppendWhere("(" + artistClauseSong + ")");
    songArtistSub.BuildSQL(songartistSubclause);
  }

  //Build full song subquery SQL (with and without song_artist)
  std::string songSubSQLNoArtist;
  std::string songSubSQL;
  if (!songSubclause.empty())
  {
    songSub.AppendWhere("(" + songSubclause + ")");
    songSub.BuildSQL(songSubSQLNoArtist);
  }
  songSubSQL = CombineClause(songSubclause, songartistSubclause);
  if (!songSubSQL.empty())
  {
    songSub.where = ""; // Clear where for song + song_artist clause
    songSub.AppendWhere("(" + songSubSQL + ")");
    songSub.BuildSQL(songSubSQL);
  }

  // Build album_artist subquery clause
  std::string albumartistSubclause;
  if (!artistClauseAlbum.empty())
    albumArtistSub.AppendWhere("(" + artistClauseAlbum + ")");
  // Album artists only with other role rules.
  // Song/song_artist subquery is inside album_artist subquery
  if (bAlbumartistAndRole)
    albumArtistSub.AppendWhere(songSubSQL);
  albumArtistSub.BuildSQL(albumartistSubclause);
  //Role rules  but not "albumartist" and no  FieldAlbumartist
  if (bRoleRules && !bAlbumArtists && albumartistField.empty())
    albumartistSubclause.clear();

  // An "albumartist" role rule same as FieldAlbumartist
  // FieldAlbumartist rules overrides any NOT "albumartist" role rule
  if (!albumartistField.empty() || (bAlbumArtists && !bSongArtists))
  {
    if (bRoleRules)
      rule = albumartistSubclause;
    else if (!artistField.empty() || !bAlbumArtists)
      rule = CombineClause(albumartistSubclause, songSubSQL);
    else
      rule = CombineClause(albumartistSubclause, songSubSQLNoArtist);
  }
  else if (!songartistSubclause.empty())
  {
    // FieldArtist, role or other Artist rules
    if (m_type != CombinationAnd)
    {
      rule = CombineClause(albumartistSubclause, songSubSQL);
    }
    else
    {
      //Repeat song rules for album and song artist routes
      rule = CombineClause(albumartistSubclause, songSubSQLNoArtist);
      if (!rule.empty())
        rule += " OR ";
      rule += songSubSQL;
    }
  }
  else if (!songSubSQL.empty())
    rule = songSubSQL;

  return rule;
}

std::string CSmartPlaylistRuleCombination::GetSongsWhereClause(const CDatabase& db) const
{
  std::string rule;
  // Processing for FieldArtist, FieldAlbumArtist fields, and other "artists" or "albums" rule fields
  // FieldSource is handled with the other "albums"  rule fields

  // Examine what role fields and build roles clause
  bool bAlbumArtists = true;
  bool bSongArtists = true;
  bool bJoinRole = false;
  bool bRoleRules = false;
  std::string roleClause =
      GetRolesWhereClause(db, bAlbumArtists, bSongArtists, bJoinRole, bRoleRules);

  // Examine "artists" and "songs" rules, building basic SQL clauses
  std::string albumSubclause;
  std::string albumSourceclause;
  std::string artistSubclause;
  std::string albumartistField;
  std::string artistField;
  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    std::string currentRule;
    if (IsFieldNative((Field)(*it)->m_field, MediaTypeAlbum, "artist"))
    { //"artists" fields including FieldArtist and FieldAlbumArtist
      std::string query = (*it)->GetWhereClause(db, "artist");
      StringUtils::Replace(query, "artistview", "artist");
      if ((*it)->m_field == FieldArtist)
        artistField = CombineClause(artistField, query);
      else if ((*it)->m_field == FieldAlbumArtist)
        albumartistField = CombineClause(albumartistField, query);
      else
        artistSubclause = CombineClause(artistSubclause, query);
    }
    else if (IsFieldNative((Field)(*it)->m_field, MediaTypeArtist, "album"))
    { //"albums" field including
      std::string query = (*it)->GetWhereClause(db, "albums");
      StringUtils::Replace(query, "albumview", "album");
      albumSubclause = CombineClause(albumSubclause, query);
    }
    else if ((*it)->m_field == FieldSource)
    {
      std::string query = (*it)->GetWhereClause(db, "albums");
      StringUtils::Replace(query, "albumview", "album");
      albumSourceclause = CombineClause(albumSourceclause, query);
    }
  }

  // Album artists only flag
  // An "albumartist" role rule same as FieldAlbumartist
  // FieldAlbumartist rules overrides any NOT "albumartist" role rule
  bool bAlbumartistOnly = !albumartistField.empty() || (bAlbumArtists && !bSongArtists);
  // Album artists only with other role rules flag
  bool bAlbumartistAndRole = bRoleRules && bAlbumartistOnly;

  // Translate the rules into SQL subqueries
  std::string albumSubSQL;
  if (!albumSubclause.empty())
  {
    CDatabase::ExistsSubQuery albumSub("album", "album.idAlbum = songview.idAlbum");
    if (!albumSourceclause.empty())
      // source clause within album subquery
      albumSubclause = CombineClause(albumSubclause, albumSourceclause);
    albumSub.AppendWhere("(" + albumSubclause + ")");
    albumSub.BuildSQL(albumSubSQL);
  }
  else if (!albumSourceclause.empty())
  {
    // Source clause does not need album table use album_source directly
    StringUtils::Replace(albumSourceclause, "album.idAlbum", "songview.idAlbum");
    albumSubSQL = albumSourceclause;
  }

  CDatabase::ExistsSubQuery albumArtistSub("album_artist",
                                           "album_artist.idAlbum = songview.idAlbum");
  albumArtistSub.AppendJoin("JOIN artist ON artist.idArtist = album_artist.idArtist");

  CDatabase::ExistsSubQuery songArtistSub("song_artist", "song_artist.idSong = songview.idSong");
  if (bAlbumartistAndRole)
    // Album artists only with other role rules, add correlation to album_artist table
    songArtistSub.AppendWhere("song_artist.idArtist = album_artist.idArtist");
  else if (!artistSubclause.empty() || !albumartistField.empty() || !artistField.empty())
    // JOIN artist when have artist clause (may be just role rule)
    songArtistSub.AppendJoin("JOIN artist ON artist.idArtist = song_artist.idArtist");
  if (!roleClause.empty())
  {
    if (bJoinRole)
    {
      roleClause = "(" + roleClause + ")";
      songArtistSub.AppendJoin("JOIN role ON song_artist.idRole = role.idRole");
    }
    songArtistSub.AppendWhere(roleClause);
  }

  // Build combined artist clause for inclusion in album and song artist subqueries
  std::string artistClauseAlbum;
  std::string artistClauseSong;
  if (!albumartistField.empty() && !artistField.empty())
  { // Separate artist clauses for song and album artist routes
    artistClauseAlbum = CombineClause(artistSubclause, albumartistField);
    artistClauseSong = CombineClause(artistSubclause, artistField);
  }
  else
  {
    artistSubclause = CombineClause(artistSubclause, albumartistField);
    artistSubclause = CombineClause(artistSubclause, artistField);
    artistClauseAlbum = artistSubclause;
    artistClauseSong = artistSubclause;
  }

  // Build song_artist subquery clause
  std::string songartistSubclause;
  if (!artistClauseSong.empty() || bRoleRules)
  {
    if (!artistClauseSong.empty() && !bAlbumartistAndRole)
      songArtistSub.AppendWhere("(" + artistClauseSong + ")");
    songArtistSub.BuildSQL(songartistSubclause);
  }

  // Build album_artist subquery clause
  std::string albumartistSubclause;
  if (!artistClauseAlbum.empty())
    albumArtistSub.AppendWhere("(" + artistClauseAlbum + ")");
  // Album artists only with other role rules. Apply to the same artist so
  // song_artist subquery is inside album_artist subquery
  if (bAlbumartistAndRole)
    albumArtistSub.AppendWhere(songartistSubclause);
  albumArtistSub.BuildSQL(albumartistSubclause);
  //Role rules  but not "albumartist" and no  FieldAlbumartist
  if (bRoleRules && !bAlbumArtists && albumartistField.empty())
    albumartistSubclause.clear();

  // Build full SQL clause
  if (bAlbumartistOnly)
    rule = albumartistSubclause;
  else if (bRoleRules)
    rule = songartistSubclause;
  else if (!artistField.empty() && !albumartistField.empty())
    rule = CombineClause(albumartistSubclause, songartistSubclause);
  else if (!artistClauseSong.empty())
    rule = albumartistSubclause + " OR " + songartistSubclause;

  // Combine album rule SQL
  if (!albumSubSQL.empty())
    rule = CombineClause(albumSubSQL, rule);
  return rule;
}

bool CSmartPlaylistRuleCombination::IsFieldRuleSimple(Field field, const MediaType& mediaType)
{
  if (field == FieldNone || mediaType == MediaTypeNone)
    return true;

  // Only music media types can have rules that need to be combined where clause
  if (mediaType != MediaTypeArtist && mediaType != MediaTypeAlbum && mediaType != MediaTypeSong)
    return true;

  if (mediaType == MediaTypeAlbum)
  {
    //Catch "albums" FieldArtist, FieldAlbumArtist fields that are in albumview but not simple
    //Allow "albums" FieldSource field, isn't in albumview but clause can be built individually
    if (field == FieldArtist || field == FieldAlbumArtist)
      return false;
    if (field == FieldSource)
      return true;
  }
  else if (mediaType == MediaTypeSong)
  {
    //Catch "songs" FieldArtist, FieldAlbumArtist fields that are in songview but not simple
    if (field == FieldArtist || field == FieldAlbumArtist)
      return false;
  }
  // Generally rules can be applied individually when they are fields of the
  // table/view directly related to the media type
  std::string strField;
  strField = DatabaseUtils::GetField(field, mediaType, DatabaseQueryPartWhere);
  return !strField.empty();
}

bool CSmartPlaylistRuleCombination::IsFieldNative(Field field,
                                                  const MediaType& mediaType,
                                                  const std::string& strTable)
{
  std::string strTablename;
  strTablename = DatabaseUtils::GetNativeTable(field, mediaType);
  return (!strTablename.empty() && strTablename == strTable);
}

std::string CSmartPlaylistRuleCombination::CombineClause(const std::string& original,
                                                         const std::string& clause) const
{
  std::string rule = original;
  if (!clause.empty())
  {
    if (!original.empty())
    {
      rule += m_type == CombinationAnd ? " AND " : " OR ";
      rule += "(";
    }
    rule += clause;
    if (!original.empty())
      rule += ")";
  }
  return rule;
}

void CSmartPlaylistRuleCombination::GetVirtualFolders(const std::string& strType, std::vector<std::string> &virtualFolders) const
{
  for (CDatabaseQueryRuleCombinations::const_iterator it = m_combinations.begin(); it != m_combinations.end(); ++it)
  {
    std::shared_ptr<CSmartPlaylistRuleCombination> combo = std::static_pointer_cast<CSmartPlaylistRuleCombination>(*it);
    if (combo)
      combo->GetVirtualFolders(strType, virtualFolders);
  }

  for (CDatabaseQueryRules::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it)
  {
    if (((*it)->m_field != FieldVirtualFolder && (*it)->m_field != FieldPlaylist) || (*it)->m_operator != CDatabaseQueryRule::OPERATOR_EQUALS)
      continue;

    std::string playlistFile = CSmartPlaylistDirectory::GetPlaylistByName((*it)->m_parameter.at(0), strType);
    if (playlistFile.empty())
      continue;

    if ((*it)->m_field == FieldVirtualFolder)
      virtualFolders.push_back(playlistFile);
    else
    {
      // look for any virtual folders in the expanded playlists
      CSmartPlaylist playlist;
      if (!playlist.Load(playlistFile))
        continue;

      if (CSmartPlaylist::CheckTypeCompatibility(playlist.GetType(), strType))
        playlist.GetVirtualFolders(virtualFolders);
    }
  }
}

void CSmartPlaylistRuleCombination::AddRule(const CSmartPlaylistRule &rule)
{
  std::shared_ptr<CSmartPlaylistRule> ptr(new CSmartPlaylistRule(rule));
  m_rules.push_back(ptr);
}

CSmartPlaylist::CSmartPlaylist()
{
  Reset();
}

bool CSmartPlaylist::OpenAndReadName(const CURL &url)
{
  if (readNameFromPath(url) == NULL)
    return false;

  return !m_playlistName.empty();
}

const TiXmlNode* CSmartPlaylist::readName(const TiXmlNode *root)
{
  if (root == NULL)
    return NULL;

  const TiXmlElement *rootElem = root->ToElement();
  if (rootElem == NULL)
    return NULL;

  if (!root || !StringUtils::EqualsNoCase(root->Value(),"smartplaylist"))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist");
    return NULL;
  }

  // load the playlist type
  const char* type = rootElem->Attribute("type");
  if (type)
    m_playlistType = type;
  // backward compatibility:
  if (m_playlistType == "music")
    m_playlistType = "songs";
  if (m_playlistType == "video")
    m_playlistType = "musicvideos";

  // load the playlist name
  XMLUtils::GetString(root, "name", m_playlistName);

  return root;
}

const TiXmlNode* CSmartPlaylist::readNameFromPath(const CURL &url)
{
  CFileStream file;
  if (!file.Open(url))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist %s (failed to read file)", url.GetRedacted().c_str());
    return NULL;
  }

  m_xmlDoc.Clear();
  file >> m_xmlDoc;

  const TiXmlNode *root = readName(m_xmlDoc.RootElement());
  if (m_playlistName.empty())
  {
    m_playlistName = CUtil::GetTitleFromPath(url.Get());
    if (URIUtils::HasExtension(m_playlistName, ".xsp"))
      URIUtils::RemoveExtension(m_playlistName);
  }

  return root;
}

const TiXmlNode* CSmartPlaylist::readNameFromXml(const std::string &xml)
{
  if (xml.empty())
  {
    CLog::Log(LOGERROR, "Error loading empty Smart playlist");
    return NULL;
  }

  m_xmlDoc.Clear();
  if (!m_xmlDoc.Parse(xml))
  {
    CLog::Log(LOGERROR, "Error loading Smart playlist (failed to parse xml: %s)", m_xmlDoc.ErrorDesc());
    return NULL;
  }

  const TiXmlNode *root = readName(m_xmlDoc.RootElement());

  return root;
}

bool CSmartPlaylist::load(const TiXmlNode *root)
{
  if (root == NULL)
    return false;

  return LoadFromXML(root);
}

bool CSmartPlaylist::Load(const CURL &url)
{
  return load(readNameFromPath(url));
}

bool CSmartPlaylist::Load(const std::string &path)
{
  const CURL pathToUrl(path);
  return load(readNameFromPath(pathToUrl));
}

bool CSmartPlaylist::Load(const CVariant &obj)
{
  if (!obj.isObject())
    return false;

  // load the playlist type
  if (obj.isMember("type") && obj["type"].isString())
    m_playlistType = obj["type"].asString();

  // backward compatibility
  if (m_playlistType == "music")
    m_playlistType = "songs";
  if (m_playlistType == "video")
    m_playlistType = "musicvideos";

  // load the playlist name
  if (obj.isMember("name") && obj["name"].isString())
    m_playlistName = obj["name"].asString();

  if (obj.isMember("rules"))
    m_ruleCombination.Load(obj["rules"], this);

  // Sort the rules by field
  m_ruleCombination.sort();

  if (obj.isMember("group") && obj["group"].isMember("type") && obj["group"]["type"].isString())
  {
    m_group = obj["group"]["type"].asString();
    if (obj["group"].isMember("mixed") && obj["group"]["mixed"].isBoolean())
      m_groupMixed = obj["group"]["mixed"].asBoolean();
  }

  // now any limits
  if (obj.isMember("limit") && (obj["limit"].isInteger() || obj["limit"].isUnsignedInteger()) && obj["limit"].asUnsignedInteger() > 0)
    m_limit = (unsigned int)obj["limit"].asUnsignedInteger();

  // and order
  if (obj.isMember("order") && obj["order"].isMember("method") && obj["order"]["method"].isString())
  {
    const CVariant &order = obj["order"];
    if (order.isMember("direction") && order["direction"].isString())
      m_orderDirection = StringUtils::EqualsNoCase(order["direction"].asString(), "ascending") ? SortOrderAscending : SortOrderDescending;

    if (order.isMember("ignorefolders") && obj["ignorefolders"].isBoolean())
      m_orderAttributes = obj["ignorefolders"].asBoolean() ? SortAttributeIgnoreFolders : SortAttributeNone;

    m_orderField = CSmartPlaylistRule::TranslateOrder(obj["order"]["method"].asString().c_str());
  }

  return true;
}

bool CSmartPlaylist::LoadFromXml(const std::string &xml)
{
  return load(readNameFromXml(xml));
}

bool CSmartPlaylist::LoadFromXML(const TiXmlNode *root, const std::string &encoding)
{
  if (!root)
    return false;

  std::string tmp;
  if (XMLUtils::GetString(root, "match", tmp))
    m_ruleCombination.SetType(StringUtils::EqualsNoCase(tmp, "all") ? CSmartPlaylistRuleCombination::CombinationAnd : CSmartPlaylistRuleCombination::CombinationOr);

  // now the rules
  const TiXmlNode *ruleNode = root->FirstChild("rule");
  while (ruleNode)
  {
    CSmartPlaylistRule rule;
    if (rule.Load(ruleNode, encoding))
      m_ruleCombination.AddRule(rule);

    ruleNode = ruleNode->NextSibling("rule");
  }
  // Sort the rules by field ! @Todo: unneeded
  // m_ruleCombination.sort();

  const TiXmlElement *groupElement = root->FirstChildElement("group");
  if (groupElement != NULL && groupElement->FirstChild() != NULL)
  {
    m_group = groupElement->FirstChild()->ValueStr();
    const char* mixed = groupElement->Attribute("mixed");
    m_groupMixed = mixed != NULL && StringUtils::EqualsNoCase(mixed, "true");
  }

  // now any limits
  // format is <limit>25</limit>
  XMLUtils::GetUInt(root, "limit", m_limit);

  // and order
  // format is <order direction="ascending">field</order>
  const TiXmlElement *order = root->FirstChildElement("order");
  if (order && order->FirstChild())
  {
    const char *direction = order->Attribute("direction");
    if (direction)
      m_orderDirection = StringUtils::EqualsNoCase(direction, "ascending") ? SortOrderAscending : SortOrderDescending;

    const char *ignorefolders = order->Attribute("ignorefolders");
    if (ignorefolders != NULL)
      m_orderAttributes = StringUtils::EqualsNoCase(ignorefolders, "true") ? SortAttributeIgnoreFolders : SortAttributeNone;

    m_orderField = CSmartPlaylistRule::TranslateOrder(order->FirstChild()->Value());
  }
  return true;
}

bool CSmartPlaylist::LoadFromJson(const std::string &json)
{
  if (json.empty())
    return false;

  CVariant obj;
  if (!CJSONVariantParser::Parse(json, obj))
    return false;

  return Load(obj);
}

bool CSmartPlaylist::Save(const std::string &path) const
{
  CXBMCTinyXML doc;
  TiXmlDeclaration decl("1.0", "UTF-8", "yes");
  doc.InsertEndChild(decl);

  TiXmlElement xmlRootElement("smartplaylist");
  xmlRootElement.SetAttribute("type",m_playlistType.c_str());
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot)
    return false;

  // add the <name> tag
  XMLUtils::SetString(pRoot, "name", m_playlistName);

  // add the <match> tag
  XMLUtils::SetString(pRoot, "match", m_ruleCombination.GetType() == CSmartPlaylistRuleCombination::CombinationAnd ? "all" : "one");

  // add <rule> tags
  m_ruleCombination.Save(pRoot);

  // add <group> tag if necessary
  if (!m_group.empty())
  {
    TiXmlElement nodeGroup("group");
    if (m_groupMixed)
      nodeGroup.SetAttribute("mixed", "true");
    TiXmlText group(m_group.c_str());
    nodeGroup.InsertEndChild(group);
    pRoot->InsertEndChild(nodeGroup);
  }

  // add <limit> tag
  if (m_limit)
    XMLUtils::SetInt(pRoot, "limit", m_limit);

  // add <order> tag
  if (m_orderField != SortByNone)
  {
    TiXmlText order(CSmartPlaylistRule::TranslateOrder(m_orderField).c_str());
    TiXmlElement nodeOrder("order");
    nodeOrder.SetAttribute("direction", m_orderDirection == SortOrderDescending ? "descending" : "ascending");
    if (m_orderAttributes & SortAttributeIgnoreFolders)
      nodeOrder.SetAttribute("ignorefolders", "true");
    nodeOrder.InsertEndChild(order);
    pRoot->InsertEndChild(nodeOrder);
  }
  return doc.SaveFile(path);
}

bool CSmartPlaylist::Save(CVariant &obj, bool full /* = true */) const
{
  if (obj.type() == CVariant::VariantTypeConstNull)
    return false;

  obj.clear();
  // add "type"
  obj["type"] = m_playlistType;

  // add "rules"
  CVariant rulesObj = CVariant(CVariant::VariantTypeObject);
  if (m_ruleCombination.Save(rulesObj))
    obj["rules"] = rulesObj;

  // add "group"
  if (!m_group.empty())
  {
    obj["group"]["type"] = m_group;
    obj["group"]["mixed"] = m_groupMixed;
  }

  // add "limit"
  if (full && m_limit)
    obj["limit"] = m_limit;

  // add "order"
  if (full && m_orderField != SortByNone)
  {
    obj["order"] = CVariant(CVariant::VariantTypeObject);
    obj["order"]["method"] = CSmartPlaylistRule::TranslateOrder(m_orderField);
    obj["order"]["direction"] = m_orderDirection == SortOrderDescending ? "descending" : "ascending";
    obj["order"]["ignorefolders"] = (m_orderAttributes & SortAttributeIgnoreFolders);
  }

  return true;
}

bool CSmartPlaylist::SaveAsJson(std::string &json, bool full /* = true */) const
{
  CVariant xsp(CVariant::VariantTypeObject);
  if (!Save(xsp, full))
    return false;

  return CJSONVariantWriter::Write(xsp, json, true) && !json.empty();
}

void CSmartPlaylist::Reset()
{
  m_ruleCombination.clear();
  m_limit = 0;
  m_orderField = SortByNone;
  m_orderDirection = SortOrderNone;
  m_orderAttributes = SortAttributeNone;
  m_playlistType = "songs"; // sane default
  m_group.clear();
  m_groupMixed = false;
}

void CSmartPlaylist::SetName(const std::string &name)
{
  m_playlistName = name;
}

void CSmartPlaylist::SetType(const std::string &type)
{
  m_playlistType = type;
}

bool CSmartPlaylist::IsVideoType() const
{
  return IsVideoType(m_playlistType);
}

bool CSmartPlaylist::IsMusicType() const
{
  return IsMusicType(m_playlistType);
}

bool CSmartPlaylist::IsVideoType(const std::string &type)
{
  return type == "movies" || type == "tvshows" || type == "episodes" ||
         type == "musicvideos" || type == "mixed";
}

bool CSmartPlaylist::IsMusicType(const std::string &type)
{
  return type == "artists" || type == "albums" ||
         type == "songs" || type == "mixed";
}

std::string CSmartPlaylist::GetWhereClause(const CDatabase &db, std::set<std::string> &referencedPlaylists) const
{
  return m_ruleCombination.GetWhereClause(db, GetType(), referencedPlaylists);
}

void CSmartPlaylist::GetVirtualFolders(std::vector<std::string> &virtualFolders) const
{
  m_ruleCombination.GetVirtualFolders(GetType(), virtualFolders);
}

std::string CSmartPlaylist::GetSaveLocation() const
{
  if (m_playlistType == "mixed")
    return "mixed";
  if (IsMusicType())
    return "music";
  // all others are video
  return "video";
}

void CSmartPlaylist::GetAvailableFields(const std::string &type, std::vector<std::string> &fieldList)
{
  std::vector<Field> typeFields = CSmartPlaylistRule::GetFields(type);
  for (std::vector<Field>::const_iterator field = typeFields.begin(); field != typeFields.end(); ++field)
  {
    for (const translateField& i : fields)
    {
      if (*field == i.field)
        fieldList.emplace_back(i.string);
    }
  }
}

bool CSmartPlaylist::IsEmpty(bool ignoreSortAndLimit /* = true */) const
{
  bool empty = m_ruleCombination.empty();
  if (empty && !ignoreSortAndLimit)
    empty = m_limit <= 0 && m_orderField == SortByNone && m_orderDirection == SortOrderNone;

  return empty;
}

bool CSmartPlaylist::CheckTypeCompatibility(const std::string &typeLeft, const std::string &typeRight)
{
  if (typeLeft == typeRight)
    return true;

  if (typeLeft == "mixed" &&
     (typeRight == "songs" || typeRight == "musicvideos"))
    return true;

  if (typeRight == "mixed" &&
     (typeLeft == "songs" || typeLeft == "musicvideos"))
    return true;

  return false;
}

CDatabaseQueryRule *CSmartPlaylist::CreateRule() const
{
  return new CSmartPlaylistRule();
}
CDatabaseQueryRuleCombination *CSmartPlaylist::CreateCombination() const
{
  return new CSmartPlaylistRuleCombination();
}
