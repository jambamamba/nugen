#include <iostream>

#include "TfLiteInterpreter.h"

//convert cat.png -resize 224x224! cat224x224.rgb


int main(int argc, char**argv)
{
    if(argc != 2)
    {
        std::cerr << "./classify <path to 224x224 rgb file>\n";
        return -1;
    }
    TfLiteInterpreter interpreter(TfLiteInterpreter::Type::Classifier);

    if(!interpreter.Create())
    {
        return -1;
    }
    if(!interpreter.LoadImage(argv[1]))
    {
        return -1;
    }

    auto res = interpreter.Inference();
    std::cout << "Inferenced class \"" << res.objects_.at(0).class_
              << "\" in "
              << res.milliseconds_
              << " milliseconds"
              << "\n";

    return 0;
}
