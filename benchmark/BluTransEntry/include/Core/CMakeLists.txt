add_library(BluTransIncludeCore INTERFACE
    Container/Any/Any.h
)
add_library(BluTrans::Include::Core ALIAS BluTransIncludeCore)
