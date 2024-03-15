#include <iostream>
#include <variant>
#include <vector>
#include <map>
#include <optional>
#include <string>
#include <fstream>
#include <sstream>

namespace json 
{

struct Node;
using Null = std::monostate;
using Bool = bool;
using Int = int64_t;
using Float = double;
using String = std::string;
using Array = std::vector<Node>;
using Object = std::map<std::string, Node>;
using Value = std::variant<Null, Bool, Int, Float, String, Array, Object>;

struct Node 
{
    Value value;

    Node() : value(Null{}) {}
    Node(Value _value) : value(_value) {}

    auto& operator[](const std::string& key) 
    {
        if (auto object = std::get_if<Object>(&value)) 
        {
            return  (*object)[key];
        }

        throw std::runtime_error("not an object");
    }

    auto& operator[](size_t index) 
    {
        if (auto array = std::get_if<Array>(&value)) 
        {
            return array->at(index);
        }

        throw std::runtime_error("not an array");
    }

    void push(const Node& rhs) 
    {
        if (auto array = std::get_if<Array>(&value)) 
        {
            array->push_back(rhs);
        }
    }
};

struct JsonParser 
{
    std::string_view json_str;
    size_t pos = 0;

    void parseWhiteSpace() 
    {
        while (pos < json_str.size() && std::isspace(json_str[pos]))
        {
            ++pos;
        }
    }

    auto parseNull() -> std::optional<Value> 
    {
        if (json_str.substr(pos, 4) == "null") 
        {
            pos += 4;
            return Null{};
        }

        return{};
    }

    auto parseTrue() -> std::optional<Value> 
    {
        if (json_str.substr(pos, 4) == "true") 
        {
            pos += 4;
            return true;
        }

        return {};
    }

    auto parseFalse() -> std::optional<Value> 
    {
        if (json_str.substr(pos, 5) == "false") 
        {
            pos += 5;
            return false;
        }

        return {};
    }

    auto parseNumber() -> std::optional<Value> 
    {
        size_t endpos = pos;

        while (endpos < json_str.size() && (
            std::isdigit(json_str[endpos]) ||
            json_str[endpos] == 'e' ||
            json_str[endpos] == '.')) 
        {
            endpos++;
        }

        std::string number(json_str.substr(pos, endpos - pos));
        pos = endpos;

        static auto isFloat = [](std::string& number) {
            return number.find('.') != number.npos || number.find('e') != number.npos;
        };

        if (isFloat(number)) 
        {
            try 
            {
                Float ret = std::stod(number);
                return ret;
            }
            catch (...) 
            {
                return {};
            }
        }
        else 
        {
            try 
            {
                Int ret = std::stoi(number);
                return ret;
            }
            catch (...) 
            {
                return {};
            }
        }
    }

    auto parseString() -> std::optional<Value> 
    {
        pos++;  // skip left "

        size_t endpos = pos;
        while (pos < json_str.size() && json_str[endpos] != '"') 
        {
            endpos++;
        }

        std::string str(json_str.substr(pos, endpos - pos));
        pos = endpos + 1;  // skip right "

        return str;
    }

    auto parseArray() -> std::optional<Value> 
    {
        pos++;  // skip [

        Array arr;
        while (pos < json_str.size() && json_str[pos] != ']') 
        {
            auto value = parseValue();
            arr.push_back(value.value());
            parseWhiteSpace();
            if (pos < json_str.size() && json_str[pos] == ',') 
            {
                pos++;  // skip ,
            }
            parseWhiteSpace();
        }

        pos++;  // skip ]
        return arr;
    }

    auto parseObject() -> std::optional<Value> 
    {
        pos++;  // skip {

        Object obj;
        while (pos < json_str.size() && json_str[pos] != '}') 
        {
            auto key = parseValue();
            if (!std::holds_alternative<String>(key.value())) 
            {
                return {};
            }

            parseWhiteSpace();
            if (pos < json_str.size() && json_str[pos] == ':') 
            {
                pos++;  // skip :
            }
            parseWhiteSpace();

            auto val = parseValue();
            obj[std::get<String>(key.value())] = val.value();

            parseWhiteSpace();
            if (pos < json_str.size() && json_str[pos] == ',') 
            {
                pos++;  // skip ,
            }
            parseWhiteSpace();
        }
        pos++;// }
        return obj;

    }

    auto parseValue() ->std::optional<Value> 
    {
        parseWhiteSpace();
        switch (json_str[pos]) 
        {
            case 'n':
                return parseNull();
            case 't':
                return parseTrue();
            case 'f':
                return parseFalse();
            case '"':
                return parseString();
            case '[':
                return parseArray();
            case '{':
                return parseObject();
            default:
                return parseNumber();
        }
    }

