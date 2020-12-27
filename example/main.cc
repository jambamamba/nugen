#include <iostream>

#include "TfLiteInterpreter.h"

//convert cat.bmp -resize 224x224! cat.rgb


int main(int argc, char**argv)
{
    if(argc != 2)
    {
        std::cerr << "./classify <path to 422x422 rgb file>\n";
        return -1;
    }
    TfLiteInterpreter interpreter;

    if(!interpreter.Create())
    {
        return -1;
    }
    if(!interpreter.LoadImage(argv[1]))
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
