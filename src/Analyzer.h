#pragma once

#include "ofMain.h"
#include <deque>
#include <vector>


class Analyzer {
/*
    General Analyzer parent class for Fake Detection analyzers
*/

protected:
    float score = 0.5f;

public:
    virtual ~Analyzer() = default;
    virtual void update(const std::vector<glm::vec2>& landmarks) = 0;
    virtual void reset() = 0;

    float getScore() const { return score; }
};
