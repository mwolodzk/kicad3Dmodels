/*
 *      file: keyvalparser.h
 *
 *      Copyright 2012 Dr. Cirilo Bernardo (cjh.bernardo@gmail.com)
 *
 *      This program is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */


#ifndef KEYVALPARSER_H
#define KEYVALPARSER_H

#include <string>
#include <map>

/**
 *      Class KeyValParser reads a file and extracts
 *      key:value pairs. Lines beginning with '#' are
 *      ignored. Spaces within a key name are discarded;
 *      leading and trailing spaces within key names and
 *      values are discarded.
 */
class KeyValParser
{
private:
    std::map<std::string, std::string> keyvals;
    int parseLine(const std::string &line);

public:
    virtual ~KeyValParser();

    int LoadKeys(const std::string &filename);
    void ClearKeys(void);
    const std::map<std::string, std::string> &GetKeys(void);
};

#endif  // KEYVALPARSER_H