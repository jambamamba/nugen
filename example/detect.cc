#include <iostream>

#include "TfLiteInterpreter.h"

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
    std::cout << "Inferenced in " << res.milliseconds_
              << " milliseconds"
              << "\n";
    for(const auto &object: res.objects_)
    {
        std::cout << "class \"" << object.class_
                  << "\", score:"
                  << object.score_
                  << ", at "
                  << "(" << object.bounding_rect_.x << "," << object.bounding_rect_.y << ")=>"
                  << "(" << object.bounding_rect_.x + object.bounding_rect_.width << "," << object.bounding_rect_.y + object.bounding_rect_.height << ")"
                  << "\n";
    }

    return 0;
}
