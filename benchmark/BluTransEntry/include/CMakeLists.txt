add_subdirectory(BluBooster)
add_subdirectory(Core)
add_subdirectory(Editor)

add_library(BluTransLinkInclude INTERFACE)
add_library(BluTrans::Include ALIAS BluTransLinkInclude)

target_link_libraries(BluTransLinkInclude INTERFACE
    BluTrans::Include::BluBlaster
    BluTrans::Include::Core
    BluTrans::Include::Editor
)
