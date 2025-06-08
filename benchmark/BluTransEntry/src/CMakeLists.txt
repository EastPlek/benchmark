add_subdirectory(Core)
add_subdirectory(Editor)

add_library(BluTransLinkSource STATIC 
    BlutransLinkSource.cpp
)
add_library(BluTrans::Source ALIAS BluTransLinkSource)

target_link_libraries(BluTransLinkSource PRIVATE
    BluTrans::Source::Core
    BluTrans::Source::Editor
)
