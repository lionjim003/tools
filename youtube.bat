@setlocal enabledelayedexpansion&&set I=1&&set SEARCHKEYWORD=%1
@FOR %%i in (%*) do @if !I!==1 (set /A I+=1) else set SEARCHKEYWORD=!SEARCHKEYWORD!+%%i
@start https://www.youtube.com/results?search_query=%SEARCHKEYWORD%