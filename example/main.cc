#include <iostream>

#include "TfLiteInterpreter.h"

//convert cat.bmp -resize 224x224! cat.rgb


int main(int argc, char**argv)
{
    TfLiteInterpreter interpreter;

    if(!interpreter.Create())
    {
        return -1;
    }
    if(!interpreter.LoadImage("/tmp/cat.rgb"))
    {
        return -1;
    }

    auto res = interpreter.Inference();
    std::cout << "Inferenced class \"" << res.class_
              << "\" in "
              << res.milliseconds_
              << " milliseconds"
              << "\n";

    return 0;
}
