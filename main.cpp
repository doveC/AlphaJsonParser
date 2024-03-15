#include <iostream>
#include <string>
#include "JsonParser.h"

using namespace std;

int main()
{
    std::ifstream fin("demo.txt");
    std::stringstream ss; 
    ss << fin.rdbuf();

    std::string s(ss.str());
    auto x = json::parser(s).value();

    cout << x << endl;

    x["web-app"]["servlet"][0]["init-param"]["cachePackageTagsTrack"].value = 7654321;

    cout << x << endl;
}