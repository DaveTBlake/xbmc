set(SOURCES GUIDialogInfoProviderSettings.cpp
            GUIDialogMusicInfo.cpp
            GUIDialogMusicOSD.cpp
            GUIDialogRefreshSettings.cpp
            GUIDialogSongInfo.cpp
            GUIDialogVisualisationPresetList.cpp)

set(HEADERS GUIDialogInfoProviderSettings.h
            GUIDialogMusicInfo.h
            GUIDialogMusicOSD.h
            GUIDialogRefreshSettings.h
            GUIDialogSongInfo.h
            GUIDialogVisualisationPresetList.h)

core_add_library(music_dialogs)
