#pragma once

#include <iostream>
#include <string>
#include <vector>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

std::string ReadLine();

int ReadLineWithNumber();

std::vector<std::string> SplitIntoWords(const std::string& text);