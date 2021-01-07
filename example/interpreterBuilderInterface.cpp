#include "interpreterBuilderInterface.h"

#include "tensorflow/lite/model_builder.h"

InterpreterBuilderInterface::InterpreterBuilderInterface(const std::string &model_path)
    : model_path_(model_path)
{}
InterpreterBuilderInterface::~InterpreterBuilderInterface() = default;
