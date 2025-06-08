add_library(BluTransSourceEditor STATIC
    Splash/BluTransSplash.cpp
)
add_library(BluTrans::Source::Editor ALIAS BluTransSourceEditor)