    auto parse() -> std::optional<Node> 
    {
        parseWhiteSpace();
        auto value = parseValue();
        if (!value) 
        {
            return {};
        }

        return Node(*value);
    }
};

auto parser(std::string_view json_str) -> std::optional<Node> 
{
    JsonParser p{ json_str };
    return p.parse();
}

class JsonGenerator 
{
public:
    static auto generate(const Node& node) -> std::string {
        return std::visit(
            [](auto&& arg) -> std::string {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, Null>) 
                {
                    return "null";
                }
                else if constexpr (std::is_same_v<T, Bool>) 
                {
                    return arg ? "true" : "false";
                }
                else if constexpr (std::is_same_v<T, Int>) 
                {
                    return std::to_string(arg);
                }
                else if constexpr (std::is_same_v<T, Float>) 
                {
                    return std::to_string(arg);
                }
                else if constexpr (std::is_same_v<T, String>)
                {
                    return generateString(arg);
                }
                else if constexpr (std::is_same_v<T, Array>) 
                {
                    return generateArray(arg);
                }
                else if constexpr (std::is_same_v<T, Object>) 
                {
                    return generateObject(arg);
                }
            },
            node.value);
    }
    static auto generateString(const String& str) -> std::string 
    {
        std::string json_str = "\"";
        json_str += str;
        json_str += '"';

        return json_str;
    }
    static auto generateArray(const Array& array) -> std::string 
    {
        std::string json_str = "[";
        for (const auto& node : array) 
        {
            json_str += generate(node);
            json_str += ',';
        }

        if (!array.empty()) 
        {
            json_str.pop_back();
        }
        json_str += ']';

        return json_str;
    }
    static auto generateObject(const Object& object) -> std::string 
    {
        std::string json_str = "{";

        for (const auto& [key, node] : object) 
        {
            json_str += generateString(key);
            json_str += ':';
            json_str += generate(node);
            json_str += ',';
        }

        if (!object.empty())
        {
            json_str.pop_back();
        }
        json_str += '}';

        return json_str;
    }
};

class JsonGeneratorFormat
{
public:
    static int currentIndentLevel;

    static auto generateToFormatString(const Node& node) -> std::string
    {
        currentIndentLevel = 0;
        return generate(node);
    }

    static auto generate(const Node& node, const bool needIndent = true) -> std::string
    {
        return std::visit(
            [needIndent](auto&& arg) -> std::string {
                using T = std::decay_t<decltype(arg)>;

                std::string str;
                if (needIndent)
                {
                    addIndent(str);
                }

                if constexpr (std::is_same_v<T, Null>) 
                {
                    str += "null";
                }
                else if constexpr (std::is_same_v<T, Bool>) 
                {
                    arg ? (str += "true") : (str += "false");
                }
                else if constexpr (std::is_same_v<T, Int>) 
                {
                    str += std::to_string(arg);
                }
                else if constexpr (std::is_same_v<T, Float>) 
                {
                    str += std::to_string(arg);
                }
                else if constexpr (std::is_same_v<T, String>)
                {
                    str += generateString(arg);
                }
                else if constexpr (std::is_same_v<T, Array>) 
                {
                    str += generateArray(arg);
                }
                else if constexpr (std::is_same_v<T, Object>) 
                {
                    str += generateObject(arg);
                }

                return str;
            },
            node.value);
    }
    static auto generateString(const String& str) -> std::string 
    {
        std::string res = "\"";
        res += str;
        res += '"';

        return res;
    }
    static auto generateArray(const Array& array) -> std::string 
    {
        std::string str;
        
        str += "[\n";
        currentIndentLevel++;
        for (size_t i = 0; i < array.size(); i++)
        {
            str += generate(array[i]);
            if (i != array.size() - 1)
            {
                str += ',';
            }
            str += '\n';
        }

        currentIndentLevel--;
        addIndent(str);
        str += "]";

        return str;
    }
    static auto generateObject(const Object& object) -> std::string 
    {
        std::string str;
        
        str += "{\n";
        currentIndentLevel++;
        for (const auto& [key, node] : object)
        {
            addIndent(str);
            str += generateString(key);
            str += ": ";
            str += generate(node, false);
            str += ",\n";
        }

        if (!object.empty())
        {
            str.pop_back();
            str.pop_back();
            str += '\n';
        }

        currentIndentLevel--;
        addIndent(str);
        str += "}";

        return str;
    }
    static void addIndent(std::string& str)
    {
        for (int i = 0; i < currentIndentLevel; i++)
        {
            str += "  ";
        }
    }
};
int JsonGeneratorFormat::currentIndentLevel = 0;

auto operator<<(std::ostream& out, const Node& t) -> std::ostream& 
{
    out << JsonGeneratorFormat::generate(t);

    return out;
}
}