#ifndef GENERATOR_H
#define GENERATOR_H

#include <string>
#include "opts.h"

class Generator {
    public:
        Generator(std::string name, std::string description) : name(name), description(description)
        { }
        virtual ~Generator() {}
        virtual void run() const = 0;
        const std::string name;
        const std::string description;
};
#endif
