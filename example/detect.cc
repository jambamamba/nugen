#include <iostream>

#include "TfLiteInterpreter.h"

//convert cat.png -resize 224x224! cat224x224.rgb
//convert cat.png -resize 300x300! cat300x300.rgb


int main(int argc, char**argv)
{
    if(argc != 2)
    {
        std::cerr << "./classify <path to 300x300 rgb file>\n";
        return -1;
    }
    TfLiteInterpreter interpreter(TfLiteInterpreter::Type::Detector);

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
              << "\" at"
              << " (" << res.bounding_rect_.x << "," << res.bounding_rect_.y << ")=>"
              << " (" << res.bounding_rect_.x + res.bounding_rect_.width << "," << res.bounding_rect_.y + res.bounding_rect_.height << ")"
              << " in "
              << res.milliseconds_
              << " milliseconds"
              << "\n";

    return 0;
}
